"""Correlate real paired Vulkan readbacks with the logged render extent.

No resizing or pixel editing. This verifies PC raster dimensions, not PS5
performance, original texture detail, or whether the captured scene is gameplay.
"""
from pathlib import Path
import argparse,hashlib,json,re
from PIL import Image,ImageStat
ROOT=Path(__file__).resolve().parents[1]

def digest(path):
    with path.open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()

def main():
    p=argparse.ArgumentParser();p.add_argument('run');a=p.parse_args()
    if not re.fullmatch(r'\d{8}T\d{6}Z-[a-f0-9]{6}',a.run):p.error('Use a local run ID')
    run=ROOT/'out/runs'/a.run
    text=(run/'game.raw.log').read_text(errors='replace')
    if 'harness: backend=VULKAN_OFFSCREEN' not in text:raise ValueError('No real Vulkan marker')
    expected='backing=3840x2880 picture=3840x2688 output=3840x2160 viewport=3840x2160 scale=6.0000 no_upscale=1'
    if expected not in text:raise ValueError('No native-detail 4K render-extent event')
    pairs=[]
    for final in sorted((run/'cap/vulkan').glob('presented_*.bmp')):
        frame=final.stem.removeprefix('presented_')
        backing=final.with_name('front_'+frame+'_host.bmp')
        guest=final.with_name('front_'+frame+'.bmp')
        if not backing.is_file() or not guest.is_file():continue
        with Image.open(final) as f,Image.open(backing) as b,Image.open(guest) as g:
            if f.size!=(3840,2160) or b.size!=(3840,2880) or g.size!=(640,480):continue
            if max(ImageStat.Stat(f.convert('RGB')).stddev)<1:raise ValueError('Uniform/empty final image')
            pairs.append({'frame':int(frame),'backing':[b.width,b.height],'picture':[3840,2688],
                          'presented':[f.width,f.height],'guest':[g.width,g.height],
                          'files':{path.name:digest(path) for path in (guest,backing,final)}})
    if not pairs:raise ValueError('No matching real 4K backing/final readback pair')
    process=json.loads((run/'process.json').read_text())
    record={'status':'PASS_PC_4K_RASTER_DIMENSIONS','run_id':a.run,'exe_sha256':process['exe_sha256'],
            'pairs':pairs,'PS5_runtime':'NOT_TESTED','PS5_performance':'NOT_TESTED',
            'scope':'Actual PC Vulkan buffers; scene/content requires visual review; original textures and movies retain source detail'}
    (run/'4k-verification.json').write_text(json.dumps(record,indent=2)+'\n')
    print(json.dumps(record,indent=2))
if __name__=='__main__':main()
