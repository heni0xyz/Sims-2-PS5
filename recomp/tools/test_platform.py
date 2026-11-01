"""Build and execute the asset-free native platform boundary tests."""
from pathlib import Path
import argparse,json,os,subprocess,time
ROOT=Path(__file__).resolve().parents[1]
def main():
    p=argparse.ArgumentParser();p.add_argument('--clang',type=Path,required=True)
    a=p.parse_args();out=ROOT/'out/platform-contracts';out.mkdir(parents=True,exist_ok=True)
    exe=out/('contracts.exe' if os.name=='nt' else 'contracts')
    command=[str(a.clang.resolve()),'-std=c++20','-O2','-fno-strict-aliasing',
             '-I'+str(ROOT/'platform/ps5'),'-I'+str(ROOT/'runtime-sdk/runtime'),
             str(ROOT/'tests/platform_contracts.cpp'),str(ROOT/'platform/ps5/contracts.cpp'),'-o',str(exe)]
    env=dict(os.environ);env['PATH']=str(a.clang.resolve().parent)+os.pathsep+env.get('PATH','')
    start=time.monotonic()
    with (out/'compile.log').open('wb') as log:
        result=subprocess.run(command,env=env,stdout=log,stderr=subprocess.STDOUT,timeout=120)
    if result.returncode:
        print((out/'compile.log').read_text(errors='replace'));return result.returncode
    result=subprocess.run([str(exe)],env=env,text=True,capture_output=True,timeout=15)
    (out/'run.log').write_text(result.stdout+result.stderr)
    print(result.stdout+result.stderr,end='')
    report={'returncode':result.returncode,'seconds':time.monotonic()-start,'command':command,
            'scope':'host-only compatibility tests; allocation, Vulkan and hardware tests remain separate'}
    (out/'summary.json').write_text(json.dumps(report,indent=2)+'\n')
    return result.returncode
if __name__=='__main__':raise SystemExit(main())
