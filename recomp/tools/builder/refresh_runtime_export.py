#!/usr/bin/env python3
# Copyright (C) 2026 OpenAI Codex, for szampan
# SPDX-License-Identifier: GPL-3.0-only
"""Refresh an evaluated RADV recipe with the actual CMake runtime objects.

Maintainer-only. Never exports the generated guest archive or owner game data.
Keeps the captured SDK/RADV flags, CRT, driver and archive ordering intact.
"""
import argparse,hashlib,json,re,shutil,subprocess
from pathlib import Path

def sha(path):
    with path.open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()

def main():
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('baseline','build','clang-bin','output'):p.add_argument('--'+name,type=Path,required=True)
    a=p.parse_args();build=a.build.resolve();out=a.output.resolve()
    state=json.loads((build/'objects-build-status.json').read_text(encoding='utf-8-sig'))
    profile=json.loads((build/'build-profile.json').read_text(encoding='utf-8-sig'))
    identity=(build/'build-id.txt').read_text().strip()
    if state['state']!='OBJECTS_READY' or state['build_id']!=identity or profile['build_id']!=identity:
        raise ValueError('Current native object build is not complete')
    for key,value in {'optimization':'O2','fast_handles':'ON','object_wait_channels':'ON'}.items():
        if profile.get(key)!=value:raise ValueError('Unexpected runtime profile: '+key)
    recipe=json.loads((a.baseline/'native-recipe.json').read_text())
    if not recipe.get('sdk_wrapper_evaluated'):raise ValueError('Use the established evaluated RADV export')
    arguments=recipe['link_argv'];pattern=r'^\{KIT\}/native/inputs/(?:\d{3}-\d{3}|runtime-\d{3})-.+\.(?:cpp|c)\.o$'
    old=[(i,x) for i,x in enumerate(arguments) if re.fullmatch(pattern,x)]
    if not old or old[-1][0]-old[0][0]+1!=len(old):raise ValueError('Runtime object boundary changed; review the recipe')
    objects=[Path(x) for x in (build/'runtime-objects.txt').read_text().splitlines() if x]
    commands=json.loads((build/'compile_commands.json').read_text())
    sources={(Path(c['output']) if Path(c['output']).is_absolute() else Path(c['directory'])/c['output']).resolve():Path(c['file']).resolve() for c in commands}
    if len(objects)<len(old) or len(objects)>len(old)+16:raise ValueError('Unexpected runtime source inventory')
    for path in objects:
        if path.resolve() not in sources:raise ValueError('Unidentified runtime object')
        if 'generated' in sources[path.resolve()].parts:raise ValueError('Guest source is not public')
        header=path.read_bytes()[:20]
        if header[:6]!=b'\x7fELF\x02\x01' or header[18:20]!=b'\x3e\x00':raise ValueError('Not a PS5 x86-64 object')
    out.mkdir(parents=True,exist_ok=False)
    obsolete={x.removeprefix('{KIT}/') for _,x in old}
    for name,digest in recipe['files'].items():
        source=a.baseline/name
        if sha(source)!=digest:raise ValueError('Baseline input differs: '+name)
        if name in obsolete:continue
        destination=out/name;destination.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(source,destination)
    new=[];source_receipts=[]
    def validate_public(path):
        listing=subprocess.check_output([str(a.clang_bin/'llvm-nm.exe'),'--defined-only',str(path)],text=True,timeout=60)
        if re.search(r'__imp__sub_[0-9A-Fa-f]+|X86FuncMappings|X86Continuations',listing):
            raise ValueError('Generated guest definitions in public native input: '+path.name)
    for i,path in enumerate(objects):
        validate_public(path)
        name='native/inputs/runtime-%03d-%s'%(i,path.name)
        shutil.copyfile(path,out/name);new.append('{KIT}/'+name)
        source=sources[path.resolve()]
        source_receipts.append(dict(object=name,source=source.name,source_sha256=sha(source),object_sha256=sha(path)))
    recipe['link_argv']=arguments[:old[0][0]]+new+arguments[old[-1][0]+1:]
    archives=[Path(x) for x in (build/'link-archives.txt').read_text().splitlines() if x]
    for path in archives:
        if path.name=='libsims2-guest.a':continue
        matches=[x for x in recipe['files'] if x.endswith('-'+path.name)]
        if len(matches)!=1:raise ValueError('Cannot identify captured runtime archive: '+path.name)
        validate_public(path);shutil.copyfile(path,out/matches[0])
    recipe['files']={p.relative_to(out).as_posix():sha(p) for p in sorted((out/'native').rglob('*')) if p.is_file()}
    recipe['runtime_refresh']=dict(build_id=identity,baseline_recipe_sha256=sha(a.baseline/'native-recipe.json'),
        sources=source_receipts,captured_non_runtime_arguments_unchanged=True,guest_archive_included=False)
    (out/'native-recipe.json').write_text(json.dumps(recipe,indent=2)+'\n')
    print(json.dumps(dict(status='PASS_PUBLIC_RUNTIME_EXPORT',build_id=identity,objects=len(objects),output=str(out))))

if __name__=='__main__':main()
