# Copyright (C) 2026 OpenAI Codex, for szampan
# SPDX-License-Identifier: GPL-3.0-only
"""Portable owner-copy build pipeline. No shell, remote compiler or VM."""
from __future__ import annotations
from pathlib import Path, PurePosixPath
import hashlib
import json
import os
import platform
import re
import shutil
import stat
import struct
import subprocess
import threading
import time
import uuid
from concurrent.futures import ThreadPoolExecutor, as_completed

EXPECTED_XBE = '16f9e41ce26c315a758f25d43ba1fa9d3809d1ad'


class BuildError(RuntimeError): pass
class Cancelled(BuildError): pass


def absolute(path):
    return Path(os.path.abspath(path)).resolve()


def sha(path, algorithm='sha256'):
    with Path(path).open('rb') as f:
        return hashlib.file_digest(f, algorithm).hexdigest()


def verify_native_elf(path):
    with Path(path).open('rb') as f:
        header=f.read(64)
        if len(header)!=64 or header[:6]!=b'\x7fELF\x02\x01' or header[18:20]!=b'\x3e\x00':
            raise BuildError('Native image is not an x86-64 ELF')
        if header[16:18]!=b'\x03\x00':raise BuildError('Native image is not position-independent (ET_DYN)')
        phoff=struct.unpack_from('<Q',header,32)[0]
        phsize,phnum=struct.unpack_from('<HH',header,54)
        if phsize!=56 or not 1<=phnum<=128:raise BuildError('Unexpected ELF segment table')
        f.seek(phoff)
        loads=[]
        for _ in range(phnum):
            ph=f.read(phsize)
            if len(ph)!=phsize:raise BuildError('Truncated ELF segment table')
            kind,flags,offset,address,_,filesize,memsize,alignment=struct.unpack('<IIQQQQQQ',ph)
            if kind!=1:continue
            if alignment<0x4000 or address%alignment!=offset%alignment or filesize>memsize:
                raise BuildError('Native ELF does not meet the SDK 16 KiB load-segment layout')
            loads.append((address,memsize,flags))
        entry=struct.unpack_from('<Q',header,24)[0]
        if not any(address<=entry<address+size and flags&1 for address,size,flags in loads):
            raise BuildError('Native entry point is outside executable segments')


def relative(root, name):
    p = PurePosixPath(name)
    if not name or p.is_absolute() or '..' in p.parts or '\\' in name or ':' in name:
        raise BuildError('Invalid kit path: '+name)
    target = root.joinpath(*p.parts)
    if not absolute(target).is_relative_to(absolute(root)):
        raise BuildError('Kit path escapes its root')
    if any(x.is_symlink() for x in [target, *target.parents] if x != root.parent):
        raise BuildError('Linked kit paths are unsupported')
    return target


def verify_kit(root, cancel=None):
    root = absolute(root)
    manifest = json.loads((root/'kit.json').read_text(encoding='utf-8'))
    if manifest.get('schema') != 1 or manifest.get('kind') != 'public_owner_builder':
        raise BuildError('Unsupported builder kit')
    if manifest.get('xbe_sha1') != EXPECTED_XBE or manifest.get('title_id') != 'PPSA99145':
        raise BuildError('Wrong game revision or title identity in kit')
    actual = {}
    def scan(directory,prefix=''):
        with os.scandir(directory) as entries:
            for entry in entries:
                if cancel and cancel.is_set():raise Cancelled('Cancelled')
                info=entry.stat(follow_symlinks=False)
                if stat.S_ISLNK(info.st_mode) or getattr(info,'st_file_attributes',0)&0x400:
                    raise BuildError('Linked/reparse kit paths are unsupported')
                name=prefix+entry.name
                if stat.S_ISDIR(info.st_mode):scan(entry.path,name+'/')
                elif stat.S_ISREG(info.st_mode):actual[name]=(Path(entry.path),info.st_size)
                else:raise BuildError('Special kit file is unsupported')
    scan(root)
    if set(actual) != set(manifest['files']) | {'kit.json'}:
        raise BuildError('Builder kit has missing or unexpected files; unpack a fresh copy')
    for name, record in manifest['files'].items():
        if cancel and cancel.is_set(): raise Cancelled('Cancelled')
        # The complete directory scan above checked each descendant once,
        # including junctions. Re-resolving every ancestor for every SDK header
        # made startup needlessly slow on Windows and network filesystems.
        p,size = actual[name]
        if size != record['bytes'] or sha(p) != record['sha256']:
            raise BuildError('Damaged builder kit file: '+name)
    return manifest


