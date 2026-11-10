# SPDX-License-Identifier: GPL-3.0-only
"""Export source only. No network, Git writes, title binaries or publication."""
from pathlib import Path, PurePosixPath
import argparse, hashlib, importlib.util, json, os, re, stat, zipfile

ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('source_policy', ROOT/'recomp/tools/prepare_source_release.py')
policy=importlib.util.module_from_spec(spec);spec.loader.exec_module(policy)
SKIP={'.git','out','private','generated','hdd','saves','__pycache__','dist','.pytest_cache'}
TEXT={'.md','.txt','.json','.toml','.py','.cpp','.h','.c','.ps1','.sh','.cmake','.yml','.yaml'}
SECRET=re.compile(rb'-----BEGIN (?:RSA |OPENSSH |EC )?PRIVATE KEY-----|gh[pousr]_[A-Za-z0-9]{30,}')
PERSONAL=re.compile(rb'C:[/\\]+Users[/\\]+(?!Public[/\\]|Default[/\\])[^/\\\s]+[/\\]')

def inspect(name,data):
    policy_name=name
    if name.startswith('vendor/PS5_PayloadSDK/include/freebsd/private/'):
        policy_name=name.replace('/private/','/sdk-internal/',1)
    policy.inspect(policy_name,data)
    p=PurePosixPath(name)
    if p.suffix.lower() in {'.png','.jpg','.jpeg','.dds','.pxz'}:
        allowed=(name.startswith(('recomp/artwork/source/','recomp/artwork/installer/','recomp/mods/dualsense/kenney/',
                                  'PS5_Sims2Independent/external/glm/doc/','PS5_Sims2Installer/external/glm/doc/'))
                 or name in {'PS5_Sims2Independent/ps5/sce_sys/icon0.png','PS5_Sims2Bootstrap/ps5/sce_sys/icon0.png',
                             'PS5_Sims2Installer/ps5/sce_sys/icon0.png',
                             'vendor/PS5_PayloadSDK/samples/install_app/FAKE02932/icon0.png',
                             'misc/sims2.png','misc/sims2.pxz','misc/sims2bg.png','misc/sims2bg.pxz'})
        if not allowed:raise ValueError('Unreviewed image path: '+name)
    if p.suffix.lower() in TEXT or p.name in {'LICENSE','CMakeLists.txt','.gitignore'}:
        if SECRET.search(data):raise ValueError('Credential content: '+name)
        if PERSONAL.search(data):raise ValueError('Private development details: '+name)

def collect(root):
    files={};folded=set();issues=[]
    def walk(directory):
        for path in sorted(directory.iterdir()):
            sdk_internal=path.relative_to(root).as_posix()=='vendor/PS5_PayloadSDK/include/freebsd/private'
            if path.name in SKIP and not sdk_internal:continue
            if path.is_symlink():raise ValueError('Source symlink refused')
            if path.is_dir():yield from walk(path)
            elif path.is_file():yield path
            else:raise ValueError('Special source file refused')
    for path in walk(root):
        name=path.relative_to(root).as_posix(); data=path.read_bytes()
        if name.casefold() in folded:raise ValueError('Case-colliding source path: '+name)
        folded.add(name.casefold())
        try:inspect(name,data)
        except ValueError as error:issues.append(str(error));continue
        files[name]=data
    if issues:raise ValueError('Source audit rejected '+str(len(issues))+' members:\n'+'\n'.join(issues[:30]))
    required={'LICENSE','README.md','recomp/LICENSE','recomp/CREDITS.txt',
              'recomp/sdk/LICENSE','recomp/runtime-sdk/LICENSE',
              'recomp/runtime-sdk/LOCAL_CHANGES_LICENSE.md','PS5_Sims2Independent/LICENSE.md',
              'PS5_Sims2Installer/LICENSE.md','recomp/compiler/installer_app.cpp',
              'recomp/compiler/native_packager.cpp','recomp/compiler/executable_file.h'}
    if not required<=files.keys():raise ValueError('Missing source component/license')
    return files

def export(root,output,version='1.1.0'):
    # Normalize dot components even when Windows realpath cannot resolve a
    # not-yet-created output directory in a restricted host environment.
    root=Path(os.path.abspath(root)).resolve()
    output=Path(os.path.abspath(output)).resolve()
    if output==root or root in output.parents:
        raise ValueError('Output must be outside the source tree')
    files=collect(root);output.mkdir(parents=True,exist_ok=False)
    if not re.fullmatch(r'[0-9]+\.[0-9]+\.[0-9]+(?:-[A-Za-z0-9.-]+)?',version):
        raise ValueError('Invalid source version')
    destination=output/('Sims-2-PS5-source-'+version+'.zip')
    inventory={}
    components=json.loads((root/'source-component-manifest.json').read_text())['components']
    source_modes={component['archive_prefix']+'/'+name:record.get('mode',0o644)
                  for component in components.values() for name,record in component['files'].items()}
    with zipfile.ZipFile(destination,'x',compression=zipfile.ZIP_DEFLATED,compresslevel=9) as archive:
        for name,data in files.items():
            info=zipfile.ZipInfo('Sims-2-PS5/'+name,date_time=(2026,11,10,0,0,0))
            mode=source_modes.get(name,0o755 if name.endswith('.sh') or data.startswith(b'#!') else 0o644)
            info.create_system=3;info.external_attr=(stat.S_IFREG|mode)<<16
            archive.writestr(info,data,compress_type=zipfile.ZIP_DEFLATED,compresslevel=9)
            inventory[name]={'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest()}
    with zipfile.ZipFile(destination) as archive:
        if archive.testzip() is not None:raise ValueError('ZIP CRC failure')
        if len(archive.namelist())!=len(set(archive.namelist())):raise ValueError('Duplicate ZIP member')
        for name,record in inventory.items():
            data=archive.read('Sims-2-PS5/'+name);inspect(name,data)
            if len(data)!=record['bytes'] or hashlib.sha256(data).hexdigest()!=record['sha256']:
                raise ValueError('Source readback mismatch: '+name)
    report={'schema':1,'status':'PASS_SOURCE_ONLY_INVENTORY','files':inventory,
            'archive':{'name':destination.name,'bytes':destination.stat().st_size,
                       'sha256':hashlib.sha256(destination.read_bytes()).hexdigest()},
            'publication':'NONE','scope':'Technical source inventory; no native binary clearance or console validation'}
    (output/'source-manifest.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    return {'status':report['status'],'files':len(files),'archive':report['archive']}

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True)
    p.add_argument('--version',default='1.1.0')
    a=p.parse_args();print(json.dumps(export(ROOT,a.output,a.version),indent=2))
