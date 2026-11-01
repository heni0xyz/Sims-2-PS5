# SPDX-License-Identifier: GPL-3.0-only
import hashlib,io,json,sys,tarfile,tempfile,unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[2]/'tools'))
from verify_source_release import verify

class SourceInventory(unittest.TestCase):
    def fixture(self,directory,change=None):
        files={'sims2-independent/'+name:b'source notice' for name in
               ('LICENSE','CREDITS.txt','SOURCE-RELEASE.md','repositories.lock.json',
                'sdk/LICENSE','runtime-sdk/LICENSE','runtime-sdk/LOCAL_CHANGES_LICENSE.md')}
        files['PS5_Sims2Independent/LICENSE.md']=b'foundation notice'
        inventory={name:{'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest()} for name,data in files.items()}
        if change:change(files)
        archive=directory/'source.tar.gz'
        with tarfile.open(archive,'w:gz') as output:
            for name,data in files.items():
                member=tarfile.TarInfo(name);member.size=len(data);output.addfile(member,io.BytesIO(data))
        manifest=directory/'source-manifest.json'
        manifest.write_text(json.dumps({'schema':1,'owner_game_content_included':False,
            'archive':{'bytes':archive.stat().st_size,'sha256':hashlib.sha256(archive.read_bytes()).hexdigest()},
            'components':{'snapshot':{'archive_prefix':'','files':inventory}}}))
        # Real exports use distinct component prefixes. A single prefix here
        # isolates member integrity from the component export implementation.
        record=json.loads(manifest.read_text());record['components']={}
        for prefix in ('sims2-independent','PS5_Sims2Independent'):
            record['components'][prefix]={'archive_prefix':prefix,'files':{
                name[len(prefix)+1:]:item for name,item in inventory.items() if name.startswith(prefix+'/')}}
        manifest.write_text(json.dumps(record))
        return archive,manifest

    def test_complete_snapshot_and_tampered_member(self):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp);archive,manifest=self.fixture(root)
            self.assertEqual(verify(archive,manifest)['files'],8)
            archive,manifest=self.fixture(root,lambda files:files.__setitem__('sims2-independent/CREDITS.txt',b'other content'))
            with self.assertRaisesRegex(ValueError,'member (size|digest) mismatch'):verify(archive,manifest)

    def test_extra_owner_member_and_missing_member_rejected(self):
        for change in (lambda files:files.__setitem__('sims2-independent/assets/default.xbe',b'XBEH'),
                       lambda files:files.pop('sims2-independent/CREDITS.txt')):
            with tempfile.TemporaryDirectory() as temp:
                archive,manifest=self.fixture(Path(temp),change)
                with self.assertRaises(ValueError):verify(archive,manifest)

if __name__=='__main__':unittest.main()
