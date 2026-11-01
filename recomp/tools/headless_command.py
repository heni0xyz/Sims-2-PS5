"""Record a normal controller command/snapshot request for the active local test."""
from pathlib import Path
import argparse,datetime,json,re
ROOT=Path(__file__).resolve().parents[1]
def main():
    p=argparse.ArgumentParser();p.add_argument('--buttons');p.add_argument('--frames',type=int,default=120)
    p.add_argument('--run',help='Specific active local run ID; default is the most recently started test')
    p.add_argument('--snapshot',action='store_true')
    p.add_argument('--capture-draws',action='store_true',help='One-shot real GPU draw diagnostics; requires --ui-layout-trace on the PC runner')
    p.add_argument('--native-resolution',choices=('on','off'),help='PC-only render-scale diagnostic; runner needs --ui-layout-trace')
    p.add_argument('--font-upscale',choices=('on','off'),help='PC-only font A/B comparison; runner needs --ui-layout-trace')
    p.add_argument('--stop',action='store_true',help='End this PC observation run; this is a watchdog stop, not a clean title exit')
    a=p.parse_args()
    allowed={'UP','DOWN','LEFT','RIGHT','START','BACK','A','B','X','Y','LT','RT','LS','LSD','LSL','LSR'}
    if a.buttons and any(x not in allowed for x in a.buttons.split('+')):p.error('Use SDK harness button names')
    if not 1<=a.frames<=10000:p.error('frames must be 1..10000')
    if not a.buttons and not a.snapshot and not a.stop and not a.native_resolution and not a.capture_draws and not a.font_upscale:p.error('Request input, capture, resolution, font comparison or a PC test stop')
    if a.stop and (a.buttons or a.snapshot or a.native_resolution or a.capture_draws or a.font_upscale):p.error('Stop must be requested separately')
    if sum(bool(x) for x in (a.capture_draws,a.native_resolution,a.font_upscale))>1:p.error('Request one diagnostic command at a time')
    if a.run and not re.fullmatch(r'\d{8}T\d{6}Z-[a-f0-9]{6}',a.run):p.error('Invalid local run ID')
    record_path=ROOT/'out/runs'/a.run/'process.json' if a.run else ROOT/'out/current-process.json'
    record=json.loads(record_path.read_text())
    run=Path(record['artifact_directory']).resolve();run.relative_to((ROOT/'out/runs').resolve())
    if run.name!=record['run_id'] or (run/'run-summary.json').exists():p.error('No active bounded test')
    if a.snapshot and (run/'snap').exists():p.error('Previous snapshot is still rendering; wait for its completion')
    if a.buttons and (run/'cmd').exists():p.error('Previous controller command has not yet been consumed')
    if a.native_resolution or a.capture_draws or a.font_upscale:
        command=run/'ui-diagnostic.cmd'
        if command.exists():p.error('Previous diagnostic command has not been consumed')
        command.write_text('capture_frame 1\n' if a.capture_draws else ('font_upscale '+('1' if a.font_upscale=='on' else '0')+'\n' if a.font_upscale else 'native_resolution '+('1' if a.native_resolution=='on' else '0')+'\n'))
    stamp=datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%S.%fZ')
    if a.snapshot:
        archive=run/'captures'/stamp;archive.mkdir(parents=True)
        for image in run.glob('snap_*.tga'):(archive/image.name).write_bytes(image.read_bytes())
        for image in (run/'cap/vulkan').glob('*.bmp'):(archive/image.name).write_bytes(image.read_bytes())
        # Retain diagnostics from older runtime builds whose failed path
        # canonicalization wrote into the run root instead of cap/vulkan.
        for image in run.glob('front_*.bmp'):(archive/image.name).write_bytes(image.read_bytes())
        (run/'snap').touch()
        (run/'snapshot-request.json').write_text(json.dumps({'requested_at':stamp,'request_unix_ns':(run/'snap').stat().st_mtime_ns})+'\n')
    if a.buttons:
        command=run/'cmd'
        temp=run/('cmd-'+stamp+'.tmp');temp.write_text(a.buttons+' '+str(a.frames)+'\n')
        temp.rename(command)
    if a.stop:(run/'stop-test').touch()
    event={'requested_at':stamp,'buttons':a.buttons,'held_frames':a.frames if a.buttons else None,
           'snapshot_requested':a.snapshot,'capture_draws':a.capture_draws,'test_stop_requested':a.stop,'native_resolution':a.native_resolution,'font_upscale':a.font_upscale,'scope':'local SDK harness input; acceptance must be checked visually'}
    with (run/'input-events.jsonl').open('a') as stream:stream.write(json.dumps(event)+'\n')
    print(json.dumps(event))
if __name__=='__main__':main()
