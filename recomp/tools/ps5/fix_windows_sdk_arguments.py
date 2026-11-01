"""Repair quoted argument inspection in the installed public Windows SDK driver.

The forwarded command line and SDK compiler/link flags stay unchanged. Originals
and before/after hashes are retained under out/host-driver-fix for review/revert.
"""
from pathlib import Path
import argparse,hashlib,json
ROOT=Path(__file__).resolve().parents[2]
def main():
    parser=argparse.ArgumentParser();parser.add_argument('sdk',type=Path);args=parser.parse_args()
    out=ROOT/'out/host-driver-fix';out.mkdir(parents=True,exist_ok=True)
    records=[]
    for name in ('prospero-clang.cmd','prospero-clang++.cmd'):
        path=args.sdk/'win'/name;data=path.read_bytes();text=data.decode('utf-8')
        normalized=text.replace('\r\n','\n')
        marker='rem Inspect a dequoted copy; forward the original command line unchanged.'
        backup=out/(name+'.original')
        if backup.exists():
            prior=json.loads((out/'manifest.json').read_text())
            expected=next(item['after_sha256'] for item in prior if item['file']==name)
            if hashlib.sha256(data).hexdigest()!=expected:
                raise SystemExit('Installed driver changed since the recorded fix; refusing to overwrite it')
            data=backup.read_bytes();normalized=data.decode('utf-8').replace('\r\n','\n')
        assert ':checkargs\n' in normalized and 'if "%1"=="" goto runcmd' in normalized
        fixed=normalized.replace(':checkargs\n',':checkargs\n'+marker+'\n    set "ARG=%~1"\n    if not defined ARG goto runcmd\n    set "ARG=%ARG:"=%"\n')
        fixed=fixed.replace('"%1"','"%ARG%"').replace('"%1:~0,1%"','"%ARG:~0,1%"')
        fixed=fixed.replace('if "%TU%"=="" (','if not defined TU (')
        assert '%*' in fixed and '-femulated-tls' in fixed and '-target x86_64-sie-ps5' in fixed
        if not backup.exists():backup.write_bytes(data)
        result=fixed.replace('\n','\r\n').encode('utf-8');path.write_bytes(result)
        records.append({'file':name,'before_sha256':hashlib.sha256(data).hexdigest(),
                        'after_sha256':hashlib.sha256(result).hexdigest(),
                        'change':'argument inspection only; original compiler arguments forwarded unchanged'})
    (out/'manifest.json').write_text(json.dumps(records,indent=2)+'\n')
    print(json.dumps({'files_repaired':len(records),'originals_retained':True}))
if __name__=='__main__':main()
