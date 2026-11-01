"""Bounded asset-backed SDK harness run; frame counters are not visual proof."""
from pathlib import Path
import argparse,datetime,hashlib,json,os,re,shutil,signal,subprocess,time,uuid
from check_assets import validate

ROOT=Path(__file__).resolve().parents[1]

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--exe',type=Path,default=ROOT/'out/headless/Sims2Independent')
    parser.add_argument('--assets',type=Path,default=ROOT/'assets')
    parser.add_argument('--seconds',type=int,default=20)
    parser.add_argument('--fps',type=int,default=0,help='Optional harness frame cap for correctness tests; zero is an unpaced logic probe')
    parser.add_argument('--backend',choices=('null','vulkan'),default='null',help='Expected renderer; Vulkan requires its startup marker and a captured real frame')
    parser.add_argument('--aspect',choices=('4:3','16:9'),default='4:3',help='Presentation/UI test mode; no extra world FOV multiplier')
    parser.add_argument('--output',choices=('1280x720','3840x2160'),default='1280x720',help='Actual Vulkan offscreen image extent')
    parser.add_argument('--native-resolution',action='store_true',help='Rasterize at least the output detail in both picture axes')
    parser.add_argument('--ui-layout-trace',action='store_true',help='Bounded private UI bridge observations; no game logic is skipped')
    parser.add_argument('--ui-texture-trace',action='store_true',help='Bounded private UI texture/palette images for asset identification; PC diagnostics only')
    parser.add_argument('--menu-diagnostics',action='store_true',help='Bounded numeric Continue geometry and variable diagnostics')
    parser.add_argument('--hd-prompts',action='store_true',help='Stage and verify the public Kenney 256px host texture pack')
    parser.add_argument('--font-upscale',action='store_true',help='Mathematical 4x reconstruction of selected owner font atlases')
    parser.add_argument('--gpu-callbacks',choices=('sync','ordered-write'),default='sync',help='Explicit isolated callback experiment; default retains the synchronous correctness baseline')
    parser.add_argument('--hdd-from',help='Clone only this completed local test run HDD; original evidence remains unchanged')
    parser.add_argument('--fresh-cache',action='store_true',help='Copy synthetic saves but start with a new Z cache so changed owner assets are actually read')
    parser.add_argument('--debug',action='store_true')
    parser.add_argument('--script',type=Path,help='SDK controller script: frame buttons [held frames]')
    parser.add_argument('--reference-every',type=int,default=0,help='CPU reference render every Nth frame; slow')
    parser.add_argument('--snapshot-after',type=float,help='Ask the SDK harness to render the next two frames at this elapsed time')
    parser.add_argument('--press-at',action='append',default=[],metavar='SECONDS:BUTTONS:FRAMES',help='Schedule normal SDK harness controller input by elapsed time')
    args=parser.parse_args()
    if not 1<=args.seconds<=1800:parser.error('seconds must be 1..1800 (bounded PC correctness test)')
    if not 0<=args.fps<=240:parser.error('fps must be 0..240')
    if not 0<=args.reference_every<=100000:parser.error('reference-every must be 0..100000')
    if args.snapshot_after is not None and not 0<=args.snapshot_after<args.seconds:parser.error('snapshot must occur within the run')
    if args.script and (not args.script.is_file() or args.script.stat().st_size>65536):parser.error('script must be a file no larger than 64 KiB')
    actions=[]
    allowed={'UP','DOWN','LEFT','RIGHT','START','BACK','A','B','X','Y','LT','RT','LS','LSD','LSL','LSR'}
    for spec in args.press_at:
        try:
            at,buttons,held=spec.split(':');at=float(at);held=int(held)
            if not 0<=at<args.seconds or not 1<=held<=10000 or any(x not in allowed for x in buttons.split('+')):raise ValueError()
        except ValueError:parser.error('Invalid timed input; use seconds:SDK-button:held-frames within run bounds')
        actions.append({'at_seconds':at,'buttons':buttons,'held_frames':held,'delivered':False,'consumed':False})
    actions.sort(key=lambda x:x['at_seconds'])
    exe=args.exe.resolve();assets=args.assets.resolve()
    if not exe.is_file():parser.error('Executable is missing; build it before running')
    try:
        if assets.is_file() and assets.suffix.casefold()=='.iso':
            verifier=ROOT/'out/host/bin'/('iso_verify.exe' if os.name=='nt' else 'iso_verify')
            check=subprocess.run([str(verifier),str(assets)],capture_output=True,text=True,timeout=60)
            if check.returncode:raise ValueError(check.stderr.strip() or 'ISO verification failed')
            asset_check=json.loads(check.stdout.strip());asset_check['iso_bytes']=assets.stat().st_size
        else:asset_check=validate(assets)
    except (OSError,ValueError) as error:raise SystemExit('Run refused: '+str(error))
    run_id=datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%SZ')+'-'+uuid.uuid4().hex[:6]
    out=ROOT/'out/runs'/run_id;out.mkdir(parents=True)
    # Resolve an existing directory in the runtime's capture canonicalization.
    (out/'cap/vulkan').mkdir(parents=True)
    # Run a private immutable copy. Windows locks mapped executables, and a
    # concurrent rebuild must neither change the recorded hash nor fail merely
    # because a bounded test is still using its own previous artifact.
    source_exe=exe
    staged=out/'executable'/exe.name;staged.parent.mkdir()
    shutil.copy2(source_exe,staged)
    exe=staged
    exe_hash=hashlib.sha256(exe.read_bytes()).hexdigest()
    (out/'asset-check.json').write_text(json.dumps(asset_check,indent=2)+'\n')
    if args.hdd_from:
        if not re.fullmatch(r'\d{8}T\d{6}Z-[a-f0-9]{6}',args.hdd_from):parser.error('Use a completed local run ID for hdd-from')
        previous=ROOT/'out/runs'/args.hdd_from
        if not (previous/'run-summary.json').is_file():parser.error('Source test did not complete')
        source=(previous/'hdd').resolve()
        source.relative_to(previous.resolve())
        if not source.is_dir():parser.error('Source test has no HDD state')
        for path in source.rglob('*'):
            if path.is_symlink() or not path.resolve().is_relative_to(source):parser.error('Linked HDD members are not accepted')
        ignore=(lambda directory,names: [name for name in names if Path(directory)==source and name.casefold()=='z']) if args.fresh_cache else None
        shutil.copytree(source,out/'hdd',ignore=ignore)
        (out/'hdd-source-summary.json').write_bytes((previous/'run-summary.json').read_bytes())
    command=[str(exe),str(assets)]
    if args.debug:
        command=['gdb','-batch','-ex','set pagination off','-ex','run',
                 '-ex','thread apply all bt 20','--args',*command]
    environment=dict(os.environ);environment.pop('REF',None)
    environment.pop('SIMS2_GPU_CALLBACKS_SYNC',None);environment.pop('SIMS2_GPU_CALLBACKS_ASYNC',None)
    environment['SIMS2_GPU_CALLBACKS_SYNC' if args.gpu_callbacks=='sync' else 'SIMS2_GPU_CALLBACKS_ASYNC']='1'
    environment.pop('SCRIPT',None);environment.pop('CAP',None)
    environment['HARNESS_FPS']=str(args.fps)
    environment['SIMS2_TEST_ASPECT']=args.aspect
    environment['HARNESS_OUTPUT_WIDTH'],environment['HARNESS_OUTPUT_HEIGHT']=args.output.split('x')
    environment.pop('SIMS2_NATIVE_RESOLUTION',None)
    if args.native_resolution:environment['SIMS2_NATIVE_RESOLUTION']='1'
    if args.ui_layout_trace:
        environment['SIMS2_UI_LAYOUT_TRACE']='1';environment['SIMS2_UI_TRACE']='1'
    else:environment.pop('SIMS2_UI_LAYOUT_TRACE',None)
    environment['XBE_CAPTURE_DIR']='cap/vulkan'
    environment.pop('SIMS2_UI_TEXTURE_TRACE',None)
    if args.ui_texture_trace:environment['SIMS2_UI_TEXTURE_TRACE']='1'
    environment.pop('SIMS2_UI_MENU_DIAGNOSTICS',None)
    environment.pop('SIMS2_FONT_UPSCALE',None)
    if args.font_upscale:environment['SIMS2_FONT_UPSCALE']='1'
    if args.menu_diagnostics:environment['SIMS2_UI_MENU_DIAGNOSTICS']='1'
    environment.pop('SIMS2_HD_PROMPTS',None);environment.pop('SIMS2_PUBLIC_RESOURCES',None)
    prompt_pack_sha256=None
    if args.hd_prompts:
        source=ROOT/'out/hd-prompts/dualsense.s2tex'
        if not source.is_file():parser.error('Build the HD prompt pack first')
        folder=out/'public-resources';folder.mkdir()
        shutil.copyfile(source,folder/source.name)
        prompt_pack_sha256=hashlib.sha256((folder/source.name).read_bytes()).hexdigest()
        environment['SIMS2_HD_PROMPTS']='1';environment['SIMS2_PUBLIC_RESOURCES']=str(folder)
    script_hash=None
    if args.script:
        script=out/'input.script';shutil.copyfile(args.script,script)
        script_hash=hashlib.sha256(script.read_bytes()).hexdigest()
        environment['SCRIPT']=str(script)
    if args.reference_every:environment['REF']=str(args.reference_every)
    snapshot_requested=False
    pending_input=None
    log=out/'game.raw.log';start=time.monotonic();reason='PROCESS_EXIT'
    with log.open('wb') as stream:
        process=subprocess.Popen(command,cwd=out,env=environment,stdout=stream,
            stderr=subprocess.STDOUT,start_new_session=os.name!='nt')
        process_record={'pid':process.pid,'run_id':run_id,'artifact_directory':str(out),'command':command,
                        'source_executable':str(source_exe),'exe_sha256':exe_hash}
        (out/'process.json').write_text(json.dumps(process_record,indent=2)+'\n')
        (ROOT/'out/current-process.json').write_text(json.dumps(process_record,indent=2)+'\n')
        while process.poll() is None:
            if pending_input is not None and not (out/'cmd').exists():
                pending_input['consumed']=True;pending_input=None
            if pending_input is None and not (out/'cmd').exists():
                due=next((action for action in actions if not action['delivered'] and time.monotonic()-start>=action['at_seconds']),None)
                if due is not None:
                    temp=out/'cmd.pending';temp.write_text(due['buttons']+' '+str(due['held_frames'])+'\n')
                    temp.rename(out/'cmd');due['delivered']=True;due['delivered_at_seconds']=time.monotonic()-start;pending_input=due
            if args.snapshot_after is not None and not snapshot_requested and time.monotonic()-start>=args.snapshot_after:
                (out/'snap').touch();snapshot_requested=True
            if time.monotonic()-start>=args.seconds:reason='TIME_BOUND';break
            if (out/'stop-test').is_file():reason='TEST_STOP_REQUESTED';break
            if log.stat().st_size>64*1024*1024:reason='LOG_BUDGET_EXCEEDED';break
            time.sleep(.2)
        if process.poll() is None:
            if os.name!='nt':os.killpg(process.pid,signal.SIGTERM)
            else:process.terminate()
            try:process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                if os.name!='nt':os.killpg(process.pid,signal.SIGKILL)
                else:process.kill()
                process.wait(timeout=5)
    text=log.read_text(errors='replace')
    fatal=[line[:1500] for line in text.splitlines()
           if re.search(r'\btrap\b|received signal|\berror:|\[error\]|fatal signal|no recompiled function|\[GAME_FATAL\]|\[NATIVE_MESSAGE\]',line,re.I)]
    frames=[int(n) for n in re.findall(r'renderer: frame (\d+)',text)]
    swaps=[int(n) for n in re.findall(r'd3d: Swap (\d+)',text)]
    pacing=[int(n) for n in re.findall(r'harness: pacing fps=(\d+)',text)]
    reference_images=[p.name for p in sorted(out.glob('snap_*.tga'))]+[str(p.relative_to(out)) for p in sorted((out/'cap').glob('ref_*.tga'))]
    vulkan_images=[str(p.relative_to(out)) for p in sorted((out/'cap/vulkan').glob('*.bmp'))]
    backend_verified=('harness: backend=VULKAN_OFFSCREEN' in text) if args.backend=='vulkan' else 'harness: backend=VULKAN_OFFSCREEN' not in text
    capture_complete=not (out/'snap').exists() and (not snapshot_requested or bool(vulkan_images if args.backend=='vulkan' else reference_images))
    failure=bool(fatal) or (reason=='PROCESS_EXIT' and process.returncode!=0) or reason=='LOG_BUDGET_EXCEEDED'
    timing_verified=not args.fps or bool(pacing and pacing[0]==args.fps)
    if args.backend=='vulkan':
        observed=bool(frames and timing_verified and backend_verified and capture_complete and vulkan_images)
        successful_status='HOST_VULKAN_FRAMES_CAPTURED'
    else:
        observed=bool(frames and timing_verified and backend_verified and capture_complete)
        successful_status='HOST_LOGIC_FRAMES_OBSERVED'
    status='FAIL' if failure else (successful_status if observed else 'INCOMPLETE')
    native_signal=signal.Signals(-process.returncode).name if os.name!='nt' and reason=='PROCESS_EXIT' and process.returncode<0 else None
    native_status=('0x%08X'%(process.returncode&0xFFFFFFFF)) if os.name=='nt' and reason=='PROCESS_EXIT' and process.returncode else None
    record={'run_id':run_id,'status':status,'reason':reason,'returncode':process.returncode,
      'native_signal':native_signal,
      'native_status':native_status,
      'seconds':time.monotonic()-start,'exe_sha256':exe_hash,'source_executable':str(source_exe),
      'input_script_sha256':script_hash,'reference_every':args.reference_every,
      'hdd_source_run':args.hdd_from,
      'fresh_cache':args.fresh_cache,
      'requested_fps_cap':args.fps,'timing_configuration_verified':timing_verified,
      'requested_aspect':args.aspect,
      'requested_output':args.output,'native_resolution_at_start':args.native_resolution,
      'requested_gpu_callback_policy':args.gpu_callbacks,
      'font_upscale_requested':args.font_upscale,
      'recording_backend_observed':'render thread: recording backend active' in text,
      'gpu_callback_policy_verified':('GPU callback policy='+('synchronous' if args.gpu_callbacks=='sync' else 'ordered-write-completion')) in text,
      'hd_prompt_pack_sha256':prompt_pack_sha256,
      'timed_inputs':actions,
      'snapshot_requested':snapshot_requested,
      'reference_images':reference_images,'vulkan_images':vulkan_images,
      'expected_backend':args.backend,'backend_configuration_verified':backend_verified,
      'snapshot_completed':capture_complete,
      'clean_process_exit':reason=='PROCESS_EXIT' and process.returncode==0,
      'last_reported_frame':max(frames,default=0),'last_reported_swap_request':max(swaps,default=0),
      'kernel_imports_resolved':'kernel imports resolved' in text,
      'first_failure':fatal[0] if fatal else (('Process terminated by '+native_signal) if native_signal else (('Windows process failed: '+native_status) if native_status else None)),'fatal_records':fatal[:20],
      'scope':('SDK Vulkan offscreen on PC: captured images require review; no PS5, display, audio-output or gameplay acceptance'
               if args.backend=='vulkan' else 'SDK null backend: CPU/HLE logic with optional simplified CPU reference images; no Vulkan, PS5 or gameplay acceptance')}
    (out/'run-summary.json').write_text(json.dumps(record,indent=2)+'\n')
    (ROOT/'out/latest-run.json').write_text(json.dumps(dict(record,artifact_directory=str(out)),indent=2)+'\n')
    print(json.dumps(record,indent=2));print('artifacts:',out)
    return 1 if failure else (0 if observed else 2)

if __name__=='__main__':raise SystemExit(main())
