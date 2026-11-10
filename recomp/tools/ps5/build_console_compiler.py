#!/usr/bin/env python3
# Copyright (C) 2026 OpenAI Codex, for szampan
# SPDX-License-Identifier: GPL-3.0-only
"""Build the embedded compiler libraries, independently of the working title.

Host TableGen tools must run on the development PC. The second build produces
PS5 libraries, NOT host executables renamed as console programs. No deployment,
firmware changes or public-release replacement is performed by this tool.
"""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import time
import ctypes
import shlex

LLVM_REVISION = 'be5b58f51c93c67947b7e18cd45c2f9913f2c311'
ROOT = Path(__file__).resolve().parents[2]


def command_arguments(command):
    if os.name!='nt':return shlex.split(command)
    shell=ctypes.WinDLL('shell32',use_last_error=True)
    kernel=ctypes.WinDLL('kernel32',use_last_error=True)
    shell.CommandLineToArgvW.argtypes=[ctypes.c_wchar_p,ctypes.POINTER(ctypes.c_int)]
    shell.CommandLineToArgvW.restype=ctypes.POINTER(ctypes.c_wchar_p)
    kernel.LocalFree.argtypes=[ctypes.c_void_p]
    kernel.LocalFree.restype=ctypes.c_void_p
    count=ctypes.c_int()
    values=shell.CommandLineToArgvW(command,ctypes.byref(count))
    if not values:raise ctypes.WinError(ctypes.get_last_error())
    try:return [values[i] for i in range(count.value)]
    finally:kernel.LocalFree(values)


def run(argv, directory, label):
    directory.mkdir(parents=True, exist_ok=True)
    log = directory / (label + '.log')
    started = time.time()
    record = dict(stage=label, argv=[str(x) for x in argv], started=started,
                  status='RUNNING', pid=os.getpid())
    state = directory / ('state-'+label+'.json')
    state.write_text(json.dumps(record, indent=2) + '\n')
    (directory / (label + '.json')).write_text(json.dumps(record, indent=2) + '\n')
    print(json.dumps(dict(stage=label, status='RUNNING', log=str(log))), flush=True)
    with log.open('ab') as output:
        result = subprocess.run([str(x) for x in argv], stdout=output, stderr=subprocess.STDOUT)
    record.update(status='PASS' if result.returncode == 0 else 'FAIL',
                  exit_code=result.returncode, seconds=round(time.time()-started, 3))
    state.write_text(json.dumps(record, indent=2) + '\n')
    (directory / (label + '.json')).write_text(json.dumps(record, indent=2) + '\n')
    print(json.dumps({k:v for k,v in record.items() if k != 'argv'}), flush=True)
    if result.returncode:
        print(log.read_text(errors='replace')[-10000:], flush=True)
        raise SystemExit(result.returncode)


def common(a):
    return [a.cmake, '-S', a.llvm/'llvm', '-G', 'Ninja',
            '-DCMAKE_MAKE_PROGRAM='+str(a.ninja), '-DCMAKE_BUILD_TYPE=Release',
            '-DLLVM_ENABLE_PROJECTS=clang;lld', '-DLLVM_TARGETS_TO_BUILD=X86',
            '-DLLVM_INCLUDE_TESTS=OFF', '-DLLVM_INCLUDE_BENCHMARKS=OFF',
            '-DLLVM_INCLUDE_EXAMPLES=OFF', '-DLLVM_INCLUDE_DOCS=OFF',
            '-DCLANG_INCLUDE_TESTS=OFF', '-DCLANG_INCLUDE_DOCS=OFF',
            '-DCLANG_ENABLE_STATIC_ANALYZER=OFF', '-DCLANG_ENABLE_HLSL=OFF',
            '-DLLVM_ENABLE_ZLIB=OFF', '-DLLVM_ENABLE_ZSTD=OFF',
            '-DLLVM_ENABLE_LIBXML2=OFF', '-DLLVM_ENABLE_LIBEDIT=OFF',
            '-DLLVM_ENABLE_LIBPFM=OFF', '-DLLVM_ENABLE_CURL=OFF',
            '-DLLVM_ENABLE_ASSERTIONS=OFF', '-DLLVM_ENABLE_RTTI=ON',
            '-DLLVM_ENABLE_EH=ON', '-DPython3_EXECUTABLE='+str(a.python)]


