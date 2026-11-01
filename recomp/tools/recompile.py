"""Regenerate from the independent reviewed config, never modifying generated C++."""
from pathlib import Path
import argparse, hashlib, json, os, re, subprocess, time
ROOT=Path(__file__).resolve().parents[1]
EXPECTED='16f9e41ce26c315a758f25d43ba1fa9d3809d1ad'

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--config',type=Path,default=ROOT/'config/game.toml')
    parser.add_argument('--tool',type=Path,default=ROOT/'out/host/bin/xbe_recomp.exe')
    args=parser.parse_args()
    xbe=ROOT/'assets/default.xbe'
    digest=hashlib.sha1(xbe.read_bytes()).hexdigest()
    if digest!=EXPECTED:raise SystemExit('Unsupported XBE SHA-1; generation refused')
    out=ROOT/'out/analysis';out.mkdir(parents=True,exist_ok=True)
    generated=ROOT/'generated/independent'
    # Preserve timestamps only for byte-identical generator output. This keeps
    # compilation incremental without changing any generated source semantics.
    previous={path:(hashlib.sha256(path.read_bytes()).digest(),path.stat().st_atime_ns,path.stat().st_mtime_ns)
              for path in generated.glob('*') if path.is_file()}
    start=time.monotonic()
    with (out/'reviewed-recompile.log').open('wb') as log:
        result=subprocess.run([str(args.tool.resolve()),str(args.config.resolve()),str(xbe),
            str(ROOT/'generated/independent'),'--xdk',str(ROOT/'sdk/xdk')],
            cwd=ROOT,stdout=log,stderr=subprocess.STDOUT,timeout=1200)
    text=(out/'reviewed-recompile.log').read_text(errors='replace')
    unchanged=0
    if result.returncode==0:
        for path,(sha,atime,mtime) in previous.items():
            if path.is_file() and hashlib.sha256(path.read_bytes()).digest()==sha:
                os.utime(path,ns=(atime,mtime));unchanged+=1
    record={'returncode':result.returncode,'seconds':time.monotonic()-start,'xbe_sha1':digest,
            'config_sha256':hashlib.sha256(args.config.read_bytes()).hexdigest(),
            'generated_cpp':len(list((ROOT/'generated/independent').glob('*.cpp'))) if result.returncode==0 else None,
            'byte_identical_files_preserved':unchanged,
            'unsupported_instructions':dict(re.findall(r'unimplemented (\w+) (\d+)',text)),
            'runtime':'NOT_TESTED by this generation command'}
    (out/'reviewed-recompile.json').write_text(json.dumps(record,indent=2)+'\n')
    print(json.dumps(record));return result.returncode

if __name__=='__main__':raise SystemExit(main())
