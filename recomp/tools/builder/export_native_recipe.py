#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Maintainer-only Linux export of the evaluated, original RADV link recipe.

Requires an already linked title's input directory. Excludes the private guest
archive. Does not change the established linker flags, ordering or platform ABI.
"""
import argparse
import hashlib
import json
from pathlib import Path
import shlex
import shutil
import subprocess

def capture_sdk_wrapper(argv,script,bash='bash'):
    """Evaluate the SDK wrapper too: it supplies PIE, 16K pages and TLS flags.

    Only replace compiler discovery and exec with an argv recorder. The SDK's
    argument loop and final command are evaluated verbatim by bash.
    """
    source=script.read_text()
    discovery='LLVM_BINDIR=$("${SCRIPT_DIR}/prospero-llvm-config" --bindir)'
    if source.count(discovery)!=1 or 'exec "${BIN_PATH}"' not in source:
        raise ValueError('Unrecognized SDK linker wrapper; review before export')
    body=source.replace(discovery,'LLVM_BINDIR=/captured-llvm')
    body=body.replace('SCRIPT_PATH="${BASH_SOURCE[0]}"','SCRIPT_PATH="$0"')
    code='exec() { printf \'%s\\0\' "$@"; }\n'+body
    # Sentinel tokens avoid MSYS command-line conversion of brace placeholders.
    arguments=[x.replace('{KIT}','__SIMS2_KIT__').replace('{WORK}','__SIMS2_WORK__') for x in argv]
    result=subprocess.run([str(bash),'-c',code,str(script),*arguments],capture_output=True,check=True)
    parts=result.stdout.decode().rstrip('\0').split('\0')
    if not parts or not parts[0].endswith('/ld.lld'):raise ValueError('SDK capture failed')
    return [x.replace('__SIMS2_KIT__','{KIT}').replace('__SIMS2_WORK__','{WORK}') for x in parts[1:]]


def finish_export(output,vulkan,sdk,bash='bash'):
    """Also usable on Windows to complete an already captured native export."""
    meta=json.loads((output/'native-recipe.json').read_text())
    if not meta.get('sdk_wrapper_evaluated'):
        script=sdk/'bin/prospero-lld'
        meta['link_argv']=capture_sdk_wrapper(meta['link_argv'],script,bash)
        meta['sdk_wrapper_evaluated']=True
        meta['sdk_wrapper_sha256']=hashlib.sha256(script.read_bytes()).hexdigest()
        shutil.copyfile(script,output/'native/prospero-lld.sh')
    # Preserve INCLUDE names: scripts are found through the recipe's -L path.
    pending=list((output/'native/inputs').glob('*.ld'));seen=set()
    import re
    while pending:
        file=pending.pop()
        if file.name in seen:continue
        seen.add(file.name)
        for name in re.findall(r'^\s*INCLUDE\s+["\']?([A-Za-z0-9_.-]+)',file.read_text(),re.M):
            src=vulkan/'tooling/native'/name
            if not src.is_file():raise ValueError('Missing included linker script: '+name)
            target=output/'native/inputs'/name
            shutil.copyfile(src,target);pending.append(target)
    # libc++ embeds dependent-library notes for pthread. The SDK represents
    # these system libraries by real, empty archives; functions come from its
    # module stubs/platform layer already in the recipe. Preserve the files
    # found through -L, rather than ignoring dependent-library diagnostics.
    for src in (sdk/'target/lib').glob('*.a'):
        if src.stat().st_size==8 and src.read_bytes()==b'!<arch>\n':
            shutil.copyfile(src,output/'native/stubs'/src.name)
    meta['files']={p.relative_to(output).as_posix():hashlib.sha256(p.read_bytes()).hexdigest()
                   for p in sorted((output/'native').rglob('*')) if p.is_file()}
    (output/'native-recipe.json').write_text(json.dumps(meta,indent=2)+'\n')
    return meta


def export(inputs, foundation, vulkan, sdk, output):
    for p in (inputs, foundation, vulkan, sdk):
        if not p.is_dir():
            raise ValueError('Missing input directory: '+str(p))
    output.mkdir(parents=True, exist_ok=False)
    script = foundation/'ps5/tools/link-title.sh'
    text = script.read_text()
    start = text.index('source "$vulkan/tools/radv-link.sh"')
    end = text.index('# A title whose localeconv', start)
    fragment = text[start:end]
    marker = '"$sdk/bin/prospero-lld" "${radv_linker_script[@]}"'
    if fragment.count(marker) != 1:
        raise ValueError('Unrecognized template recipe; review upstream changes')
    fragment = fragment.replace(marker, 'capture "${radv_linker_script[@]}"')
    capture = output/'evaluated-argv.json'
    command = 'set -euo pipefail\n'
    for k, v in dict(work=inputs, vulkan=vulkan, sdk=sdk,
                     native=vulkan/'tooling/native',
                     archive=vulkan/'.deps/native/radv-release/lib/libvulkan_radeon.ps5.a').items():
        command += k+'='+shlex.quote(str(v))+'\n'
    command += 'objects=("@'+str(inputs/'inputs.rsp')+'")\n'
    command += 'capture() { python3 - "$@" <<\'PY\'\nimport json,sys\n'
    command += 'json.dump(sys.argv[1:],open('+repr(str(capture))+',"w"))\nPY\n}\n'
    subprocess.run(['bash','-c',command+fragment], check=True, cwd=inputs)
    argv = json.loads(capture.read_text())
    expanded = []
    for arg in argv:
        expanded.extend(shlex.split((inputs/arg[1:]).read_text()) if arg.startswith('@') else [arg])
    mapping = {}
    def copy_file(path):
        path = path.resolve()
        if path.name.endswith('libsims2-guest.a'):
            return '{WORK}/libsims2-guest.a'
        if path not in mapping:
            name = 'native/inputs/%03d-%s' % (len(mapping),path.name)
            dest = output/name
            dest.parent.mkdir(parents=True,exist_ok=True)
            shutil.copyfile(path,dest)
            mapping[path] = '{KIT}/'+name
        return mapping[path]
    relocated = []
    directories = {str(sdk/'target/lib'):'{KIT}/native/stubs',
                   str(vulkan/'tooling/native'):'{KIT}/native/inputs'}
    for index, arg in enumerate(expanded):
        if index and expanded[index-1] == '-o':
            relocated.append('{WORK}/llvm-pie.elf'); continue
        if arg in directories:
            relocated.append(directories[arg]); continue
        path = Path(arg)
        was_absolute = path.is_absolute()
        if not arg.startswith('-') and not path.is_absolute(): path = inputs/path
        if path.is_file():
            relocated.append(copy_file(path))
        elif was_absolute:
            raise ValueError('Unresolved recipe input: '+arg)
        else:
            relocated.append(arg)
    # Converter resolves all SDK library NIDs through the exact same stubs.
    stubs = output/'native/stubs'; stubs.mkdir(parents=True,exist_ok=True)
    for src in sorted((sdk/'target/lib').glob('*.so')): shutil.copyfile(src,stubs/src.name)
    for src in sorted((inputs/'link/stubs').glob('*.so')): shutil.copyfile(src,stubs/src.name)
    meta = {'schema':1, 'link_argv':relocated,
            'origin':'PS5_VulkanTemplate link-title.sh + PS5_Vulkan radv-link.sh, evaluated unchanged',
            'recipe_sha256':hashlib.sha256(script.read_bytes()).hexdigest(),
            'radv_recipe_sha256':hashlib.sha256((vulkan/'tools/radv-link.sh').read_bytes()).hexdigest(),
            'files':{p.relative_to(output).as_posix():hashlib.sha256(p.read_bytes()).hexdigest()
                     for p in sorted(output.rglob('*')) if p.is_file() and p != capture}}
    capture.unlink() # It contains maintainer paths; never goes in the portable kit.
    (output/'native-recipe.json').write_text(json.dumps(meta,indent=2)+'\n')
    finish_export(output,vulkan,sdk)
    print(json.dumps({'exported_public_files':len(meta['files']),'arguments':len(relocated)}))


if __name__ == '__main__':
    p=argparse.ArgumentParser(description=__doc__)
    for key in ('inputs','foundation','vulkan','sdk','output'):p.add_argument('--'+key,required=True,type=Path)
    a=p.parse_args()
    export(*(getattr(a,k).resolve() for k in ('inputs','foundation','vulkan','sdk','output')))
