# SPDX-License-Identifier: GPL-3.0-only
"""Build the existing public PS5 native packaging tool for this host.

Uses PS5_Vulkan's four-source/zlib recipe; no title linking or deployment.
"""
from pathlib import Path, PurePosixPath
import argparse, hashlib, json, os, subprocess, tarfile

EXPECTED='bb329a0a2cd0274d05519d61c667c062e06990d72e125ee2dfa8de64f0119d16'

def run(argv,log,env):
    with log.open('wb') as output:
        result=subprocess.run([str(x) for x in argv],stdout=output,stderr=subprocess.STDOUT,env=env)
    if result.returncode:
        raise RuntimeError(log.read_text(errors='replace')[-8000:])

def main():
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('vulkan','zlib','clang','cmake','ninja','output'):p.add_argument('--'+name,type=Path,required=True)
    a=p.parse_args();out=a.output.resolve();out.mkdir(parents=True,exist_ok=True)
    if hashlib.sha256(a.zlib.read_bytes()).hexdigest()!=EXPECTED:raise ValueError('zlib source fingerprint mismatch')
    source=out/'zlib-1.3.2'
    if not (source/'CMakeLists.txt').is_file():
        # Explicit regular-file extraction also works in restricted Windows
        # hosts where realpath(ALLOW_MISSING) is refused by the sandbox.
        with tarfile.open(a.zlib) as archive:
            for member in archive:
                name=PurePosixPath(member.name)
                if name.is_absolute() or '..' in name.parts or name.parts[0]!='zlib-1.3.2':
                    raise ValueError('Unexpected public source archive path')
                if member.isdir():continue
                if not member.isfile():raise ValueError('Linked/special source member refused')
                target=out.joinpath(*name.parts)
                target.parent.mkdir(parents=True,exist_ok=True)
                target.write_bytes(archive.extractfile(member).read())
    env=dict(os.environ,PATH=str(a.clang.resolve().parent)+os.pathsep+os.environ.get('PATH',''))
    cc=a.clang.resolve().with_name('clang.exe' if os.name=='nt' else 'clang')
    run([a.cmake,'-S',source,'-B',out/'zlib-build','-G','Ninja',
         '-DCMAKE_MAKE_PROGRAM='+str(a.ninja.resolve()),'-DCMAKE_C_COMPILER='+str(cc),
         '-DCMAKE_BUILD_TYPE=Release','-DZLIB_BUILD_EXAMPLES=OFF','-DZLIB_BUILD_TESTING=OFF'],out/'configure.log',env)
    run([a.cmake,'--build',out/'zlib-build','--target','zlibstatic','--parallel','2'],out/'zlib-build.log',env)
    libraries=list((out/'zlib-build').rglob('libz*.a'))
    libraries=[x for x in libraries if '.dll.' not in x.name]
    if len(libraries)!=1:raise ValueError('Expected one static zlib archive')
    native=a.vulkan/'tooling/native';exe=out/('ps5-native-tool.exe' if os.name=='nt' else 'ps5-native-tool')
    files=[native/x for x in ('native_app_builder.cpp','self_container.cpp','elf_object.cpp','sce_module_writer.cpp')]
    run([a.clang,'-std=c++20','-O2','-Wall','-Wextra','-Werror','-I'+str(source),
         '-I'+str(out/'zlib-build'),*files,libraries[0],'-o',exe],out/'native-tool-build.log',env)
    report={'status':'HOST_TOOL_BUILT_NOT_CONSOLE_TESTED','tool_sha256':hashlib.sha256(exe.read_bytes()).hexdigest(),
            'sources':{x.name:hashlib.sha256(x.read_bytes()).hexdigest() for x in files},
            'zlib_source_sha256':EXPECTED,'scope':'Original public packaging implementation; native host build only'}
    (out/'build-manifest.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))

if __name__=='__main__':main()
