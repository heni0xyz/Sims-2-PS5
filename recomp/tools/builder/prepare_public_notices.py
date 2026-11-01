# SPDX-License-Identifier: GPL-3.0-only
"""Correct title license scope and stage dependency notices in a public kit.

Does not change executable tools, linked inputs, generated code or settings.
The original template's MIT statement applies to its original files, not to
the authored GPLv3 application. Source locations must work outside the build PC.
"""
from pathlib import Path
import argparse
import hashlib
import importlib.metadata
import json
import shutil
import sys

ROOT=Path(__file__).resolve().parents[2]

def prepare(kit):
    manifest=json.loads((kit/'kit.json').read_text())
    def put(source,name):
        dest=kit/name;dest.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(source,dest)
    def text(name,value):
        dest=kit/name;dest.parent.mkdir(parents=True,exist_ok=True);dest.write_text(value,encoding='utf-8')
    components=kit/'title/licenses/components.json'
    records=json.loads(components.read_text())
    def clean(value):
        if isinstance(value,dict):
            return {k:clean(v) for k,v in value.items() if k!='path' and not (
                k=='remote' and isinstance(v,str) and not v.startswith(('https://','http://')))}
        if isinstance(value,list):return [clean(v) for v in value]
        return value
    records=clean(records)
    for record in records:
        if record.get('id')=='title':
            record['upstream_licence_scope']=record['licence']
            record['name']='The Sims 2 authored application and original PS5 Vulkan foundation/libraries'
            record['licence']='GPL-3.0-only (authored application); original foundation/library notices retained'
            record['copyright']+=['Copyright (C) 2026 OpenAI Codex, for szampan']
            record['source']['directions']='SOURCES.txt; authored sources in recomp/ and PS5_Sims2Bootstrap/'
    text('title/licenses/components.json',json.dumps(records,indent=2)+'\n')
    notice=('THE SIMS 2 — LICENSE AND SOURCE SCOPE\n\n'
        'Authored application, builder and new patches: GPL-3.0-only.\n'
        'Original foundation and dependency files retain their own copyright/licenses.\n'
        'The combined authored title is supplied under GPLv3; upstream MIT/Apache/OFL/CC0\n'
        'notices are not replaced. Original Xbox game code/data are not licensed by us.\n'
        'The public instruction title and builder kit contain no translated game functions.\n'
        'The complete game executable/data are generated privately from the owner ISO.\n\n'
        'Authored source: the accompanying Sims-2-PS5 source archive/repository.\n'
        'Exact native SDK additions are in vendor/PS5_PayloadSDK.\n'
        'Other exact dependency source directions are in SOURCES.txt and source SOURCES.md.\n'
        'Detailed dependency licenses: licenses/. Credits: CREDITS.txt.\n')
    text('title/LEGAL.txt',notice)
    directions=('Source repository: https://github.com/heni0xyz/Sims-2-PS5\n'
        'Keep the accompanying source ZIP and source directions beside binary downloads.\n'
        'Local Xbox SDK sources and native SDK additions are included in that source ZIP.\n'
        'PS5_Vulkan/native tool: https://github.com/mihawk-99/PS5_Vulkan/tree/f410e67a0241b3e8edcdbcd2a9b285dfc4a412ed\n'
        'Mesa/RADV: https://github.com/mihawk-99/PS5_Mesa/tree/1ba9874f478657ccf6da72ce55f0260164cc0bd0\n'
        'glslang: https://github.com/KhronosGroup/glslang/tree/1062752a891c95b2bfeed9e356562d88f9df84ac\n'
        'SPIR-V source pins: glslang known_good.json and update_glslang_sources.py at that revision.\n'
        'Windows compiler: https://github.com/mstorsjo/llvm-mingw/releases/tag/20261020\n'
        'zlib1.3.2: https://zlib.net/zlib-1.3.2.tar.gz\n'
        'Zydis: https://github.com/zyantific/zydis/tree/a2278f1d254e492f6a6b39f6cb5d1f5d515659dc\n'
        'toml++: https://github.com/marzer/tomlplusplus/tree/30172438cee64926dc41fdd9c11fb3ba5b2ba9de\n')
    text('title/SOURCES.txt',directions);text('SOURCES.txt',directions)
    put(ROOT/'CREDITS.txt','title/CREDITS.txt')
    dependencies={
        'Zydis-MIT.txt':ROOT/'out/host/_deps/zydis-src/LICENSE',
        'Zycore-MIT.txt':ROOT/'out/host/_deps/zydis-src/dependencies/zycore/LICENSE',
        'tomlplusplus-MIT.txt':ROOT/'out/host/_deps/tomlplusplus-src/LICENSE',
        'zlib.txt':ROOT/'out/builder-native-tools/windows-x86_64/zlib-1.3.2/LICENSE',
        'CPython.txt':Path(sys.prefix)/'LICENSE.txt',
    }
    pylicense=next(x.locate() for x in importlib.metadata.files('pyinstaller') if str(x).endswith('licenses/COPYING.txt'))
    dependencies['PyInstaller-with-bootloader-exception.txt']=pylicense
    for name,source in dependencies.items():
        if not source.is_file():raise FileNotFoundError(source)
        put(source,'licenses/'+name)
    for directory in (Path(sys.prefix)/'tcl').glob('*'):
        if directory.is_dir():
            for source in directory.glob('license*'):
                if source.is_file():put(source,'licenses/'+directory.name+'-'+source.name)
    manifest['files']={p.relative_to(kit).as_posix():dict(bytes=p.stat().st_size,
        sha256=hashlib.file_digest(p.open('rb'),'sha256').hexdigest())
        for p in sorted(kit.rglob('*')) if p.is_file() and p.name!='kit.json'}
    (kit/'kit.json').write_text(json.dumps(manifest,indent=2)+'\n')
    print(json.dumps(dict(status='PUBLIC_NOTICES_PREPARED',files=len(manifest['files']))))

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--kit',required=True,type=Path)
    prepare(p.parse_args().kit.resolve())
