"""Link independent guest objects using the established template/RADV recipe."""
from pathlib import Path
import argparse,hashlib,json,os,re,shutil,subprocess,tarfile,time
from artifact import validate_artifact

ROOT=Path(__file__).resolve().parents[2]

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--inputs',type=Path,required=True)
    parser.add_argument('--foundation',type=Path,required=True)
    parser.add_argument('--vulkan',type=Path,required=True)
    parser.add_argument('--payload-sdk-fork',type=Path,required=True)
    parser.add_argument('--retry',action='store_true',help='Retry a recorded failed link with identical inputs')
    parser.add_argument('--validate-only',action='store_true',help='Validate/package an already linked matching build')
    parser.add_argument('--resolution',choices=('1080p','4k'),default='1080p',help='Package output mode; 4k also enables the minimum native-detail policy')
    args=parser.parse_args()
    with tarfile.open(args.inputs) as archive:
        manifest=json.load(archive.extractfile('link-input-manifest.json'))
        identity=manifest['build_id']
        if not re.fullmatch('[a-f0-9]{24}',identity):raise ValueError('Unexpected build identity')
        profile=manifest['build_profile']
        for name,value in {'build_id':identity,'optimization':'O2','fast_handles':'ON','object_wait_channels':'ON'}.items():
            if profile.get(name)!=value:raise ValueError('Unexpected profile: '+name)
        out=ROOT/'out/native'/identity
        if args.retry or args.validate_only:
            status=json.loads((out/'status.json').read_text())
            if args.retry and status['state']!='LINK_FAILED':raise ValueError('Only a failed link may be retried')
            previous=json.loads((out/'inputs/link-input-manifest.json').read_text())
            if previous!=manifest:raise ValueError('Retry inputs differ from the recorded build')
            if args.retry and (out/'link.log').exists():
                (out/'link.log').rename(out/('link-failed-'+str(time.time_ns())+'.log'))
        resume=args.retry or args.validate_only
        out.mkdir(parents=True,exist_ok=resume)
        inputs=out/'inputs';inputs.mkdir(exist_ok=resume);archive.extractall(inputs,filter='data')
    environment=dict(os.environ,PS5_PAYLOAD_SDK_FORK=str(args.payload_sdk_fork.resolve()),PS5_CLANG='/usr/bin/clang-19')
    start=time.monotonic()
    if args.validate_only:
        if not (inputs/'link/llvm-pie.elf').is_file():raise ValueError('No matching linked ELF to validate')
    else:
      with (out/'link.log').open('wb') as log:
        result=subprocess.run(['bash',str(ROOT/'tools/ps5/link_exported.sh'),str(inputs),
             str(args.foundation.resolve()),str(args.vulkan.resolve())],env=environment,
             cwd=ROOT,stdout=log,stderr=subprocess.STDOUT,timeout=1800)
    if not args.validate_only and result.returncode:
        (out/'status.json').write_text(json.dumps({'state':'LINK_FAILED','returncode':result.returncode})+'\n')
        print('\n'.join((out/'link.log').read_text(errors='replace').splitlines()[-25:]));return result.returncode
    param=json.loads((args.foundation/'ps5/sce_sys/param.json').read_text())
    if param['titleId']!='PPSA99145':raise ValueError('Independent title identity mismatch')
    package=args.foundation/'dist'/param['titleId']
    brand_dir=inputs/'branding'
    param_override=None
    if brand_dir.exists():
        brand=json.loads((brand_dir/'branding-manifest.json').read_text())
        branded_param=json.loads((brand_dir/'param.json').read_text())
        if branded_param['titleId']!=param['titleId'] or branded_param['contentId']!=param['contentId']:
            raise ValueError('Branding must preserve the title/content identity')
        allowed={'icon0.png','pic0.dds','pic1.dds','param.json'}
        if set(brand['files'])!=allowed:raise ValueError('Unexpected branding inventory')
        for filename,record in brand['files'].items():
            source=brand_dir/filename
            if hashlib.sha256(source.read_bytes()).hexdigest()!=record['sha256']:raise ValueError('Branding digest mismatch')
            shutil.copyfile(source,package/'sce_sys'/filename)
        param_override=brand_dir/'param.json'
    native=args.resolution=='4k'
    public_files=('public-resources/dualsense.s2tex','public-resources/dualsense-manifest.json',
                  'LICENSE','LICENSING.md','licenses/artwork/CC0-1.0.txt',
                  'licenses/xbox-runtime/LICENSE.txt','licenses/xbox-runtime/LOCAL_CHANGES_LICENSE.md',
                  'public-resources/installer-font.s2font','public-resources/installer-font-manifest.json',
                  'public-resources/OpenSans-Regular.ttf','licenses/opensans/OFL.txt','licenses/opensans/NOTICE.txt')
    hd_prompts=(inputs/public_files[0]).is_file()
    if hd_prompts:
        for name in public_files:
            source=inputs/name;record=manifest['files'][name]
            if source.stat().st_size!=record['bytes'] or hashlib.sha256(source.read_bytes()).hexdigest()!=record['sha256']:
                raise ValueError('Public resource digest mismatch: '+name)
            destination=package/name;destination.parent.mkdir(parents=True,exist_ok=True)
            shutil.copyfile(source,destination)
    width,height=(3840,2160) if native else (1920,1080)
    settings=f'width={width}\nheight={height}\nrender_percent=100\nfps_cap=60\nvsync=true\naspect_ratio="16:9"\nui_aspect_correction=true\nui_edge_anchors=true\nui_native_portrait=true\nhd_input_prompts={str(hd_prompts).lower()}\nui_menu_diagnostics=true\nui_continue_refresh=true\nui_font_upscale=true\ngpu_callbacks_async=false\nnative_render_resolution={str(native).lower()}\n'
    (package/'settings.toml').write_text(settings)
    resolution={'output':[width,height],'native_render_resolution':native,'expected_backing':[3840,2880] if native else None,
                'expected_picture':[3840,2688] if native else None,'PS5_measured':'NOT_TESTED'}
    (package/'BUILD-PROFILE.json').write_text(json.dumps(dict(profile,game_source='independent USA XBE recompilation',resolution=resolution,runtime='NOT_TESTED'),indent=2)+'\n')
    (package/'INDEPENDENT-README.txt').write_text('Independent recompilation candidate. Build '+identity+'\n'
        'Not yet verified on PS5. Supply your legal USA assets. No game data is included here.\n'
        'First install: put exactly one USA Xbox .iso beside eboot.bin, launch and press Cross to Extract.\n'
        'Installation writes verified assets and applies English PlayStation/storage-device notices.\n'
        'After success choose Cross to delete the source ISO or Circle to keep it. No automatic deletion.\n'
        'Keep sce_sys, sce_module and public-resources with eboot.bin. About 2 GB extra storage is required.\n'
        '16:9 with authored UI correction, 60 FPS cap (performance not verified).\n'
        +('Kenney CC0 prompts: 256x256 host textures; raw and privately patched USA assets supported.\n' if hd_prompts else 'Kenney icons require the private data-patched copy.\n')
        +('4K mode: expected backing 3840x2880, visible picture 3840x2688, output 3840x2160. GPU cost rises; PS5 verification required.\n' if native else 'Original height-based render scaling.\n')+
        'Keep the previous working title and its saves separate. Credits: CREDITS.txt.\n')
    prompt_license=package/'licenses/input-prompts/License.txt'
    prompt_license.parent.mkdir(parents=True,exist_ok=True)
    shutil.copyfile(ROOT/'mods/dualsense/kenney/License.txt',prompt_license)
    elf=inputs/'link/llvm-pie.elf'
    verified,package,elf=validate_artifact(args.foundation,args.vulkan,out,elf_override=elf,
        kind='independent_game_bringup_candidate',build_id=identity,param_override=param_override)
    archive=shutil.make_archive(str(out/'Sims2Independent-PPSA99145'),'zip',root_dir=package.parent,base_dir=package.name)
    report={'state':'BUILD_PASS_RUNTIME_NOT_TESTED','build_id':identity,'title_id':param['titleId'],
            'seconds':time.monotonic()-start,'eboot_sha256':verified['eboot_sha256'],
            'debug_elf_sha256':verified['debug_elf_sha256'],'package':str(archive),'debug_elf':str(elf),
            'resolution':resolution,'gameplay':'NOT_TESTED','performance':'NOT_TESTED'}
    (out/'status.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report),flush=True)
    return 0

if __name__=='__main__':raise SystemExit(main())
