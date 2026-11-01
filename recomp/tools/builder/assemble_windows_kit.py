# SPDX-License-Identifier: GPL-3.0-only
"""Maintainer-only: assemble a positive-inventory Windows owner-copy kit.

Public native export must come from export_native_recipe.py. Never copies the
private game's eboot, generated sources, guest archive, assets or saves.
"""
from pathlib import Path
import argparse
import hashlib
import json
import re
import shutil
import subprocess

ROOT=Path(__file__).resolve().parents[2]

def sha(p):
    with p.open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()

def refresh_native(native,kit):
    """Refresh a reviewed recipe without rebuilding its public libraries."""
    manifest=json.loads((kit/'kit.json').read_text())
    recipe=json.loads((native/'native-recipe.json').read_text())
    if not recipe.get('sdk_wrapper_evaluated'):raise ValueError('SDK wrapper not captured')
    for name,digest in recipe['files'].items():
        if manifest['files'].get(name,{}).get('sha256')==digest:continue
        source=native/name
        if sha(source)!=digest:raise ValueError('Native source digest mismatch')
        dest=kit/name;dest.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(source,dest)
        manifest['files'][name]=dict(bytes=dest.stat().st_size,sha256=digest)
    shutil.copyfile(native/'native-recipe.json',kit/'native-recipe.json')
    manifest['files']['native-recipe.json']=dict(bytes=(kit/'native-recipe.json').stat().st_size,sha256=sha(kit/'native-recipe.json'))
    manifest['link_argv']=['{KIT}/tools/ld.lld.exe',*recipe['link_argv']]
    (kit/'kit.json').write_text(json.dumps(manifest,indent=2)+'\n')


