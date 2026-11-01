# SPDX-License-Identifier: GPL-3.0-only
"""Verify a notice-only exe repack keeps tested Python/native runtime intact."""
from pathlib import Path
import argparse
import hashlib
import json
import io
import zipfile
import pefile
from PyInstaller.archive.readers import CArchiveReader

def digest(data):return hashlib.sha256(data).hexdigest()

def compare(before,after):
    a,b=CArchiveReader(str(before)),CArchiveReader(str(after))
    def normalized(reader):return {name.replace('\\','/'):name for name in reader.toc}
    an,bn=normalized(a),normalized(b)
    am=json.loads(a.extract(an['kit/kit.json']));bm=json.loads(b.extract(bn['kit/kit.json']))
    for key in set(am)|set(bm):
        if key!='files' and am.get(key)!=bm.get(key):raise ValueError('Pipeline metadata changed: '+key)
    changes=[n for n in sorted(set(am['files'])|set(bm['files'])) if am['files'].get(n)!=bm['files'].get(n)]
    allowed={'title/LEGAL.txt','title/SOURCES.txt','title/CREDITS.txt','title/licenses/components.json','SOURCES.txt'}
    for name in changes:
        if name not in allowed and not name.startswith('licenses/'):
            raise ValueError('Executable, compile input, settings or resource changed: '+name)
    outer_a={n for n in an if not n.startswith('kit/')}
    outer_b={n for n in bn if not n.startswith('kit/')}
    if outer_a!=outer_b:raise ValueError('Frozen interpreter/application inventory changed')
    normalized_zip_metadata=[]
    for name in sorted(outer_a):
        before_data,after_data=a.extract(an[name]),b.extract(bn[name])
        if before_data==after_data:continue
        if name=='base_library.zip':
            # PyInstaller stamps a newly created ZIP with the build time.
            # Verify every uncompressed module byte, never ignore the archive.
            with zipfile.ZipFile(io.BytesIO(before_data)) as za,zipfile.ZipFile(io.BytesIO(after_data)) as zb:
                if sorted(za.namelist())!=sorted(zb.namelist()):raise ValueError('Standard-library inventory changed')
                for member in za.namelist():
                    if za.read(member)!=zb.read(member):raise ValueError('Standard-library module changed: '+member)
            normalized_zip_metadata.append(name)
            continue
        raise ValueError('Frozen runtime changed: '+name)
    def text(path):
        pe=pefile.PE(str(path))
        return b''.join(s.get_data() for s in pe.sections if s.Name.rstrip(b'\0')==b'.text')
    if text(before)!=text(after):raise ValueError('Native bootloader code changed')
    return dict(status='PASS_NOTICE_ONLY_REPACK',changed_kit_files=changes,
                frozen_runtime_entries_compared=len(outer_a),
                pyz_sha256=digest(b.extract(bn['PYZ.pyz'])),
                zip_metadata_only_differences=normalized_zip_metadata,
                native_bootloader_text_sha256=digest(text(after)),
                scope='Frozen application, interpreter and all tools/link/compile inputs unchanged; notices only')

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('before',type=Path);p.add_argument('after',type=Path);p.add_argument('--report',type=Path,required=True)
    a=p.parse_args();result=compare(a.before,a.after)
    a.report.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
