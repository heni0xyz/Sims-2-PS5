"""Validate required owner data before launching; never fetch game content."""
from pathlib import Path
import argparse, hashlib, json

XBE_SHA1 = '16f9e41ce26c315a758f25d43ba1fa9d3809d1ad'
DSP_SHA256 = '5d8512e350d3de422cc0e86c6879f995da6d59c2153b3b4e94e360ab54ee6284'
REQUIRED = ('default.xbe', 'default.NGH', 'sims2.bin',
            'DATA/index.ind', 'DATA/index1.ind', 'DATA/datasets.arc',
            'DATA/textures.arc', 'DATA/shaders.arc', 'movies/ea_games.xmv')

def member(root, relative):
    current = root
    for name in relative.split('/'):
        if not current.is_dir():
            raise ValueError('Missing game member: ' + relative)
        matches = [p for p in current.iterdir() if p.name.casefold() == name.casefold()]
        if len(matches) != 1:
            raise ValueError('Missing or ambiguous game member: ' + relative)
        current = matches[0]
    if not current.is_file() or current.stat().st_size == 0:
        raise ValueError('Empty or invalid game member: ' + relative)
    return current

def validate(root):
    root = Path(root).resolve()
    files = {name: member(root, name) for name in REQUIRED}
    if hashlib.sha1(files['default.xbe'].read_bytes()).hexdigest() != XBE_SHA1:
        raise ValueError('Unsupported XBE revision; expected USA retail SHA-1 ' + XBE_SHA1)
    if hashlib.sha256(files['sims2.bin'].read_bytes()).hexdigest() != DSP_SHA256:
        raise ValueError('DSP data differs from the verified owner USA disc')
    return {'status': 'PASS_REQUIRED_ASSETS', 'xbe_sha1': XBE_SHA1,
            'dsp_sha256': DSP_SHA256,
            'required_bytes': {name: path.stat().st_size for name, path in files.items()},
            'scope': 'Required members and XBE/DSP hashes; not full-disc integrity or gameplay proof'}

def verify_inventory(root, inventory):
    files=inventory.get('files',{})
    if not files:raise ValueError('Empty asset inventory')
    total=0
    for name,expected in files.items():
        if '\\' in name or any(part in ('','.','..') or ':' in part for part in name.split('/')):
            raise ValueError('Unsafe inventory member')
        path=member(Path(root),name)
        if path.stat().st_size!=expected['bytes']:
            raise ValueError('Asset length mismatch: '+name)
        with path.open('rb') as stream:sha=hashlib.file_digest(stream,'sha256').hexdigest()
        if sha!=expected['sha256']:raise ValueError('Asset SHA-256 mismatch: '+name)
        total+=expected['bytes']
    return {'status':'PASS_OWNER_INVENTORY','files':len(files),'bytes':total,
            'scope':'Matches the supplied owner-disc inventory; not a gameplay test'}

def main():
    p = argparse.ArgumentParser()
    p.add_argument('directory', type=Path)
    p.add_argument('--manifest',type=Path,help='Verify every file against an owner-disc extraction inventory')
    args = p.parse_args()
    try:
        report=validate(args.directory)
        if args.manifest:report['full_inventory']=verify_inventory(args.directory,json.loads(args.manifest.read_text()))
        print(json.dumps(report, indent=2))
    except (OSError, ValueError) as error:
        print(json.dumps({'status': 'FAIL', 'error': str(error)}))
        return 1
    return 0

if __name__ == '__main__':
    raise SystemExit(main())