def assemble(a):
    kit=a.output.resolve();kit.mkdir(parents=True,exist_ok=False)
    def copy(src,name):
        dest=kit/name;dest.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(src,dest)
    def tree(src,name):shutil.copytree(src,kit/name)
    native=json.loads((a.native/'native-recipe.json').read_text())
    if not native.get('sdk_wrapper_evaluated'):raise ValueError('Capture the SDK wrapper before assembling a kit')
    for name,digest in native['files'].items():
        if sha(a.native/name)!=digest:raise ValueError('Corrupt native export: '+name)
        copy(a.native/name,name)
    copy(a.native/'native-recipe.json','native-recipe.json')
    # Defense against accidentally exporting generated guest archive members.
    bins=a.clang_bin
    for p in sorted((kit/'native/inputs').iterdir()):
        if p.suffix not in ('.o','.a'):continue
        nm=subprocess.run([str(bins/'llvm-nm.exe'),'--defined-only',str(p)],capture_output=True,check=True,text=True)
        if re.search(r'__imp__sub_[0-9A-Fa-f]+|X86FuncMappings|X86Continuations',nm.stdout):
            raise ValueError('Generated game definitions in PUBLIC input: '+p.name)
    # Bundle just the host tools and their non-system DLL dependencies.
    for name in ('clang++.exe','clang.exe','ld.lld.exe','llvm-ar.exe','llvm-nm.exe'):
        copy(bins/name,'tools/'+name)
    for name in ('iso_xbe.exe','xbe_recomp.exe','iso_install.exe'):
        copy(a.host_tools/name,'tools/'+name)
    copy(a.native_tool,'tools/ps5-native-tool.exe')
    # llvm-mingw's small clang.exe launches the real driver.
    if (bins/'clang-23.exe').is_file():copy(bins/'clang-23.exe','tools/clang-23.exe')
    pending=list((kit/'tools').iterdir());seen=set()
    while pending:
        file=pending.pop()
        if file.name.lower() in seen:continue
        seen.add(file.name.lower())
        listing=subprocess.run([str(bins/'llvm-objdump.exe'),'-p',str(file)],capture_output=True,check=True,text=True).stdout
        for name in re.findall(r'DLL Name:\s*(\S+)',listing):
            src=bins/name
            if src.is_file() and not (kit/'tools'/name).exists():
                copy(src,'tools/'+name);pending.append(kit/'tools'/name)
    resource=Path(subprocess.check_output([str(bins/'clang.exe'),'--print-resource-dir'],text=True).strip())
    tree(resource/'include','clang-resource/include')
    tree(a.sdk/'target/include','sdk/target/include')
    tree(ROOT/'runtime-sdk/runtime/cpu','runtime/cpu')
    tree(ROOT/'sdk/xdk','xdk')
    copy(ROOT/'config/game.toml','config/game.toml')
    for directory in ('sce_sys','sce_module','shaders','public-resources','licenses'):
        tree(a.title/directory,'title/'+directory)
    for name in ('settings.toml','CREDITS.txt','LICENSE','LICENSING.md','LEGAL.txt'):
        if (a.title/name).is_file():copy(a.title/name,'title/'+name)
    (kit/'title/README.txt').write_text('The Sims 2 owner-prepared PS5 title.\n'
        'Close the title before copying it to PS5. Preserve your existing hdd folder and saves.\n'
        'This private eboot was generated from your ISO. Keep it private.\n',encoding='utf-8')
    # Licenses/source are required for the public tools as well as title parts.
    copy(ROOT/'LICENSE','licenses/project-GPL-3.0.txt')
    copy(ROOT/'sdk/LICENSE','licenses/XboxXBERecomp-MIT.txt')
    copy(bins.parent/'LICENSE.TXT','licenses/LLVM-Apache-2.0-with-exception.txt')
    compile_argv=['{KIT}/tools/clang-23.exe','--driver-mode=g++','-target','x86_64-sie-ps5',
        '-isysroot','{KIT}/sdk','--sysroot={KIT}/sdk/target',
        '-stdlib++-isystem','{KIT}/sdk/target/include/c++/v1','-resource-dir','{KIT}/clang-resource',
        '-fno-stack-protector','-fno-plt','-femulated-tls','-frtti','-fexceptions',
        '-I{KIT}/runtime','-I{GENERATED}','--no-default-config','-O2','-g1','-DNDEBUG','-std=gnu++20',
        '-fPIC','-fno-strict-aliasing','-Wno-unused-label','-Wno-parentheses-equality','-Wno-unused-variable',
        '-march=x86-64-v2','-fno-omit-frame-pointer','-fdenormal-fp-math=ieee',
        '-ffunction-sections','-fdata-sections','-o','{OBJECT}','-c','{SOURCE}']
    stubs=list((kit/'native/stubs').glob('*.so'))
    allowed=set()
    for file in stubs:
        if file.name in ('libkernel_sys.so','libScePosixForWebKit.so'):continue
        listing=subprocess.run([str(bins/'llvm-nm.exe'),'-D','--defined-only',str(file)],
                               capture_output=True,text=True,check=True).stdout
        allowed.update(line.split()[-1].split('@')[0] for line in listing.splitlines() if line.split())
    manifest=dict(schema=1,kind='public_owner_builder',host='windows-x86_64',title_id='PPSA99145',
        xbe_sha1='16f9e41ce26c315a758f25d43ba1fa9d3809d1ad',
        runtime_build_id=(ROOT/'out/build/ps5-game-profile/build-id.txt').read_text().strip(),
        required_free_bytes=4<<30, compile_jobs=2, generated_cpp_count=len(list((ROOT/'generated/independent').glob('*.cpp'))),
        generated_files={p.name:sha(p) for p in sorted((ROOT/'generated/independent').iterdir()) if p.is_file()},
        extract_xbe_argv=['{KIT}/tools/iso_xbe.exe','{ISO}','{XBE}'],
        generate_argv=['{KIT}/tools/xbe_recomp.exe','{KIT}/config/game.toml','{XBE}','{GENERATED}','--xdk','{KIT}/xdk'],
        compile_argv=compile_argv,
        archive_argv=['{KIT}/tools/llvm-ar.exe','qc','{WORK}/libsims2-guest.a'],
        link_argv=['{KIT}/tools/ld.lld.exe',*native['link_argv']],
        undefined_argv=['{KIT}/tools/llvm-nm.exe','-D','--undefined-only','{WORK}/llvm-pie.elf'],
        allowed_imports=sorted(allowed),
        convert_argv=['{KIT}/tools/ps5-native-tool.exe','link','--in','{WORK}/llvm-pie.elf','--out','{WORK}/eboot.elf',
            '--stub-dir','{KIT}/native/stubs','--stub','{KIT}/native/stubs/libSceAgc.so',
            '--stub','{KIT}/native/stubs/libSceAgcDriver.so','--module-sdk','0x02000009',
            '--companion-sdk','0x08050001','--file-name','eboot.elf'],
        sign_argv=['{KIT}/tools/ps5-native-tool.exe','self','--sign','--in','{WORK}/eboot.elf',
                   '--out','{TITLE}/eboot.bin','--magic','0x1D3D154F'],
        inspect_argv=['{KIT}/tools/ps5-native-tool.exe','self','--inspect','--file','{TITLE}/eboot.bin'],
        install_argv=['{KIT}/tools/iso_install.exe','{ISO}','{TITLE}'])
    manifest['files']={p.relative_to(kit).as_posix():dict(bytes=p.stat().st_size,sha256=sha(p))
                       for p in sorted(kit.rglob('*')) if p.is_file()}
    (kit/'kit.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(dict(public_files=len(manifest['files']),host=manifest['host'],
                         bytes=sum(x['bytes'] for x in manifest['files'].values()))))

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    for key in ('native','clang-bin','sdk','host-tools','native-tool','title','output'):
        p.add_argument('--'+key,type=Path,required=True)
    assemble(p.parse_args())
