# Copyright (C) 2026 OpenAI Codex, for szampan
# SPDX-License-Identifier: GPL-3.0-only
"""Prepare public native-title resources and a verified private object inventory.

No private object is copied into the diagnostic. The console reuses its own
previously generated objects, verifying each against the captured digest.
"""
import hashlib
import json
from pathlib import Path
import shutil
from analyze_linker_probe import analyze
from builder.core import BuildError, sha, verify_kit


def source_inputs(root):
    names = ['native_packager.cpp', 'native_packager.h', 'console_native_title_steps.inc']
    for folder in ('ps5_native_tool', 'zlib_headers'):
        location = root/'compiler/third_party'/folder
        origin = json.loads((location/'ORIGIN.json').read_text())
        names.append('third_party/'+folder+'/ORIGIN.json')
        for original, item in origin['files'].items():
            name = Path(original).name
            if sha(location/name) != item['sha256']:
                raise BuildError('Vendored public native source changed: '+name)
            names.append('third_party/'+folder+'/'+name)
    return names


def prepare(root, evidence, kit, output):
    recorded = json.loads((evidence/'run-summary.json').read_text())
    if recorded.get('state') != 'PASS' or not recorded.get('console_idle_after'):
        raise BuildError('Native packaging requires a finished passing console compilation')
    capture = analyze((evidence/'klog.log').read_text(encoding='utf-8'), recorded['build_id'],
                      True, True, True, 65)
    if capture['status'] != 'PASS' or len(capture['runs']) != 1:
        raise BuildError('Private source compilation is not proven complete')
    run = capture['runs'][0]
    if run['run_id'] != recorded['run_id']:
        raise BuildError('Private evidence run differs')
    names = ['x86_func_mapping.o']+['x86_recomp.'+str(n)+'.o' for n in range(64)]
    objects = []
    for name in names:
        digest = run['compiled_objects'].get(name)
        if digest != sha(evidence/'compiler-run'/name):
            raise BuildError('Fetched private object differs: '+name)
        objects.append(dict(name=name, sha256=digest))
    manifest = verify_kit(kit)
    target = output/'native-title-kit'
    shutil.copytree(kit/'native', target/'native', dirs_exist_ok=True)
    public = [dict(name=name, sha256=item['sha256'], bytes=item['bytes']) for name, item in sorted(manifest['files'].items())
              if name.startswith('native/')]
    if len(public) != len([p for p in target.rglob('*') if p.is_file()]):
        raise BuildError('Unexpected native resource inventory')
    arguments = manifest['link_argv']
    # Preserve actual conversion order, including explicit stubs followed by
    # the sorted stub directory, exactly as the existing public tool does.
    conversion = manifest['convert_argv']
    stubs = [conversion[i+1].replace('{KIT}/', '') for i, value in enumerate(conversion) if value == '--stub']
    stubs += [p.relative_to(kit).as_posix() for p in sorted((kit/'native/stubs').glob('*.so'))]
    def array(name, values):
        return 'static constexpr const char *'+name+'[] = {\n'+''.join('  '+json.dumps(value)+',\n' for value in values)+'};\n'
    header = 'struct Sims2NativeFile { const char *name; const char *sha256; };\n'
    for name, items in [('sims2NativePublicInputs', public), ('sims2NativeObjects', objects)]:
        header += 'static constexpr Sims2NativeFile '+name+'[] = {\n'+''.join(
            '  {"%s", "%s"},\n' % (item['name'], item['sha256']) for item in items)+'};\n'
    header += array('sims2NativeLinkArguments', arguments)
    header += array('sims2NativeStubFiles', stubs)
    header += array('sims2NativeAllowedImports', manifest['allowed_imports'])
    header += 'static constexpr const char *sims2NativeObjectDirectory = '+json.dumps(
        '/data/sims2-compiler-probe/'+recorded['run_id'])+';\n'
    return header, dict(source_build=recorded['build_id'], source_run=recorded['run_id'],
                        objects=objects, public_inputs=public)
