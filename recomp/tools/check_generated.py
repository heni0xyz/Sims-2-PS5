"""Compile-check every freshly generated unit using the SDK code-DLL headers."""
from pathlib import Path
import argparse,concurrent.futures,hashlib,json,subprocess,time
ROOT=Path(__file__).resolve().parents[1]
def main():
    p=argparse.ArgumentParser();p.add_argument('--clang',type=Path,required=True)
    p.add_argument('--jobs',type=int,default=2)
    p.add_argument('--limit',type=int,help='Bounded probe before checking every unit');a=p.parse_args()
    generated=ROOT/'generated/independent';out=ROOT/'out/syntax';out.mkdir(parents=True,exist_ok=True)
    files=sorted(generated.glob('*.cpp'));assert files
    if a.limit:files=files[:a.limit]
    def check(source):
        command=[str(a.clang),'--target=x86_64-pc-windows-msvc','-fms-extensions','-std=c++20','-DX86_GAME_CODE','-fms-compatibility-version=19.40',
            '-fno-strict-aliasing','-nostdinc','-nostdinc++','-fsyntax-only',
            '-I',str(ROOT/'sdk/runtime'),'-I',str(generated),
            '-isystem',str(ROOT/'sdk/runtime/cpu/freestanding'),str(source)]
        start=time.monotonic()
        with (out/(source.name+'.log')).open('wb') as log:
            result=subprocess.run(command,cwd=ROOT,stdout=log,stderr=subprocess.STDOUT,timeout=300)
        return {'file':source.name,'returncode':result.returncode,'seconds':time.monotonic()-start,
                'sha256':hashlib.sha256(source.read_bytes()).hexdigest()}
    with concurrent.futures.ThreadPoolExecutor(max_workers=max(1,min(a.jobs,4))) as pool:
        rows=list(pool.map(check,files))
    report={'status':'PASS' if all(v['returncode']==0 for v in rows) else 'FAIL',
            'scope':'Windows host syntax checks only; no PS5 link or runtime proof','units':rows}
    (out/'summary.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({'status':report['status'],'units':len(rows),
                      'failed':[v['file'] for v in rows if v['returncode']]}))
    return 0 if report['status']=='PASS' else 1
if __name__=='__main__':raise SystemExit(main())
