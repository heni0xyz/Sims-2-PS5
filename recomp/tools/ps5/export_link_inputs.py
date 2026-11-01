#!/usr/bin/env python3
"""Move already compiled PS5 ELF inputs to the Linux packaging host, privately.

This does not link or create a PS5 title. The receiving host still uses the
unmodified template link-title.sh and PS5_Vulkan/tools/radv-link.sh.
"""
from __future__ import annotations
import argparse
import hashlib
import io
import json
import tarfile
from pathlib import Path

def export(build: Path, output: Path):
    build=build.resolve()
    objects=[Path(p) for p in (build/'runtime-objects.txt').read_text().splitlines() if p]
    archives=[Path(p) for p in (build/'link-archives.txt').read_text().splitlines() if p]
    if not objects or len(archives)<8:
        raise ValueError('incomplete CMake link input inventory')
    manifest={'schema':1,'build_id':(build/'build-id.txt').read_text().strip(),
              'kind':'private_game_derived_intermediates','files':{}}
    profile=build/'build-profile.json'
    if profile.exists():
        record=json.loads(profile.read_text(encoding='utf-8-sig'))
        if record.get('build_id')!=manifest['build_id']:
            raise ValueError('profile metadata does not match the compiled inputs')
        manifest['build_profile']=record
    credits=Path(__file__).resolve().parents[2]/'CREDITS.txt'
    if not credits.is_file() or not credits.stat().st_size:
        raise ValueError('missing project patch credits')
    names=[]
    # Branding is built deterministically on the Windows authoring host. The
    # Linux linker needs no image tool/dependency and receives only shell assets.
    from prepare_branding import prepare
    branding=build/'branding';brand=prepare(branding)
    root=Path(__file__).resolve().parents[2]
    from build_hd_prompts import build as build_prompts
    public=build/'public-resources';build_prompts(public)
    from build_installer_font import build as build_font
    build_font(public)
    output.parent.mkdir(parents=True,exist_ok=True)
    with tarfile.open(output,'w:gz') as bundle:
        for index,path in enumerate(objects+archives):
            header=path.open('rb').read(20)
            if index<len(objects):
                if header[:6]!=b'\x7fELF\x02\x01' or header[18:20]!=b'\x3e\x00':
                    raise ValueError('not a PS5 ELF64 object: '+path.name)
            elif header[:8]!=b'!<arch>\n':
                raise ValueError('not a static archive: '+path.name)
            name=f'inputs/{index:03d}-{path.name}'
            names.append(name)
            manifest['files'][name]={'sha256':hashlib.file_digest(path.open('rb'),'sha256').hexdigest(),
                                     'bytes':path.stat().st_size}
            bundle.add(path,arcname=name,recursive=False)
        # Attribution travels with the private bundle, covered by its hash
        # inventory, but is never passed to the native linker as an object.
        manifest['files']['CREDITS.txt']={'sha256':hashlib.file_digest(credits.open('rb'),'sha256').hexdigest(),
                                          'bytes':credits.stat().st_size}
        bundle.add(credits,arcname='CREDITS.txt',recursive=False)
        for filename in [*brand['files'],'branding-manifest.json']:
            path=branding/filename;name='branding/'+filename
            manifest['files'][name]={'sha256':hashlib.file_digest(path.open('rb'),'sha256').hexdigest(),'bytes':path.stat().st_size}
            bundle.add(path,arcname=name,recursive=False)
        public_files={
            'public-resources/dualsense.s2tex':public/'dualsense.s2tex',
            'public-resources/dualsense-manifest.json':public/'manifest.json',
            'LICENSE':root/'LICENSE',
            'LICENSING.md':root/'LICENSING.md',
            'licenses/artwork/CC0-1.0.txt':root/'artwork/CC0-1.0.txt',
            'licenses/xbox-runtime/LICENSE.txt':root/'runtime-sdk/LICENSE',
            'licenses/xbox-runtime/LOCAL_CHANGES_LICENSE.md':root/'runtime-sdk/LOCAL_CHANGES_LICENSE.md',
            'public-resources/installer-font.s2font':public/'installer-font.s2font',
            'public-resources/installer-font-manifest.json':public/'installer-font-manifest.json',
            'public-resources/OpenSans-Regular.ttf':root/'third_party/opensans/OpenSans-Regular.ttf',
            'licenses/opensans/OFL.txt':root/'third_party/opensans/OFL.txt',
            'licenses/opensans/NOTICE.txt':root/'third_party/opensans/README.md',
        }
        for name,path in public_files.items():
            manifest['files'][name]={'sha256':hashlib.file_digest(path.open('rb'),'sha256').hexdigest(),'bytes':path.stat().st_size}
            bundle.add(path,arcname=name,recursive=False)
        arguments=names[:len(objects)]+['--start-group']+names[len(objects):]+['--end-group']
        payloads={'inputs.rsp':'\n'.join(arguments)+'\n','samples.txt':'sims2independent\n',
                  'link-input-manifest.json':json.dumps(manifest,indent=2)+'\n'}
        for name,text in payloads.items():
            data=text.encode();entry=tarfile.TarInfo(name);entry.size=len(data);entry.mode=0o644
            bundle.addfile(entry,io.BytesIO(data))
    return manifest

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('build',type=Path);parser.add_argument('output',type=Path)
    args=parser.parse_args()
    result=export(args.build,args.output)
    print(json.dumps({'build_id':result['build_id'],'files':len(result['files']),
                      'bundle_bytes':args.output.stat().st_size}))
