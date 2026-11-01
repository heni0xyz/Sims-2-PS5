"""Fresh analysis from the owner's ISO; never reads the existing game repo."""
from pathlib import Path
import argparse,hashlib,json,subprocess,time
ROOT=Path(__file__).resolve().parents[1]
EXPECTED='16f9e41ce26c315a758f25d43ba1fa9d3809d1ad'
def main():
    p=argparse.ArgumentParser();p.add_argument('--iso',type=Path,required=True)
    p.add_argument('--tools',type=Path,default=ROOT/'out/host/bin');a=p.parse_args()
    tools=a.tools.resolve();iso=a.iso.resolve();assets=ROOT/'assets';assets.mkdir(exist_ok=True)
    out=ROOT/'out/analysis';out.mkdir(parents=True,exist_ok=True)
    xbe=assets/'default.xbe'
    suffix='.exe' if (tools/'iso_xbe.exe').exists() else ''
    def run(name,args):
        start=time.monotonic()
        with (out/(name+'.log')).open('wb') as log:
            r=subprocess.run(args,cwd=ROOT,stdout=log,stderr=subprocess.STDOUT,timeout=1200)
        print(f'{name}: exit={r.returncode}, seconds={time.monotonic()-start:.2f}',flush=True)
        if r.returncode:raise RuntimeError(f'{name} failed; inspect out/analysis/{name}.log')
    if not xbe.exists():run('extract',[str(tools/('iso_xbe'+suffix)),str(iso),str(xbe)])
    digest=hashlib.sha1(xbe.read_bytes()).hexdigest()
    if digest!=EXPECTED:raise ValueError('XBE SHA-1 mismatch; no analysis of another revision')
    config=out/'game.auto.toml'
    run('symbols',[str(tools/('xbe_symbols'+suffix)),str(xbe),str(ROOT/'sdk/xdk'),
                   '--library','Sims2Independent.dll','--name','independent','--out',str(config)])
    generated=ROOT/'generated/independent';generated.mkdir(parents=True,exist_ok=True)
    run('recompile',[str(tools/('xbe_recomp'+suffix)),str(config),str(xbe),str(generated),'--xdk',str(ROOT/'sdk/xdk')])
    report={'source':'owner-supplied ISO; no existing Sims 2 project files read',
            'iso_bytes':iso.stat().st_size,'xbe_sha1':digest,'xbe_bytes':xbe.stat().st_size,
            'config_sha256':hashlib.sha256(config.read_bytes()).hexdigest(),
            'generated_files':len(list(generated.glob('*'))),'native_ps5_runtime':'NOT_IMPLEMENTED',
            'gameplay':'NOT_TESTED','generated_code_is_private':True}
    (out/'summary.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))
if __name__=='__main__':main()
