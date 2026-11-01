"""Lossless PNG copies of real final Vulkan readbacks; no crop/resize/repaint."""
from pathlib import Path
import argparse,hashlib,json,re
from PIL import Image
ROOT=Path(__file__).resolve().parents[1]

def main():
    p=argparse.ArgumentParser();p.add_argument('run');a=p.parse_args()
    if not re.fullmatch(r'\d{8}T\d{6}Z-[a-f0-9]{6}',a.run):p.error('Use a local run ID')
    run=ROOT/'out/runs'/a.run
    process=json.loads((run/'process.json').read_text())
    if 'VULKAN_OFFSCREEN' not in (run/'game.raw.log').read_text(errors='replace'):p.error('No real Vulkan startup marker')
    records=[];out=ROOT/'out/screens'/a.run;out.mkdir(parents=True,exist_ok=True)
    for source in sorted((run/'cap/vulkan').glob('presented_*.bmp')):
        with Image.open(source) as frame:
            if frame.width*9!=frame.height*16:raise ValueError('Actual readback is not 16:9: '+source.name)
            target=out/(source.stem+'.png');frame.save(target)
            records.append({'file':target.name,'width':frame.width,'height':frame.height,
                'source_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),
                'png_sha256':hashlib.sha256(target.read_bytes()).hexdigest()})
    if not records:raise ValueError('No final presented readbacks; raw fronts are not display images')
    manifest={'run_id':a.run,'exe_sha256':process['exe_sha256'],'images':records,
        'operation':'Lossless format conversion only; no resize/crop/repaint',
        'scope':'PC software Vulkan final presentation; not native render resolution or PS5 FPS proof'}
    (out/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
    print(json.dumps({'directory':str(out),**manifest},indent=2))
if __name__=='__main__':main()
