"""Verify a downloaded native title against its manifest before extraction."""
from pathlib import Path,PurePosixPath
import argparse,hashlib,json,zipfile

def main():
    p=argparse.ArgumentParser();p.add_argument('directory',type=Path);p.add_argument('--extract',action='store_true');a=p.parse_args()
    directory=a.directory.resolve()
    manifest=json.loads((directory/'build-manifest.json').read_text())
    status=json.loads((directory/'status.json').read_text())
    if status['state']!='BUILD_PASS_RUNTIME_NOT_TESTED' or status['build_id']!=manifest['build_id']:
        raise ValueError('Build status/manifest mismatch')
    title=manifest['title_id'];archive=directory/('Sims2Independent-'+title+'.zip')
    with (directory/'llvm-pie.elf').open('rb') as stream:elf_hash=hashlib.file_digest(stream,'sha256').hexdigest()
    if elf_hash!=manifest['debug_elf_sha256']:raise ValueError('Debug ELF digest mismatch')
    forbidden={'.iso','.xbe','.arc','.xmv','.ngh'}
    with zipfile.ZipFile(archive) as bundle:
        files={}
        for entry in bundle.infolist():
            path=PurePosixPath(entry.filename)
            if path.is_absolute() or '..' in path.parts or not path.parts or path.parts[0]!=title or '\\' in entry.filename:
                raise ValueError('Unsafe package member')
            if entry.is_dir():continue
            name=path.relative_to(title).as_posix()
            private_root=path.parts[1].lower() in {'assets','data','movies','hdd','udata','tdata'}
            if name in files or path.suffix.lower() in forbidden or private_root or path.name.lower() in {'sims2.bin','savegame','savemeta.xbx'}:
                raise ValueError('Duplicate or owner-data package member')
            files[name]=entry
        if files.keys()!=manifest['files'].keys():raise ValueError('Package inventory differs from manifest')
        for name,entry in files.items():
            data=bundle.read(entry);record=manifest['files'][name]
            if len(data)!=record['bytes'] or hashlib.sha256(data).hexdigest()!=record['sha256']:
                raise ValueError('Package digest mismatch: '+name)
        if manifest['files']['eboot.bin']['sha256']!=manifest['eboot_sha256']:
            raise ValueError('eboot identity mismatch')
        destination=directory/title
        if a.extract:
            if destination.exists():raise ValueError('Extraction destination already exists')
            bundle.extractall(directory)
    with archive.open('rb') as stream:zip_hash=hashlib.file_digest(stream,'sha256').hexdigest()
    record={'status':'PASS_LOCAL_DELIVERY','build_id':manifest['build_id'],'title_id':title,
        'files_verified':len(files),'eboot_sha256':manifest['eboot_sha256'],
        'debug_elf_sha256':elf_hash,'zip_sha256':zip_hash,'owner_assets_included':False,
        'extracted':a.extract,'PS5_runtime':'NOT_TESTED','gameplay_performance':'NOT_TESTED'}
    (directory/'local-validation.json').write_text(json.dumps(record,indent=2)+'\n')
    print(json.dumps(record,indent=2))

if __name__=='__main__':main()
