# SPDX-License-Identifier: GPL-3.0-only
"""Render Kenney's CC0 vectors at 256px and build a host-only texture pack.

No original game pixels enter the pack. The optional selector preparation reads
the owner's archive only to record irreversible texture fingerprints.
"""
from pathlib import Path
import argparse,hashlib,io,json,struct,sys
from PIL import Image
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'out/python-deps'))
MASK=(1<<64)-1
def gpu_hash(data):
    k0=0x9E3779B97F4A7C15;k1=0xE7037ED1A0B428DB
    def mix(a,b):
        r=a*b;return (r&MASK)^(r>>64)
    seed=1469598103934665603^k0;i=0
    while i+16<=len(data):
        x,y=struct.unpack_from('<QQ',data,i);seed=mix(x^k1,y^seed);i+=16
    r=len(data)-i
    if r:
        x=int.from_bytes(data[i:i+min(r,8)],'little');y=int.from_bytes(data[-8:],'little') if r>=8 else 0
        seed=mix(x^k1,y^seed^r)
    return mix(seed^k0,len(data)^k1)

def selectors(archive):
    from patch_prompts import read_index,texture_mips,digest
    manifest=json.loads((ROOT/'mods/dualsense/manifest.json').read_text())
    if digest(archive)!=manifest['source_archive_sha256']:raise ValueError('Expected untouched supported owner archive')
    records=[]
    with archive.open('rb') as f:
        index=read_index(f)
        for item in manifest['replacements']:
            _,_,original=texture_mips(f,index[item['resource']],item['resource'])
            patched=(ROOT/'mods/dualsense'/item['mips_file']).read_bytes()
            if hashlib.sha256(original).hexdigest()!=item['original_mips_sha256']:raise ValueError('Original fingerprint mismatch')
            if hashlib.sha256(patched).hexdigest()!=item['mips_sha256']:raise ValueError('Patched fingerprint mismatch')
            records.append(dict(resource=item['resource'],glyph=item['glyph'],original=f'{gpu_hash(original):016x}',patched=f'{gpu_hash(patched):016x}',svg_sha256=item['svg_sha256']))
    out=ROOT/'mods/dualsense-hd';out.mkdir(exist_ok=True)
    (out/'selectors.json').write_text(json.dumps({'schema':1,'source_format':'BC2 32x32/16x16/8x8','source_bytes':1344,'selectors':records},indent=2)+'\n')

def build(output):
    import resvg_py
    if resvg_py.__version__!='0.5.0':raise ValueError('Use the recorded resvg_py 0.5.0')
    source=json.loads((ROOT/'mods/dualsense-hd/selectors.json').read_text());items=source['selectors']
    if source['source_bytes']!=1344 or len(items)!=17:raise ValueError('Unexpected selector inventory')
    output=Path(output);output.mkdir(parents=True,exist_ok=True)
    width=height=256;levels=9;index_size=64
    header=struct.pack('<8s6I',b'S2HDPACK',1,len(items),width,height,levels,index_size)
    payload=bytearray();index=bytearray();records=[];base=len(header)+len(items)*index_size
    for item in items:
        svg=ROOT/'mods/dualsense/kenney/SVG'/(item['glyph']+'.svg')
        if hashlib.sha256(svg.read_bytes()).hexdigest()!=item['svg_sha256']:raise ValueError('SVG source mismatch: '+item['glyph'])
        png=resvg_py.svg_to_bytes(svg_path=str(svg),width=width,height=height,skip_system_fonts=True)
        image=Image.open(io.BytesIO(png)).convert('RGBA');image.save(output/(item['resource']+'.png'))
        data=b''.join(image.convert('RGBa').resize((width>>level,height>>level),Image.Resampling.LANCZOS).convert('RGBA')
            .transpose(Image.Transpose.FLIP_TOP_BOTTOM).tobytes('raw','BGRA') for level in range(levels))
        name=item['resource'].encode('ascii')
        if len(name)>=32:raise ValueError('Texture name too long')
        index+=struct.pack('<QQII32s8x',int(item['original'],16),int(item['patched'],16),base+len(payload),len(data),name)
        payload+=data
        records.append(dict(resource=item['resource'],glyph=item['glyph'],bytes=len(data),sha256=hashlib.sha256(data).hexdigest()))
    contents=header+index+payload
    pack=output/'dualsense.s2tex';pack.write_bytes(contents+struct.pack('<Q',gpu_hash(contents)))
    report={'schema':1,'license':'CC0-1.0','author':'Kenney','compiler_author':'OpenAI Codex for szampan',
        'renderer':'resvg_py 0.5.0','width':width,'height':height,'levels':levels,'format':'BGRA8_UNORM, bottom-up mip levels',
        'original_game_pixels_included':False,'bytes':pack.stat().st_size,'sha256':hashlib.sha256(pack.read_bytes()).hexdigest(),'textures':records}
    (output/'manifest.json').write_text(json.dumps(report,indent=2)+'\n');return report

def main():
    p=argparse.ArgumentParser();p.add_argument('--prepare-selectors',type=Path,metavar='OWNER_TEXTURES_ARC');p.add_argument('--output',type=Path,default=ROOT/'out/hd-prompts');a=p.parse_args()
    if a.prepare_selectors:selectors(a.prepare_selectors)
    report=build(a.output);print(json.dumps({k:v for k,v in report.items() if k!='textures'},indent=2))
if __name__=='__main__':main()
