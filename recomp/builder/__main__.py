# SPDX-License-Identifier: GPL-3.0-only
import argparse
from pathlib import Path
from .core import Builder, BuildError

def main():
    p=argparse.ArgumentParser(description='Prepare your own USA Xbox ISO for The Sims 2 on PS5')
    p.add_argument('--kit',type=Path,required=True)
    p.add_argument('--iso',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True,help='NEW folder; existing folders are refused')
    p.add_argument('--resume-work',type=Path,help='Retry a matching failed workspace with hash-checked objects')
    a=p.parse_args()
    try:
        Builder(a.kit,a.iso,a.output,lambda s,d: print(s+': '+d,flush=True),resume_work=a.resume_work).build()
    except (BuildError,OSError,ValueError) as error:
        print('BUILD FAILED: '+str(error));return 1
    return 0

if __name__=='__main__':raise SystemExit(main())
