# Copyright (C) 2026 OpenAI Codex, for szampan
# SPDX-License-Identifier: GPL-3.0-only
"""Convert szampan's 4K CC0 background to the native PS5 BC7 DDS profile."""
from pathlib import Path
import argparse,hashlib,json,shutil,struct,subprocess
from PIL import Image,ImageChops,ImageStat
ROOT=Path(__file__).resolve().parents[2]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def prepare(cxx):
    inputs=[ROOT/'artwork/source/20261110/thesims2bg.png',Path(__file__),ROOT/'tools/branding/encode_bc7.cpp',
            ROOT/'tools/branding/third_party/bc7enc.h',ROOT/'tools/branding/third_party/bc7enc.cpp']
    key=hashlib.sha256(''.join(sha(p) for p in inputs).encode()).hexdigest()
    out=ROOT/'out/branding'/('home-'+key[:16]);out.mkdir(parents=True,exist_ok=True)
    if (out/'receipt.json').exists():
        r=json.loads((out/'receipt.json').read_text())
        if all(sha(out/n)==h for n,h in r['outputs'].items()):return out
    exe=out/'encode_bc7.exe'
    subprocess.run([str(cxx),'-std=c++17','-O2','-static','-pthread',str(inputs[2]),str(inputs[4]),'-o',str(exe)],check=True)
    with Image.open(inputs[0]) as image:
        source=image.convert('RGBA')
        if source.size!=(3840,2160):raise ValueError('Expected original 4K background')
        (out/'source.rgba').write_bytes(source.tobytes())
    subprocess.run([str(exe),str(out/'source.rgba'),'3840','2160',str(out/'pic0.dds')],check=True)
    data=(out/'pic0.dds').read_bytes()
    if (data[:4],data[84:88],struct.unpack_from('<I',data,128)[0],len(data))!=(b'DDS ',b'DX10',98,148+3840*2160):
        raise ValueError('Wrong PS5 DDS profile')
    with Image.open(out/'pic0.dds') as image:
        if image.size!=source.size:raise ValueError('Wrong decoded size')
        diff=ImageChops.difference(image.convert('RGBA'),source)
        error=max(ImageStat.Stat(diff).mean[:3])
        if error>3:raise ValueError('Unexpected background compression error')
    shutil.copyfile(out/'pic0.dds',out/'pic1.dds')
    (out/'receipt.json').write_text(json.dumps(dict(source_sha256=sha(inputs[0]),profile='3840x2160 BC7_UNORM DX10 straight-alpha',
        mean_channel_error=error,outputs={n:sha(out/n) for n in ('pic0.dds','pic1.dds')}),indent=2)+'\n')
    return out
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--cxx',type=Path,required=True);a=p.parse_args();print(prepare(a.cxx))
