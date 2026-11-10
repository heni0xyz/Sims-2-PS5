# SPDX-License-Identifier: GPL-3.0-only
"""Prepare a local, committed source snapshot; never push or publish anything.

Inspect only HEAD trees, then export without Git history, owner data or native
game binaries. This inventory is a technical exclusion check, not permission
to distribute translated game code or third-party material.
"""
from pathlib import Path,PurePosixPath
import argparse,gzip,hashlib,io,json,subprocess,tarfile
ROOT=Path(__file__).resolve().parents[1]
DENIED={'.iso','.xbe','.arc','.xmv','.prx','.elf','.o','.obj','.a','.lib','.exe','.dll','.bin','.dmp','.log'}

def inspect(name,data):
    p=PurePosixPath(name)
    if not name or '\\' in name or ':' in name or p.is_absolute() or any(x in ('..','') for x in p.parts):raise ValueError('Unsafe source path: '+name)
    if p.name.casefold() in {'.env','id_rsa','id_ed25519','known_hosts','authorized_keys'} or p.suffix.casefold() in {'.pem','.key','.pfx'}:
        raise ValueError('Credential/config path: '+name)
    if any(x.casefold() in {'private','out','generated','hdd','saves','.git'} for x in p.parts):
        raise ValueError('Private/generated path: '+name)
    if 'assets' in p.parts and p.name!='README.md':raise ValueError('Game asset path: '+name)
    if p.suffix.casefold() in DENIED:raise ValueError('Binary/game data extension: '+name)
    if data[:2]==b'MZ' or data[:4] in (b'\x7fELF',b'XBEH'):raise ValueError('Native/game executable content: '+name)
    if len(data)>8*1024*1024:raise ValueError('Unexpected large source member: '+name)

def prepare(git,foundation,output):
    components={'project':ROOT,'sdk':ROOT/'sdk','runtime-sdk':ROOT/'runtime-sdk','ps5-foundation':foundation.resolve()}
    records={};archives={}
    prefixes={'project':'sims2-independent','sdk':'sims2-independent/sdk','runtime-sdk':'sims2-independent/runtime-sdk','ps5-foundation':'PS5_Sims2Independent'}
    for name,repo in components.items():
        def command(*args):return subprocess.check_output([str(git),'-C',str(repo),*args],stderr=subprocess.PIPE)
        if command('status','--porcelain').strip():raise ValueError('Commit local source changes first: '+name)
        revision=command('rev-parse','HEAD').decode().strip()
        blob=command('archive','--format=tar','HEAD');files={};omitted=[]
        with tarfile.open(fileobj=io.BytesIO(blob)) as archive:
            for item in archive:
                if item.isdir():continue
                if not item.isfile():raise ValueError('Linked/special source member: '+item.name)
                if name=='ps5-foundation' and item.name.startswith('ps5/screenshots/'):
                    omitted.append(item.name);continue # optional upstream demonstration media
                data=archive.extractfile(item).read();inspect(item.name,data)
                files[item.name]={'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest()}
        if 'LICENSE' not in files and not any(x.startswith('LICENSE.') for x in files):raise ValueError('Missing component license: '+name)
        records[name]={'revision':revision,'files':files,'omitted_optional_demo_media':omitted,'archive_prefix':prefixes[name]};archives[name]=blob
    lock=json.loads((ROOT/'repositories.lock.json').read_text())
    for name,pin in (('sdk',lock['sdk']['revision']),('runtime-sdk',lock['runtime_sdk']['revision']),
                     ('ps5-foundation',lock['ps5_foundation']['local_revision'])):
        if records[name]['revision']!=pin:raise ValueError('Source revision differs from repositories.lock.json: '+name)
    output.mkdir(parents=True,exist_ok=False)
    destination=output/'The-Sims-2-source.tar.gz'
    # Stable archive metadata allows independently repeated exports of the
    # same committed trees to have the same SHA-256, including the gzip header.
    with destination.open('wb') as raw, gzip.GzipFile(filename='',mode='wb',fileobj=raw,mtime=0) as compressed, tarfile.open(fileobj=compressed,mode='w') as result:
        for name,blob in archives.items():
            with tarfile.open(fileobj=io.BytesIO(blob)) as source:
                for item in source:
                    if not item.isfile():continue
                    if item.name not in records[name]['files']:continue
                    data=source.extractfile(item).read();entry=tarfile.TarInfo(prefixes[name]+'/'+item.name)
                    entry.size=len(data);entry.mode=item.mode&0o777
                    result.addfile(entry,io.BytesIO(data))
    report={'schema':1,'state':'LOCAL_SOURCE_PREPARED_NOT_PUBLISHED','owner_game_content_included':False,
            'translated_game_binary_distribution':'REQUIRES_SEPARATE_REVIEW','components':records,
            'archive':{'name':destination.name,'bytes':destination.stat().st_size,'sha256':hashlib.sha256(destination.read_bytes()).hexdigest()}}
    (output/'source-manifest.json').write_text(json.dumps(report,indent=2)+'\n')
    from verify_source_release import verify
    verify(destination,output/'source-manifest.json')
    return {'state':report['state'],'files':sum(len(x['files']) for x in records.values()),'archive':report['archive']}

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--git',type=Path,required=True)
    p.add_argument('--foundation',type=Path,default=ROOT.parent/'PS5_Sims2Independent')
    p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    print(json.dumps(prepare(a.git.resolve(),a.foundation,a.output.resolve()),indent=2))
