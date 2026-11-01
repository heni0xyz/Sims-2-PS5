"""Exercise SDK P8 colour/alpha lookup and cache invalidation without assets."""
from pathlib import Path
import argparse,json,os,subprocess
ROOT=Path(__file__).resolve().parents[1]

def main():
    p=argparse.ArgumentParser();p.add_argument('--clang',type=Path,required=True);a=p.parse_args()
    out=ROOT/'out/reference-palette';out.mkdir(parents=True,exist_ok=True)
    rt=ROOT/'runtime-sdk/runtime';exe=out/('palette.exe' if os.name=='nt' else 'palette')
    env=dict(os.environ);env['PATH']=str(a.clang.resolve().parent)+os.pathsep+env.get('PATH','')
    cmd=[str(a.clang.resolve()),'-std=c++20','-O1','-ffunction-sections','-fdata-sections',
         '-fms-extensions','-I'+str(rt),'-I'+str(ROOT/'runtime-sdk/harness'),
         str(ROOT/'tests/reference_palette.cpp'),str(rt/'gpu/texture.cpp'),
         str(rt/'gpu/shaders.cpp'),str(rt/'gpu/vertex.cpp'),str(rt/'core/log.cpp'),
         '-Wl,--gc-sections','-o',str(exe)]
    with (out/'compile.log').open('wb') as stream:
        compiled=subprocess.run(cmd,env=env,stdout=stream,stderr=subprocess.STDOUT,timeout=180)
    if compiled.returncode:
        (out/'summary.json').write_text(json.dumps({'returncode':compiled.returncode,
            'stage':'compile/link','command':cmd},indent=2)+'\n')
        print((out/'compile.log').read_text(errors='replace'));return compiled.returncode
    result=subprocess.run([str(exe)],env=env,text=True,capture_output=True,timeout=15)
    (out/'run.log').write_text(result.stdout+result.stderr)
    (out/'summary.json').write_text(json.dumps({'returncode':result.returncode,
        'scope':'asset-free CPU reference palette colour/alpha and cache regression',
        'command':cmd},indent=2)+'\n')
    print(result.stdout+result.stderr,end='');return result.returncode

if __name__=='__main__':raise SystemExit(main())
