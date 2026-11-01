# SPDX-License-Identifier: GPL-3.0-only
"""Package the GUI on its actual host; never cross-claims a macOS build."""
from pathlib import Path
import argparse
import os
import subprocess
import sys
import importlib.metadata
import json
import shutil

ROOT=Path(__file__).resolve().parents[2]

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--output',required=True,type=Path)
    p.add_argument('--kit',type=Path,help='Embed an audited public kit in a single Windows exe')
    p.add_argument('--portable',action='store_true',help='Folder distribution: copy the kit beside a small exe; no per-launch extraction')
    a=p.parse_args()
    if a.kit:
        manifest=json.loads((a.kit/'kit.json').read_text())
        required={'title/SOURCES.txt','licenses/Zydis-MIT.txt','licenses/Zycore-MIT.txt',
                  'licenses/tomlplusplus-MIT.txt','licenses/zlib.txt','licenses/CPython.txt',
                  'licenses/PyInstaller-with-bootloader-exception.txt'}
        components=json.loads((a.kit/'title/licenses/components.json').read_text())
        if not required<=manifest['files'].keys() or not any(
                x.get('id')=='title' and x.get('licence','').startswith('GPL-3.0-only') for x in components):
            raise ValueError('Run prepare_public_notices.py before packaging the public kit')
    env=dict(os.environ,PYINSTALLER_CONFIG_DIR=str(ROOT/'out/pyinstaller-config'))
    if a.portable and not a.kit:raise ValueError('--portable requires --kit')
    options=['--onefile','--add-data',str(a.kit.resolve())+os.pathsep+'kit'] if a.kit and not a.portable else ['--onedir']
    python_license=Path(sys.prefix)/'LICENSE.txt'
    pyinstaller_license=next(x.locate() for x in importlib.metadata.files('pyinstaller')
                            if str(x).endswith('licenses/COPYING.txt'))
    for file in (python_license,pyinstaller_license):
        if not file.is_file():raise FileNotFoundError('Missing packaged dependency license: '+str(file))
        options+=['--add-data',str(file)+os.pathsep+'licenses']
    subprocess.run([sys.executable,'-m','PyInstaller','--noconfirm',*options,'--windowed',
                    '--name','The Sims 2 Builder','--paths',str(ROOT),
                    '--distpath',str(a.output.resolve()),
                    '--workpath',str(ROOT/'out/builder-pyinstaller'),
                    '--specpath',str(ROOT/'out/builder-pyinstaller'),
                    str(ROOT/'builder/entry.py')],cwd=ROOT,env=env,check=True)
    if a.portable:
        target=a.output.resolve()/'The Sims 2 Builder/kit'
        shutil.copytree(a.kit.resolve(),target)
