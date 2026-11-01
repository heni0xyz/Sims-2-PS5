"""Compile a real ELF/TLS fixture from a path with spaces and parentheses."""
from pathlib import Path
import argparse,json,os,subprocess
ROOT=Path(__file__).resolve().parents[2]
def main():
    parser=argparse.ArgumentParser();parser.add_argument('--sdk',type=Path,required=True)
    parser.add_argument('--clang-bin',type=Path,required=True);args=parser.parse_args()
    folder=ROOT/'out/host-driver-fix/quote (case)';folder.mkdir(parents=True,exist_ok=True)
    (folder/'probe.h').write_text('#define PROBE_VALUE 7\n')
    (folder/'probe.cpp').write_text('#include "probe.h"\nthread_local int value=PROBE_VALUE;\nextern "C" int probe(){return value;}\n')
    wrapper=args.sdk/'win/prospero-clang++.cmd'
    command=f'cmd.exe /d /s /c ""{wrapper}" -I"{folder}" -O1 -fPIC -c "{folder}/probe.cpp" -o "{folder}/probe.o""'
    env=dict(os.environ,PATH=str(args.clang_bin)+os.pathsep+os.environ['PATH'])
    result=subprocess.run(command,cwd=folder,env=env,capture_output=True,text=True,timeout=60)
    (folder/'compiler.log').write_text(result.stdout+result.stderr)
    obj=folder/'probe.o';data=obj.read_bytes() if obj.exists() else b''
    valid=result.returncode==0 and data[:6]==b'\x7fELF\x02\x01' and data[18:20]==b'\x3e\x00' and b'__emutls_v.value' in data
    report={'status':'PASS' if valid else 'FAIL','returncode':result.returncode,'elf64_x86_64':data[:6]==b'\x7fELF\x02\x01',
            'emulated_tls_symbol':b'__emutls_v.value' in data,'spaces_and_parentheses':True}
    (folder/'summary.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
    if not valid:print(result.stdout+result.stderr)
    return 0 if valid else 1
if __name__=='__main__':raise SystemExit(main())
