# SPDX-License-Identifier: GPL-3.0-only
"""Asset-free extraction contracts; all fixture writes stay in a new workspace dir."""
from pathlib import Path
import argparse,datetime,json,os,subprocess
ROOT=Path(__file__).resolve().parents[1]
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--clang',type=Path,required=True);a=p.parse_args()
    out=ROOT/'out/install-tests'/datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%S.%fZ');out.mkdir(parents=True)
    environment=dict(os.environ,PATH=str(a.clang.resolve().parent)+os.pathsep+os.environ['PATH'])
    import sys
    sys.path.insert(0,str(ROOT/'tools/ps5'))
    from build_installer_font import build
    build(out/'font')
    sources=['tests/install.cpp','src/game/install.cpp','src/game/install_notices.cpp','src/game/install_screen.cpp','src/game/install_font.cpp','src/game/disc_boot.cpp','runtime-sdk/runtime/kernel/disc.cpp','runtime-sdk/runtime/kernel/crypto.cpp']
    exe=out/('test.exe' if os.name=='nt' else 'test')
    command=[str(a.clang.resolve()),'-std=c++20','-O2','-I'+str(ROOT/'src'),'-I'+str(ROOT/'runtime-sdk/runtime'),*[str(ROOT/s) for s in sources],'-o',str(exe)]
    build=subprocess.run(command,env=environment,capture_output=True,text=True,timeout=120);(out/'compile.log').write_text(build.stdout+build.stderr)
    if build.returncode:print(build.stdout+build.stderr);raise SystemExit(build.returncode)
    result=subprocess.run([str(exe),str(out),str(out/'font/installer-font.s2font')],env=environment,capture_output=True,text=True,timeout=60)
    (out/'run.log').write_text(result.stdout+result.stderr);print(result.stdout+result.stderr,end='')
    (out/'summary.json').write_text(json.dumps({'returncode':result.returncode,'scope':'Synthetic extraction transaction; no game data or console'},indent=2)+'\n')
    raise SystemExit(result.returncode)
