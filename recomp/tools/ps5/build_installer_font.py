# SPDX-License-Identifier: GPL-3.0-only
"""Bake the supplied unmodified OFL Open Sans into a public installer resource."""
from pathlib import Path
import argparse,hashlib,json,struct
from PIL import Image,ImageFont
ROOT=Path(__file__).resolve().parents[2]
EXPECTED='33e93bec67d91c396876db50694213802a39e43a911bb5c22322f0dbdf4d5e43'
SIZES=(56,96,144)
X_SCALE=1.0 # Native 3840x2160 canvas; square pixels, no guest-picture stretch

def build(output):
    source=ROOT/'third_party/opensans/OpenSans-Regular.ttf'
    if hashlib.sha256(source.read_bytes()).hexdigest()!=EXPECTED:raise ValueError('Open Sans source fingerprint mismatch')
    output=Path(output);output.mkdir(parents=True,exist_ok=True)
    payload=bytearray();table=bytearray();base=24+len(SIZES)*(16+95*16+95*95*2)
    for size in SIZES:
        font=ImageFont.truetype(str(source),size=size);ascent,descent=font.getmetrics()
        table+=struct.pack('<4H2I',size,ascent,descent,ascent+descent,95,95*95)
        lengths={chr(c):font.getlength(chr(c)) for c in range(32,127)}
        for code in range(32,127):
            glyph=chr(code);mask,offset=font.getmask2(glyph,mode='L',anchor='ls');width,height=mask.size
            data=bytes(mask)
            if width and height:
                new_width=max(1,round(width*X_SCALE));data=Image.frombytes('L',(width,height),data).resize((new_width,height),Image.Resampling.LANCZOS).tobytes();width=new_width
            table+=struct.pack('<4h2HI',round(lengths[glyph]*64*X_SCALE),round(offset[0]*X_SCALE),offset[1],0,width,height,base+len(payload));payload+=data
        for left in range(32,127):
            for right in range(32,127):
                a,b=chr(left),chr(right);table+=struct.pack('<h',round((font.getlength(a+b)-lengths[a]-lengths[b])*64*X_SCALE))
    data=struct.pack('<8s4I',b'S2FONT1\0',1,3,95,16)+table+payload
    resource=output/'installer-font.s2font';resource.write_bytes(data+hashlib.sha1(data).digest())
    report={'schema':1,'font':'Open Sans Regular 3.003','copyright':'Copyright 2020 The Open Sans Project Authors',
            'license':'OFL-1.1','source_sha256':EXPECTED,'sizes':SIZES,'glyphs_per_size':95,'kerning':True,'horizontal_picture_compensation':X_SCALE,
            'resource_bytes':resource.stat().st_size,'resource_sha256':hashlib.sha256(resource.read_bytes()).hexdigest(),
            'compiler':'Pillow/FreeType; authored pack/layout code GPL-3.0-only; font data remains OFL-1.1'}
    (output/'installer-font-manifest.json').write_text(json.dumps(report,indent=2)+'\n');return report

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--output',type=Path,default=ROOT/'out/installer-font');a=p.parse_args()
    print(json.dumps(build(a.output),indent=2))
