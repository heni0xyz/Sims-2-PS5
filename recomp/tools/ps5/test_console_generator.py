#!/usr/bin/env python3
# Copyright (C) 2026 OpenAI Codex, for szampan
# SPDX-License-Identifier: GPL-3.0-only
"""Check in-process generator output against the released builder's hashes.

Private generated files remain under ignored out/. This is host-only evidence,
not evidence of executing the generator on a console.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import time

ROOT=Path(__file__).resolve().parents[2]
EXPECTED_XBE='16f9e41ce26c315a758f25d43ba1fa9d3809d1ad'


def digest(path,algorithm='sha256'):
    with path.open('rb') as stream:return hashlib.file_digest(stream,algorithm).hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--kit',type=Path,required=True)
    parser.add_argument('--executable',type=Path,required=True)
    parser.add_argument('--clang-bin',type=Path,required=True)
    parser.add_argument('--xbe',type=Path,default=ROOT/'assets/default.xbe')
    parser.add_argument('--output',type=Path,default=ROOT/'out/console-compiler/generator-parity')
    args=parser.parse_args();out=args.output.resolve();out.mkdir(parents=True,exist_ok=False)
    manifest=json.loads((args.kit/'kit.json').read_text())
    result=dict(schema=1,status='RUNNING',generator_runs_on='Windows',console='NOT_TESTED')
    started=time.monotonic()
    env=dict(os.environ,PATH=str(args.clang_bin.resolve())+os.pathsep+os.environ.get('PATH',''))
    try:
        if digest(args.xbe,'sha1')!=EXPECTED_XBE:raise RuntimeError('Unsupported XBE SHA-1')
        output=out/'generated'
        command=[str(args.executable.resolve()),str((ROOT/'config/game.toml').resolve()),
                 str(args.xbe.resolve()),str(output),str((ROOT/'sdk/xdk').resolve())]
        with (out/'generate.log').open('wb') as log:
            subprocess.run(command,env=env,stdout=log,stderr=subprocess.STDOUT,check=True,timeout=300)
        actual={p.relative_to(output).as_posix():digest(p) for p in output.rglob('*') if p.is_file()}
        expected=manifest['generated_files']
        changed=sorted(name for name in actual.keys()&expected.keys() if actual[name]!=expected[name])
        result.update(files=len(actual),missing=sorted(expected.keys()-actual.keys()),
                      extra=sorted(actual.keys()-expected.keys()),changed=changed)
        if actual!=expected:raise RuntimeError('In-process generation differs from reviewed builder output')
        with (out/'overwrite-refusal.log').open('wb') as log:
            refused=subprocess.run(command,env=env,stdout=log,stderr=subprocess.STDOUT,timeout=30)
        if refused.returncode!=2:raise RuntimeError('Existing output was not refused')
        after={p.relative_to(output).as_posix():digest(p) for p in output.rglob('*') if p.is_file()}
        if after!=actual:raise RuntimeError('Existing generated files were modified')
        result.update(status='PASS',overwrite_refused=True)
    except Exception as error:result.update(status='FAIL',error=str(error))
    result['seconds']=round(time.monotonic()-started,3)
    (out/'result.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result))
    if result['status']!='PASS':raise SystemExit(1)


if __name__=='__main__':main()
