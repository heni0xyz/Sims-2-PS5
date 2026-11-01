# SPDX-License-Identifier: GPL-3.0-only
# OpenAI Codex for szampan.
"""Create an isolated, hashed settings-only GPU completion comparison.

The executable and symbols remain byte-identical. This does not build new code
or establish a performance improvement. Original title/defaults stay untouched.
"""
from pathlib import Path, PurePosixPath
import argparse, copy, hashlib, json, shutil, tomllib, zipfile

def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()

def prepare(base, output):
    manifest=json.loads((base/'build-manifest.json').read_text())
    title=manifest['title_id']
    if title!='PPSA99145':raise ValueError('Unexpected title')
    if output.exists():raise ValueError('Choose a fresh output; existing files are never overwritten')
    for name,record in manifest['files'].items():
        path=PurePosixPath(name)
        if path.is_absolute() or '..' in path.parts or '\\' in name or ':' in name:
            raise ValueError('Unsafe member path')
        member=base/title/name
        if member.is_symlink() or member.stat().st_size!=record['bytes'] or digest(member)!=record['sha256']:
            raise ValueError('Baseline member differs: '+name)
    if digest(base/'llvm-pie.elf')!=manifest['debug_elf_sha256']:
        raise ValueError('Baseline symbols differ')
    original=(base/title/'settings.toml').read_bytes()
    settings=tomllib.loads(original.decode())
    if settings.get('gpu_callbacks_async') is not False:raise ValueError('Baseline must be synchronous')
    if settings.get('fps_cap')!=60 or not settings.get('native_render_resolution'):
        raise ValueError('Expected native-resolution 60-cap baseline')
    before=b'gpu_callbacks_async=false'
    if original.count(before)!=1:raise ValueError('Expected one exact callback policy')
    changed=original.replace(before,b'gpu_callbacks_async=true')
    expected=dict(settings,gpu_callbacks_async=True)
    if tomllib.loads(changed.decode())!=expected:raise ValueError('Unexpected settings change')
    output.mkdir(parents=True)
    for name in manifest['files']:
        target=output/title/name;target.parent.mkdir(parents=True,exist_ok=True)
        shutil.copyfile(base/title/name,target)
    (output/title/'settings.toml').write_bytes(changed)
    (output/'rollback-settings.toml').write_bytes(original)
    shutil.copyfile(base/'llvm-pie.elf',output/'llvm-pie.elf')
    candidate=copy.deepcopy(manifest)
    candidate['configuration_variant']='ordered-write-completion-experiment'
    candidate['baseline_settings_sha256']=hashlib.sha256(original).hexdigest()
    candidate['files']['settings.toml']={'sha256':hashlib.sha256(changed).hexdigest(),'bytes':len(changed)}
    for name,record in candidate['files'].items():
        if digest(output/title/name)!=record['sha256']:raise ValueError('Candidate member differs')
    (output/'build-manifest.json').write_text(json.dumps(candidate,indent=2)+'\n')
    status={'state':'BUILD_PASS_RUNTIME_NOT_TESTED','build_id':candidate['build_id'],
            'configuration_only':True,'configuration_variant':candidate['configuration_variant'],
            'PS5_runtime':'NOT_TESTED','performance':'NOT_TESTED'}
    (output/'status.json').write_text(json.dumps(status,indent=2)+'\n')
    with zipfile.ZipFile(output/f'Sims2Independent-{title}.zip','w',zipfile.ZIP_DEFLATED,compresslevel=1) as archive:
        for name in candidate['files']:archive.write(output/title/name,f'{title}/{name}')
    record={'state':'CONFIGURATION_EXPERIMENT_READY_NOT_MEASURED','build_id':candidate['build_id'],
            'variant':candidate['configuration_variant'],'changed_title_files':['settings.toml'],
            'eboot_byte_identical':digest(output/title/'eboot.bin')==manifest['eboot_sha256'],
            'settings_sha256':candidate['files']['settings.toml']['sha256'],
            'baseline_settings_sha256':candidate['baseline_settings_sha256'],
            'PS5_runtime':'NOT_TESTED','stable_60fps':'NOT_ESTABLISHED'}
    (output/'experiment.json').write_text(json.dumps(record,indent=2)+'\n')
    return record

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--base',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
    a=p.parse_args();print(json.dumps(prepare(a.base.resolve(),a.output.absolute()),indent=2))
