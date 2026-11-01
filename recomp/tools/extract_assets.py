"""Prepare a complete private owner-disc directory with a hashed inventory."""
from pathlib import Path
import argparse,hashlib,json,subprocess,time
from check_assets import validate
ROOT=Path(__file__).resolve().parents[1]
def main():
    p=argparse.ArgumentParser()
    p.add_argument('--iso',type=Path,required=True)
    p.add_argument('--output',type=Path,default=ROOT/'private/owner-assets')
    p.add_argument('--extractor',type=Path,default=ROOT/('out/host/bin/iso_assets.exe'))
    a=p.parse_args()
    output=a.output.resolve();iso=a.iso.resolve()
    if output.exists():p.error('Output already exists; existing data will not be overwritten')
    if not iso.is_file():p.error('Supply the owner ISO file')
    start=time.monotonic()
    result=subprocess.run([str(a.extractor.resolve()),str(iso),str(output)],cwd=ROOT)
    if result.returncode:raise SystemExit('Extraction failed; partial output is not accepted')
    check=validate(output)
    files={}
    for path in sorted(output.rglob('*')):
        if path.is_file():
            with path.open('rb') as stream:sha=hashlib.file_digest(stream,'sha256').hexdigest()
            files[path.relative_to(output).as_posix()]={'bytes':path.stat().st_size,'sha256':sha}
    report={'schema':1,'source':'owner-supplied disc; no downloaded game data',
            'iso_bytes':iso.stat().st_size,'asset_check':check,'files':files,
            'elapsed_seconds':time.monotonic()-start}
    (output/'owner-asset-manifest.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({'status':'PASS_COMPLETE_EXTRACTION','files':len(files),
          'bytes':sum(item['bytes'] for item in files.values()),'seconds':report['elapsed_seconds']}))
if __name__=='__main__':main()
