"""Make a fresh owner-only PS5 test folder from verified native files and a dump.

OpenAI Codex for szampan. Never publishes, deploys, or copies any save files.
"""
from pathlib import Path
import argparse,copy,hashlib,json,os,shutil,sys,tomllib
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from check_assets import validate,verify_inventory,member

def digest(path):
    with path.open('rb') as stream:return hashlib.file_digest(stream,'sha256').hexdigest()

def main():
    p=argparse.ArgumentParser();p.add_argument('--native',type=Path,required=True)
    p.add_argument('--assets',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
    p.add_argument('--dualsense',action='store_true',help='Apply the authored Kenney filled-white prompt patch to the new copy only')
    p.add_argument('--playstation-text',action='store_true',help='Patch English checking/loading/saving console notices in the new QuickData copy')
    a=p.parse_args()
    # Normalize '..' lexically before resolving: Windows sandbox parent access
    # restrictions can leave those components in Path.resolve(strict=False).
    native=Path(os.path.abspath(a.native)).resolve()
    assets=Path(os.path.abspath(a.assets)).resolve()
    out=Path(os.path.abspath(a.output)).resolve()
    if out.exists():raise ValueError('Choose a fresh output directory; existing data is never overwritten')
    manifest=json.loads((native/'build-manifest.json').read_text())
    verified=json.loads((native/'local-validation.json').read_text())
    if verified['status']!='PASS_LOCAL_DELIVERY' or verified['build_id']!=manifest['build_id']:
        raise ValueError('Validate the native delivery first')
    title=manifest['title_id']
    if title!='PPSA99145':raise ValueError('Not the independent test title')
    owner=json.loads((assets/'owner-asset-manifest.json').read_text())
    validate(assets);source_check=verify_inventory(assets,owner)
    for name,record in manifest['files'].items():
        source=(native/title/name).resolve();source.relative_to(native/title)
        if source.is_symlink() or source.stat().st_size!=record['bytes'] or digest(source)!=record['sha256']:
            raise ValueError('Native source differs from validated delivery: '+name)
    out.mkdir(parents=True);package=out/title;package.mkdir()
    for name in manifest['files']:
        target=package/name;target.parent.mkdir(parents=True,exist_ok=True)
        shutil.copyfile(native/title/name,target)
    game=package/'assets';game.mkdir()
    patched_owner=copy.deepcopy(owner);prompt_report=None;text_report=None
    for name in owner['files']:
        if a.dualsense and name.casefold()=='data/textures.arc':continue
        if a.playstation_text and name.casefold()=='data/quickdat.arc':continue
        source=member(assets,name)
        source.resolve().relative_to(assets)
        if source.is_symlink():raise ValueError('Owner asset links are not accepted')
        target=game/name;target.parent.mkdir(parents=True,exist_ok=True)
        shutil.copyfile(source,target)
    if a.dualsense:
        from patch_prompts import apply
        name=next(name for name in owner['files'] if name.casefold()=='data/textures.arc')
        prompt_report=apply(member(assets,name),game/name,Path(__file__).resolve().parents[2]/'mods/dualsense')
        patched_owner['files'][name]['sha256']=prompt_report['output_sha256']
        patched_owner['files'][name]['bytes']=prompt_report['size']
        (out/'prompt-patch-report.json').write_text(json.dumps(prompt_report,indent=2)+'\n')
    if a.playstation_text:
        from patch_ui_text import apply as patch_text
        name=next(name for name in owner['files'] if name.casefold()=='data/quickdat.arc')
        text_report=patch_text(member(assets,name),game/name)
        patched_owner['files'][name]['sha256']=text_report['output_sha256']
        patched_owner['files'][name]['bytes']=text_report['bytes']
        (out/'ui-text-patch-report.json').write_text(json.dumps(text_report,indent=2)+'\n')
    for name,record in manifest['files'].items():
        if digest(package/name)!=record['sha256']:raise ValueError('Copied native file mismatch: '+name)
    copied=verify_inventory(game,patched_owner);validate(game)
    settings=tomllib.loads((native/title/'settings.toml').read_text())
    record={'status':'PASS_PRIVATE_TITLE_COPY','build_id':manifest['build_id'],'title_id':title,
        'native_files':len(manifest['files']),'owner_files':copied['files'],'owner_bytes':copied['bytes'],
        'source_inventory':source_check['status'],'personal_saves_included':False,
        'distribution':'PRIVATE_OWNER_COPY_ONLY','PS5_runtime':'NOT_TESTED',
        'aspect_ratio':settings.get('aspect_ratio','auto'),'fps_cap':settings.get('fps_cap'),
        'output':[settings.get('width'),settings.get('height')],
        'native_render_resolution':settings.get('native_render_resolution',False),
        'dualsense_prompts':bool(prompt_report),'ui_aspect_correction':settings.get('ui_aspect_correction',False),
        'ui_text_patch':bool(text_report),'ui_text_coverage':text_report['coverage'] if text_report else None,
        'gpu_callbacks_async':settings.get('gpu_callbacks_async',False),
        'baseline':'Correctness test; native 4K and gameplay performance remain unverified'}
    (out/'private-title-manifest.json').write_text(json.dumps(record,indent=2)+'\n')
    (out/'PRIVATE-OWNER-COPY.txt').write_text('This folder contains your extracted copyrighted game data.\n'
        'Keep it private; never commit or publish it. No saves are included.\n'
        'Independent title PPSA99145; PS5 runtime not yet tested.\n'
        'Configured aspect: '+str(record['aspect_ratio'])+'; FPS cap: '+str(record['fps_cap'])+'.\n'
        'Kenney filled-white prompts: '+str(bool(prompt_report))+'. Native 4K remains unverified.\n')
    print(json.dumps(record,indent=2))

if __name__=='__main__':main()