class Builder:
    def __init__(self, kit, iso, output, notify=None, cancel=None, resume_work=None):
        self.kit, self.iso, self.output = map(absolute, (kit, iso, output))
        self.notify = notify or (lambda stage, detail: None)
        self.cancel = cancel or threading.Event()
        self.work = None
        self.state = 'NEW'
        self.tool_metrics = []
        self.resume_work = absolute(resume_work) if resume_work else None

    def event(self, stage, detail):
        self.state = stage
        self.notify(stage, detail)
        if self.work:
            record = dict(stage=stage, detail=detail, elapsed_seconds=round(time.monotonic()-self.started,2))
            with (self.work/'events.jsonl').open('a',encoding='utf-8') as f:
                f.write(json.dumps(record)+'\n')

    def expand(self, argv, **extra):
        values = dict(KIT=str(self.kit), WORK=str(self.work), ISO=str(self.iso),
                      XBE=str(self.work/'default.xbe'), GENERATED=str(self.work/'generated'),
                      TITLE=str(self.work/'PPSA99145'), **extra)
        def one(arg):
            return re.sub(r'\{([A-Z_]+)\}',lambda m: values[m[1]],arg)
        return [one(x) for x in argv]

    def run(self, argv, log_name, timeout=1200):
        if self.cancel.is_set(): raise Cancelled('Cancelled')
        log = self.work/(log_name+'.log')
        env = dict(os.environ)
        env['PATH'] = str(self.kit/'tools')+os.pathsep+env.get('PATH','')
        env['TMP'] = env['TEMP'] = str(self.work/'tmp')
        env['TMPDIR'] = str(self.work/'tmp')
        start = time.monotonic()
        peak_rss = 0
        def sample_memory(process):
            if os.name != 'nt':return
            import ctypes
            from ctypes import wintypes
            class Counters(ctypes.Structure):
                _fields_=[('cb',wintypes.DWORD),('faults',wintypes.DWORD),
                          *[(name,ctypes.c_size_t) for name in ('peak_rss','rss','peak_pool','pool',
                             'peak_nonpaged','nonpaged','pagefile','peak_pagefile')]]
            counters=Counters();counters.cb=ctypes.sizeof(counters)
            query=ctypes.windll.psapi.GetProcessMemoryInfo
            query.argtypes=(wintypes.HANDLE,ctypes.POINTER(Counters),wintypes.DWORD)
            if query(wintypes.HANDLE(int(process._handle)),ctypes.byref(counters),counters.cb):
                nonlocal peak_rss
                peak_rss=max(peak_rss,counters.peak_rss)
        creationflags = subprocess.CREATE_NO_WINDOW if os.name == 'nt' else 0
        with log.open('wb') as out:
            process = subprocess.Popen(argv,cwd=self.work,stdout=out,stderr=subprocess.STDOUT,
                                       env=env,creationflags=creationflags)
            try:
                while process.poll() is None:
                    sample_memory(process)
                    if self.cancel.wait(.15): raise Cancelled('Cancelled; your ISO was kept')
                    if time.monotonic()-start > timeout:
                        raise BuildError('Timed out during '+log_name+'; see '+str(log))
            except BaseException:
                if process.poll() is None:
                    process.terminate()
                    try: process.wait(timeout=5)
                    except subprocess.TimeoutExpired:
                        process.kill(); process.wait()
                raise
        self.tool_metrics.append(dict(tool=log_name,seconds=round(time.monotonic()-start,3),
                                      peak_working_set_bytes=peak_rss or None))
        if process.returncode:
            tail = log.read_text(encoding='utf-8',errors='replace')[-5000:]
            raise BuildError(log_name+' failed (code '+str(process.returncode)+')\n'+tail)
        return log

    def build(self):
        self.started = time.monotonic()
        if self.output.exists():
            raise BuildError('Choose a NEW output folder. Existing folders and saves are never overwritten.')
        if not self.iso.is_file() or self.iso.suffix.lower() != '.iso':
            raise BuildError('Select your Xbox USA .iso file')
        if self.output == self.kit or self.output.is_relative_to(self.kit):
            raise BuildError('Output must be outside the builder kit')
        self.event('VERIFY_KIT','Checking bundled tools and runtime')
        manifest = verify_kit(self.kit, self.cancel)
        host = 'windows-x86_64' if os.name == 'nt' else 'macos-'+platform.machine()
        if manifest['host'] != host:
            raise BuildError('This kit is for '+manifest['host']+'; use the bundle for '+host)
        self.output.parent.mkdir(parents=True,exist_ok=True)
        if shutil.disk_usage(self.output.parent).free < manifest['required_free_bytes']:
            raise BuildError('Not enough free storage in the output location')
        self.work = self.resume_work or self.output.parent/('.sims2-build-'+uuid.uuid4().hex[:12])
        compile_identity=dict(argv=manifest['compile_argv'],generated=manifest['generated_files'],
            files={n:r['sha256'] for n,r in manifest['files'].items()
                   if n.startswith(('tools/','sdk/','runtime/','clang-resource/'))})
        fingerprint=hashlib.sha256(json.dumps(compile_identity,sort_keys=True).encode()).hexdigest()
        source_info=dict(path=str(self.iso),bytes=self.iso.stat().st_size,mtime_ns=self.iso.stat().st_mtime_ns)
        if self.resume_work:
            if self.work.parent!=self.output.parent or not re.fullmatch(r'\.sims2-build-[a-f0-9]{12}',self.work.name):
                raise BuildError('Resume folder must be a builder workspace beside the output')
            cache=json.loads((self.work/'build-cache.json').read_text())
            if cache.get('schema')!=1 or cache.get('compile_fingerprint')!=fingerprint or cache.get('iso')!=source_info:
                raise BuildError('Resume cache does not match this ISO and compiler kit')
            if sha(self.work/'default.xbe','sha1')!=EXPECTED_XBE:
                raise BuildError('Resume XBE fingerprint mismatch')
        else:
            self.work.mkdir();(self.work/'tmp').mkdir()
            cache=dict(schema=1,compile_fingerprint=fingerprint,iso=source_info,objects={})
        def save_cache():
            temp=self.work/'build-cache.json.new'
            temp.write_text(json.dumps(cache,indent=2)+'\n')
            temp.replace(self.work/'build-cache.json')
        try:
            self.event('EXTRACT_XBE','Reading default.xbe from your ISO')
            if not self.resume_work:self.run(self.expand(manifest['extract_xbe_argv']),'extract-xbe')
            if sha(self.work/'default.xbe','sha1') != EXPECTED_XBE:
                raise BuildError('Unsupported game revision. This builder requires the original Xbox USA release.')
            self.event('GENERATE','Translating your game into native C++')
            if not self.resume_work:
                (self.work/'generated').mkdir()
                self.run(self.expand(manifest['generate_argv']),'generate')
            generated = self.work/'generated'
            expected = manifest['generated_files']
            actual = {p.name for p in generated.iterdir() if p.is_file()}
            if actual != set(expected):
                raise BuildError('Generated file inventory differs from the reviewed USA revision')
            for name, digest in expected.items():
                if sha(generated/name) != digest:
                    raise BuildError('Generated code differs from the reviewed translation: '+name)
            sources = sorted(generated.glob('*.cpp'))
            if len(sources) != manifest['generated_cpp_count']:
                raise BuildError('Unexpected number of generated compilation units')
            objects = [self.work/(src.stem+'.o') for src in sources]
            save_cache()
            def compile_one(src,obj):
                if cache['objects'].get(src.name) and obj.is_file() and sha(obj)==cache['objects'][src.name]:
                    return src.name
                self.run(self.expand(manifest['compile_argv'], SOURCE=str(src), OBJECT=str(obj)),
                         'compile-'+src.stem)
                return src.name
            workers=max(1,min(2,int(manifest.get('compile_jobs',1))))
            self.event('COMPILE',f'Compiling 0/{len(sources)} ({workers} tasks)')
            pool=ThreadPoolExecutor(max_workers=workers)
            futures=[]
            try:
                futures=[pool.submit(compile_one,src,obj) for src,obj in zip(sources,objects)]
                for done,future in enumerate(as_completed(futures),1):
                    name=future.result()
                    cache['objects'][name]=sha(self.work/(Path(name).stem+'.o'))
                    save_cache()
                    self.event('COMPILE',f'Compiled {done}/{len(sources)}: '+name)
            except BaseException:
                self.cancel.set()
                for future in futures:future.cancel()
                raise
            finally:
                pool.shutdown(wait=True,cancel_futures=True)
            self.event('ARCHIVE','Combining your compiled game functions')
            archive=self.work/'libsims2-guest.a'
            if archive.exists():archive.rename(self.work/('libsims2-guest.previous-'+uuid.uuid4().hex[:8]+'.a'))
            self.run(self.expand(manifest['archive_argv'])+[str(x) for x in objects],'archive')
            self.event('LINK','Linking the PS5 title with the prepared runtime and RADV')
            self.run(self.expand(manifest['link_argv']),'link',timeout=1800)
            self.event('CHECK_IMPORTS','Checking native import bindings')
            log = self.run(self.expand(manifest['undefined_argv']),'undefined-imports')
            symbols = set()
            for line in log.read_text(errors='replace').splitlines():
                m=re.search(r'\bU\s+(\S+)',line)
                if m: symbols.add(m[1].split('@')[0])
            unsupported = symbols-set(manifest['allowed_imports'])
            if unsupported: raise BuildError('Unsupported native imports: '+', '.join(sorted(unsupported)))
            elf = self.work/'llvm-pie.elf'
            verify_native_elf(elf)
            self.event('PACKAGE','Creating eboot.bin and title resources')
            title = self.work/'PPSA99145'
            if title.exists():title.rename(self.work/('PPSA99145.previous-'+uuid.uuid4().hex[:8]))
            shutil.copytree(self.kit/'title',title)
            self.run(self.expand(manifest['convert_argv']),'convert')
            self.run(self.expand(manifest['sign_argv']),'sign')
            self.run(self.expand(manifest['inspect_argv']),'inspect')
            self.event('INSTALL_ASSETS','Extracting and patching your game data (this can take several minutes)')
            self.run(self.expand(manifest['install_argv']),'install-assets',timeout=3600)
            param = json.loads((title/'sce_sys/param.json').read_text())
            if param['titleId'] != manifest['title_id']: raise BuildError('Wrong packaged title identity')
            for name in ('eboot.bin','sce_module/libc.prx','assets/default.xbe','assets/DATA/index.ind'):
                if not (title/name).is_file(): raise BuildError('Incomplete title: '+name)
            result = dict(schema=1, status='BUILD_PASS_RUNTIME_NOT_TESTED',
                          runtime_build_id=manifest['runtime_build_id'], xbe_sha1=EXPECTED_XBE,
                          eboot_sha256=sha(title/'eboot.bin'),debug_elf_sha256=sha(elf),
                          elapsed_seconds=round(time.monotonic()-self.started,2),
                          tool_metrics=self.tool_metrics,
                          files={p.relative_to(title).as_posix():dict(bytes=p.stat().st_size,sha256=sha(p))
                                 for p in sorted(title.rglob('*')) if p.is_file()})
            (title/'OWNER-BUILD.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
            if self.cancel.is_set(): raise Cancelled('Cancelled; your ISO was kept')
            # Publish the entire new directory in one rename; never merge with saves.
            if self.output.exists(): raise BuildError('Output appeared during build; refusing to overwrite')
            title.rename(self.output)
            self.event('COMPLETE','Ready: '+str(self.output))
            return result
        except BaseException as error:
            self.event('CANCELLED' if isinstance(error,Cancelled) else 'FAILED',str(error))
            raise
