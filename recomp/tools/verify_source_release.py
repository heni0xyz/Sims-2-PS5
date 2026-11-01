# SPDX-License-Identifier: GPL-3.0-only
"""Verify a local source snapshot against its complete external inventory.

Does not extract files, run code from the archive, publish, or certify game
binary distribution. Run before reviewing/uploading a source-only release.
"""
from pathlib import Path
import argparse,hashlib,json,tarfile
from prepare_source_release import inspect

def verify(archive,manifest):
    report=json.loads(manifest.read_text())
    if report.get('schema')!=1 or report.get('owner_game_content_included') is not False:
        raise ValueError('Unexpected source manifest')
    recorded=report['archive']
    with archive.open('rb') as stream:digest=hashlib.file_digest(stream,'sha256').hexdigest()
    if archive.stat().st_size!=recorded['bytes'] or digest!=recorded['sha256']:
        raise ValueError('Source archive digest mismatch')
    expected={}
    for component in report['components'].values():
        prefix=component['archive_prefix']
        inspect(prefix+'/LICENSE',b'')
        for name,record in component['files'].items():
            inspect(name,b'')
            target=prefix+'/'+name
            if target in expected:raise ValueError('Overlapping component inventory: '+target)
            expected[target]=record
    seen=set()
    with tarfile.open(archive) as source:
        for member in source:
            if not member.isfile() or member.name in seen:raise ValueError('Special/duplicate source member')
            if member.name not in expected:raise ValueError('Uninventoried source member: '+member.name)
            record=expected[member.name]
            if member.size!=record['bytes'] or member.size>8*1024*1024:raise ValueError('Source member size mismatch')
            data=source.extractfile(member).read()
            inspect(member.name,data)
            if hashlib.sha256(data).hexdigest()!=record['sha256']:raise ValueError('Source member digest mismatch: '+member.name)
            seen.add(member.name)
    if seen!=set(expected):raise ValueError('Missing source members')
    for required in ('sims2-independent/LICENSE','sims2-independent/CREDITS.txt',
                     'sims2-independent/SOURCE-RELEASE.md','sims2-independent/repositories.lock.json',
                     'sims2-independent/sdk/LICENSE','sims2-independent/runtime-sdk/LICENSE',
                     'sims2-independent/runtime-sdk/LOCAL_CHANGES_LICENSE.md','PS5_Sims2Independent/LICENSE.md'):
        if required not in seen:raise ValueError('Missing release component/notice: '+required)
    return {'status':'PASS_SOURCE_INVENTORY','files':len(seen),'archive_sha256':digest,
            'scope':'Source content/inventory only; no publication or PS5 runtime acceptance'}

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('archive',type=Path);p.add_argument('manifest',type=Path)
    a=p.parse_args();print(json.dumps(verify(a.archive,a.manifest),indent=2))
