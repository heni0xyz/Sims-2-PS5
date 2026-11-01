"""Compile the authored aspect policy regression; no game assets or GPU needed."""
from pathlib import Path
import argparse,os,subprocess,json
ROOT=Path(__file__).resolve().parents[1]
def main():
    p=argparse.ArgumentParser();p.add_argument('--clang',type=Path,required=True);a=p.parse_args()
    env=dict(os.environ,PATH=str(a.clang.resolve().parent)+os.pathsep+os.environ['PATH'])
    for name,flags,sources in (
        ('ui-policy',[],['tests/ps5/ui_aspect_test.cpp']),
        ('render-resolution',[],['tests/render_resolution.cpp']),
        ('texture-replacement',[],['tests/texture_replacement.cpp']),
        ('font-resample',[],['tests/font_resample.cpp','src/game/font_resample.cpp']),
        ('menu-diagnostics',[],['tests/menu_diagnostics.cpp','src/game/menu_diagnostics.cpp']),
        ('ui-layout-test',['-DSIMS2_UI_LAYOUT_UNIT_TEST=1'],['tests/ui_layout.cpp','src/game/ui_layout.cpp'])):
        out=ROOT/'out'/name;out.mkdir(parents=True,exist_ok=True)
        exe=out/('policy.exe' if os.name=='nt' else 'policy')
        command=[str(a.clang.resolve()),'-std=c++20','-O2','-I'+str(ROOT/'src'),
                 '-I'+str(ROOT/'runtime-sdk/runtime'),*flags,*[str(ROOT/s) for s in sources],'-o',str(exe)]
        with (out/'compile.log').open('wb') as log:
            result=subprocess.run(command,env=env,stdout=log,stderr=subprocess.STDOUT,timeout=120)
        if result.returncode:print((out/'compile.log').read_text(errors='replace'));return result.returncode
        result=subprocess.run([str(exe)],env=env,capture_output=True,text=True,timeout=15)
        (out/'run.log').write_text(result.stdout+result.stderr)
        (out/'summary.json').write_text(json.dumps({'returncode':result.returncode,'scope':'authored UI policy/group/clip-space regression only, not game/console acceptance'},indent=2)+'\n')
        print(result.stdout+result.stderr,end='')
        if result.returncode:return result.returncode
    return 0
if __name__=='__main__':raise SystemExit(main())
