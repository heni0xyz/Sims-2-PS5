"""Build shell artwork from szampan's original PNGs; PXZ is source-only.

Deterministic asset conversion, preserving the original files and composition.
The established PS5 title recipe consumes icon0.png, pic0.dds and pic1.dds.
"""
from pathlib import Path
import argparse,hashlib,json,shutil,struct
from PIL import Image
ROOT=Path(__file__).resolve().parents[2]

def prepare(output):
    output=Path(output);output.mkdir(parents=True,exist_ok=True)
    source=ROOT/'artwork/source'
    with Image.open(source/'sims2.png') as image:
        if image.size!=(2048,2048):raise ValueError('Expected the supplied 2048x2048 icon')
        icon=image.convert('RGB').resize((512,512),Image.Resampling.LANCZOS)
        icon.save(output/'icon0.png',optimize=True)
    with Image.open(source/'sims2bg.png') as image:
        if image.size!=(3840,2160):raise ValueError('Expected the supplied 3840x2160 background')
        background=image.convert('RGBA').resize((1920,1080),Image.Resampling.LANCZOS)
        background.save(output/'pic0.dds')
        # Verify actual DDS pixels/dimensions, rather than trusting its extension.
        with Image.open(output/'pic0.dds') as decoded:
            if decoded.size!=background.size or decoded.convert('RGBA').tobytes()!=background.tobytes():
                raise ValueError('DDS roundtrip mismatch')
        shutil.copyfile(output/'pic0.dds',output/'pic1.dds')
    record={'author':'szampan','display_title':'The Sims 2','source_only':['sims2.pxz','sims2bg.pxz'],
        'files':{p.name:{'bytes':p.stat().st_size,'sha256':hashlib.sha256(p.read_bytes()).hexdigest()}
                 for p in (output/'icon0.png',output/'pic0.dds',output/'pic1.dds')}}
    param=ROOT/'ps5/sce_sys/param.json'
    shutil.copyfile(param,output/'param.json')
    record['files']['param.json']={'bytes':param.stat().st_size,'sha256':hashlib.sha256(param.read_bytes()).hexdigest()}
    (output/'branding-manifest.json').write_text(json.dumps(record,indent=2)+'\n')
    return record

def main():
    p=argparse.ArgumentParser();p.add_argument('--output',type=Path,default=ROOT/'out/branding/sce_sys');a=p.parse_args()
    print(json.dumps(prepare(a.output),indent=2))
if __name__=='__main__':main()
