#!/usr/bin/env python3
# Copyright (C) 2026 OpenAI Codex, for szampan
# SPDX-License-Identifier: GPL-3.0-only
"""Private maintainer relink using a verified kit and previously compiled game.

Uses the captured RADV recipe and existing native tools. The output contains
owner-derived compiled game code and is never a public release artifact.
"""
import argparse,json,os,re,shutil,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];sys.path.insert(0,str(ROOT))
from builder.core import Builder,BuildError,sha,verify_kit,verify_native_elf

def main():
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('kit','guest-archive','output'):p.add_argument('--'+name,type=Path,required=True)
    a=p.parse_args();kit=a.kit.resolve();work=a.output.resolve();manifest=verify_kit(kit)
    guest=a.guest_archive.resolve()
    with guest.open('rb') as f:
        if f.read(8)!=b'!<arch>\n':raise BuildError('Expected a compiled private guest archive')
    members=subprocess.check_output([str(kit/'tools/llvm-ar.exe'),'t',str(guest)],text=True,timeout=60).splitlines()
    expected={'x86_func_mapping.o',*('x86_recomp.%d.o'%i for i in range(64))}
    if set(members)!=expected or len(members)!=65:raise BuildError('Unexpected guest archive member inventory')
    work.mkdir(parents=True,exist_ok=False);(work/'tmp').mkdir()
    shutil.copyfile(guest,work/'libsims2-guest.a')
    shutil.copytree(kit/'title',work/'PPSA99145')
    job=Builder(kit,work/'unused.iso',work/'unused-output');job.work=work
    argv=job.expand(manifest['link_argv']);(work/'link-arguments.json').write_text(json.dumps(argv,indent=2)+'\n')
    if os.name=='nt' and len(subprocess.list2cmdline(argv))>30000:
        rsp=work/'link.rsp';rsp.write_text(subprocess.list2cmdline(argv[1:])+'\n',encoding='utf-8')
        argv=[argv[0],'--rsp-quoting=windows','@'+str(rsp)]
    print('Linking private game with refreshed runtime',flush=True);job.run(argv,'link',timeout=1800)
    verify_native_elf(work/'llvm-pie.elf')
    log=job.run(job.expand(manifest['undefined_argv']),'undefined-imports')
    imports=set(re.findall(r'\b[Uwv]\s+(\S+)',log.read_text()))
    unknown=imports-set(manifest['allowed_imports'])
    if unknown:raise BuildError('Unexpected native imports: '+', '.join(sorted(unknown)))
    for stage in ('convert','sign','inspect'):job.run(job.expand(manifest[stage+'_argv']),stage)
    if 'integrity: valid' not in (work/'inspect.log').read_text():raise BuildError('Native SELF integrity not verified')
    title=work/'PPSA99145'
    receipt=dict(status='BUILD_PASS_RUNTIME_NOT_TESTED',kind='private_owner_derived_game',
        runtime_build_id=manifest['runtime_build_id'],kit_sha256=sha(kit/'kit.json'),guest_archive_sha256=sha(guest),
        title=str(title),debug_elf=str(work/'llvm-pie.elf'),native_elf=str(work/'eboot.elf'),
        files={p.relative_to(title).as_posix():dict(bytes=p.stat().st_size,sha256=sha(p)) for p in sorted(title.rglob('*')) if p.is_file()})
    (work/'game-build.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps(dict(status=receipt['status'],runtime_build_id=receipt['runtime_build_id'],title=str(title))),flush=True)

if __name__=='__main__':main()
