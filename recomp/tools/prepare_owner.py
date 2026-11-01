# SPDX-License-Identifier: GPL-3.0-only
"""Extract only the verified owner XBE and regenerate from reviewed analysis."""
from pathlib import Path
import argparse, hashlib, os, shutil, subprocess, sys, tempfile
ROOT=Path(__file__).resolve().parents[1]
EXPECTED='16f9e41ce26c315a758f25d43ba1fa9d3809d1ad'

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--iso',type=Path,required=True)
    p.add_argument('--host-bin',type=Path,default=ROOT/'out/host/bin')
    a=p.parse_args(); suffix='.exe' if os.name=='nt' else ''
    extractor=a.host_bin/('iso_xbe'+suffix); generator=a.host_bin/('xbe_recomp'+suffix)
    for tool in (extractor,generator):
        if not tool.is_file():p.error('Build the host tools first: '+str(tool))
    iso=a.iso.resolve(strict=True)
    if not iso.is_file():p.error('Expected a regular owner ISO file')
    stage=ROOT/'out/owner-preparation'; stage.mkdir(parents=True,exist_ok=True)
    with tempfile.TemporaryDirectory(dir=stage) as temporary:
        xbe=Path(temporary)/'default.xbe'
        subprocess.run([str(extractor.resolve()),str(iso),str(xbe)],check=True,timeout=120)
        if hashlib.sha1(xbe.read_bytes()).hexdigest()!=EXPECTED:
            raise SystemExit('Unsupported XBE SHA-1; existing assets were not changed')
        destination=ROOT/'assets/default.xbe'; destination.parent.mkdir(exist_ok=True)
        if destination.exists():
            if destination.read_bytes()!=xbe.read_bytes():
                raise SystemExit('Different assets/default.xbe exists; refusing to overwrite')
        else:
            with destination.open('xb') as output, xbe.open('rb') as source:
                shutil.copyfileobj(source,output)
    subprocess.run([sys.executable,str(ROOT/'tools/recompile.py'),
                    '--tool',str(generator.resolve())],check=True,cwd=ROOT)
    print('Verified owner XBE and reviewed recompilation ready; no ISO/console modification.')

if __name__=='__main__':main()