def main():
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('llvm','cmake','ninja','clang_bin','python','kit'):
        p.add_argument('--'+name.replace('_','-'), type=Path)
    p.add_argument('--settings',type=Path,help='Reuse maintainer tool paths from a previous invocation')
    p.add_argument('--output',type=Path)
    p.add_argument('--phase',choices=('configure-host','build-host','configure-ps5','build-support','build-wrapper','build-linker','build-ps5','configure-generator','build-generator','continue','check-interface','test-fatal','finish-current'),required=True)
    p.add_argument('--jobs',type=int,default=2)
    a=p.parse_args()
    saved=json.loads(a.settings.read_text()) if a.settings else {}
    for name in ('llvm','cmake','ninja','clang_bin','python','kit','output'):
        if getattr(a,name) is None and name in saved:setattr(a,name,Path(saved[name]))
    if a.output is None:a.output=ROOT/'out/console-compiler'
    missing=[name for name in ('llvm','cmake','ninja','clang_bin','python','kit') if getattr(a,name) is None]
    if missing:p.error('Missing tool paths: '+', '.join(missing))
    if not 1 <= a.jobs <= 4: p.error('Use 1..4 bounded compilation jobs')
    for name in ('llvm','cmake','ninja','clang_bin','python','kit','output'):
        setattr(a,name,getattr(a,name).resolve())
    os.environ['PATH']=str(a.clang_bin)+os.pathsep+os.environ.get('PATH','')
    git=shutil.which('git')
    if not git:p.error('Git must be on PATH to check the pinned LLVM revision')
    revision=subprocess.check_output([git,'-C',str(a.llvm),'rev-parse','HEAD'],text=True,timeout=30).strip()
    if revision!=LLVM_REVISION:p.error('Unexpected LLVM revision: '+revision)
    a.output.mkdir(parents=True,exist_ok=True)
    (a.output/'build-settings.json').write_text(json.dumps({name:str(getattr(a,name))
        for name in ('llvm','cmake','ninja','clang_bin','python','kit','output')},indent=2)+'\n')
    (a.output/'source-version.json').write_text(json.dumps(dict(
        llvm_revision=revision,llvm_source=str(a.llvm),kit=str(a.kit),
        authored_component='compiler/embedded.cpp',console_status='NOT_TESTED'),indent=2)+'\n')
    host=a.output/'host'; target=a.output/'ps5'
    if a.phase=='configure-host':
        run(common(a)+['-B',host,'-DCMAKE_C_COMPILER='+str(a.clang_bin/'clang.exe'),
            '-DCMAKE_CXX_COMPILER='+str(a.clang_bin/'clang++.exe'),
            '-DLLVM_USE_LINKER=lld'],a.output,a.phase)
    elif a.phase=='build-host':
        run([a.ninja,'-C',host,'-j',str(a.jobs),'llvm-tblgen','llvm-min-tblgen','clang-tblgen'],a.output,a.phase)
    elif a.phase=='configure-ps5':
        if not (host/'CMakeCache.txt').is_file():p.error('Configure the host build first')
        # LLVM's positive EH/RTTI options assume the compiler's defaults enable
        # them. The PS5 driver defaults disable both, so make that ABI explicit.
        # CMake's *_FLAGS_INIT only applies to a fresh cache; migrate ours too.
        migrated=[]
        if (target/'CMakeCache.txt').is_file():
            for line in (target/'CMakeCache.txt').read_text().splitlines():
                if line.startswith('CMAKE_CXX_FLAGS:STRING='):
                    flags=line.split('=',1)[1]
                    for flag in ('-fexceptions','-frtti'):
                        if flag not in flags.split():flags+=' '+flag
                    migrated=['-DCMAKE_CXX_FLAGS='+flags]
        run(common(a)+['-B',target,
            '-DCMAKE_TOOLCHAIN_FILE='+str(ROOT/'compiler/ps5-toolchain.cmake'),
            '-DSIMS2_COMPILER_BIN='+a.clang_bin.as_posix(),
            '-DSIMS2_PUBLIC_KIT='+a.kit.as_posix(),
            '-DLLVM_TABLEGEN='+(host/'bin/llvm-tblgen.exe').as_posix(),
            '-DCLANG_TABLEGEN='+(host/'bin/clang-tblgen.exe').as_posix(),
            '-DLLVM_HOST_TRIPLE=x86_64-sie-ps5',
            '-DLLVM_DEFAULT_TARGET_TRIPLE=x86_64-sie-ps5',
            '-DLLVM_ENABLE_PROJECTS=clang;lld',
            '-DLLVM_EXTERNAL_PROJECTS=sims2compiler',
            '-DLLVM_EXTERNAL_SIMS2COMPILER_SOURCE_DIR='+(ROOT/'compiler').as_posix(),
            '-DLLVM_ENABLE_BACKTRACES=OFF', '-DLLVM_ENABLE_CRASH_OVERRIDES=OFF',
            '-DLLVM_ENABLE_PLUGINS=OFF', '-DLLVM_BUILD_TOOLS=OFF',
            '-DCLANG_BUILD_TOOLS=OFF']+migrated,a.output,a.phase)
    elif a.phase=='configure-generator':
        run([a.cmake,'-S',ROOT/'compiler/generator','-B',a.output/'generator-ps5','-G','Ninja',
             '-DCMAKE_MAKE_PROGRAM='+str(a.ninja),'-DCMAKE_BUILD_TYPE=Release',
             '-DCMAKE_TOOLCHAIN_FILE='+str(ROOT/'compiler/ps5-toolchain.cmake'),
             '-DSIMS2_COMPILER_BIN='+a.clang_bin.as_posix(),
             '-DSIMS2_PUBLIC_KIT='+a.kit.as_posix()],a.output,a.phase)
    elif a.phase=='build-generator':
        run([a.ninja,'-C',a.output/'generator-ps5','-j',str(a.jobs),'sims2-console-generator'],a.output,a.phase)
    elif a.phase=='test-fatal':
        run([a.cmake,'-S',ROOT/'compiler/fatal-test','-B',a.output/'fatal-host','-G','Ninja',
             '-DCMAKE_MAKE_PROGRAM='+str(a.ninja),'-DCMAKE_BUILD_TYPE=Release',
             '-DCMAKE_CXX_COMPILER='+str(a.clang_bin/'clang++.exe'),
             '-DLLVM_DIR='+(host/'lib/cmake/llvm').as_posix()],a.output,'fatal-host-configure')
        run([a.ninja,'-C',a.output/'fatal-host','-j','1'],a.output,'fatal-host-build')
        run([a.output/'fatal-host/sims2-compiler-fatal-test.exe'],a.output,'fatal-host-run')
    elif a.phase=='finish-current':
        # A fresh Ninja pass is required after source/CMake changes made during
        # the long build; never label its old graph as the final current artifact.
        state=a.output/'state-finish-current.json'
        state.write_text(json.dumps(dict(stage=a.phase,status='WAITING',pid=os.getpid(),started=time.time()))+'\n')
        print(json.dumps(dict(stage=a.phase,status='WAITING',for_stage='build-ps5')),flush=True)
        deadline=time.monotonic()+6*3600
        while True:
            try:
                saved=json.loads((a.output/'build-ps5.json').read_text())
            except (FileNotFoundError,json.JSONDecodeError):saved={}
            if saved.get('status') in ('PASS','FAIL'):break
            if time.monotonic()>deadline:raise SystemExit('Timed out awaiting build-ps5')
            time.sleep(2)
        if saved.get('status')!='PASS':raise SystemExit('Build failed; inspect before retrying')
        run([a.ninja,'-C',target,'-j',str(a.jobs),'sims2-console-compiler'],a.output,'build-ps5-final')
        state.write_text(json.dumps(dict(stage=a.phase,status='PASS',pid=os.getpid(),finished=time.time()))+'\n')
    elif a.phase=='build-linker':
        run([a.ninja,'-C',target,'-j',str(a.jobs),'sims2-console-linker','lldELF'],a.output,a.phase)
    elif a.phase=='build-support':
        run([a.ninja,'-C',target,'-j',str(a.jobs),'LLVMSupport'],a.output,a.phase)
    elif a.phase=='continue':
        # Another explicitly started Ninja currently owns the PS5 build tree.
        # Wait for its recorded verdict, never run two Ninja writers there.
        deadline=time.monotonic()+6*3600
        for prerequisite in ('build-host','build-support'):
            verdict=a.output/(prerequisite+'.json')
            print(json.dumps(dict(status='WAITING',stage=prerequisite)),flush=True)
            while True:
                saved=None
                try:
                    if verdict.is_file():saved=json.loads(verdict.read_text())
                except json.JSONDecodeError:
                    pass # A short concurrent status write is not a verdict.
                if saved and saved.get('status') in ('PASS','FAIL'):break
                if time.monotonic()>deadline:raise SystemExit('Timed out awaiting '+prerequisite)
                time.sleep(2)
            if saved.get('status')!='PASS':raise SystemExit('Prerequisite failed: '+prerequisite)
        base=[sys.executable,str(Path(__file__).resolve())]
        for key in ('llvm','cmake','ninja','clang_bin','python','kit','output'):
            base+=['--'+key.replace('_','-'),str(getattr(a,key))]
        for stage in ('build-wrapper','build-ps5'):
            run(base+['--jobs',str(a.jobs),'--phase',stage],a.output,'pipeline-'+stage)
    elif a.phase=='check-interface':
        # Read Ninja's actual compile command; emit separate objects outside its
        # build tree so this probe can run while the long library build owns it.
        listing=subprocess.check_output([str(a.ninja),'-C',str(target),'-t','targets','all'],text=True)
        objects=[line.split(': ',1)[0] for line in listing.splitlines()
                 if line.split(': ',1)[0].endswith('/embedded.cpp.o')]
        if len(objects)!=1:p.error('Cannot identify the embedded compiler object')
        database=json.loads(subprocess.check_output([str(a.ninja),'-C',str(target),'-t','compdb-targets',objects[0]],text=True))
        entries=[entry for entry in database if Path(entry['file']).name=='embedded.cpp']
        if len(entries)!=1:p.error('Cannot identify the interface compile command')
        original=command_arguments(entries[0]['command'])
        if Path(original[0]).resolve()!=(a.clang_bin/'clang-23.exe').resolve():p.error('Unexpected compiler in command database')
        clean=[];index=0
        while index<len(original):
            argument=original[index]
            if argument in ('-o','-MF','-MT','-MQ'):index+=2;continue
            if argument in ('-MD','-MMD'):index+=1;continue
            if Path(argument).name=='embedded.cpp':index+=1;continue
            clean.append(argument);index+=1
        probe=a.output/'interface-check';probe.mkdir(exist_ok=True)
        for name in ('embedded','linker','worker','fatal'):
            run(clean+[ROOT/'compiler'/(name+'.cpp'),'-o',probe/(name+'.o')],a.output,'check-interface-'+name)
    elif a.phase=='build-wrapper':
        for tool in ('llvm-tblgen.exe','llvm-min-tblgen.exe','clang-tblgen.exe'):
            if not (host/'bin'/tool).is_file():p.error('Build host tool first: '+tool)
        listing=subprocess.check_output([str(a.ninja),'-C',str(target),'-t','targets','all'],text=True)
        objects=[line.split(': ',1)[0] for line in listing.splitlines()
                 if line.split(': ',1)[0].endswith('/embedded.cpp.o')]
        if len(objects)!=1:p.error('Cannot identify the embedded compiler object')
        run([a.ninja,'-C',target,'-j',str(a.jobs),objects[0]],a.output,a.phase)
    else:
        for tool in ('llvm-tblgen.exe','llvm-min-tblgen.exe','clang-tblgen.exe'):
            if not (host/'bin'/tool).is_file():p.error('Build host tool first: '+tool)
        run([a.ninja,'-C',target,'-j',str(a.jobs),'sims2-console-compiler'],a.output,a.phase)


if __name__=='__main__':main()
