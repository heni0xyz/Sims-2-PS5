"""Regression checks for asset preflight; fixtures contain no game data."""
from pathlib import Path
import tempfile,unittest
from check_assets import REQUIRED,member,validate,verify_inventory

class PreflightTests(unittest.TestCase):
    def setUp(self):
        parent=Path(__file__).resolve().parents[1]/'out/test-fixtures'
        parent.mkdir(parents=True,exist_ok=True)
        self.tmp=tempfile.TemporaryDirectory(dir=parent)
        self.root=Path(self.tmp.name).resolve()
        self.root.relative_to(parent.resolve())
        self.addCleanup(self.tmp.cleanup)
    def fixture(self):
        for name in REQUIRED:
            path=self.root/name
            path.parent.mkdir(parents=True,exist_ok=True)
            path.write_bytes(b'synthetic test data')
    def test_missing_dsp_refuses_before_launch(self):
        self.fixture();(self.root/'sims2.bin').unlink()
        with self.assertRaisesRegex(ValueError,'sims2.bin'):validate(self.root)
    def test_wrong_xbe_refuses_before_launch(self):
        self.fixture()
        with self.assertRaisesRegex(ValueError,'Unsupported XBE'):validate(self.root)
    def test_case_insensitive_disc_names(self):
        path=self.root/'INDEX.IND';path.write_bytes(b'x')
        self.assertEqual(member(self.root,'index.ind'),path)
    def test_empty_file_is_not_complete(self):
        (self.root/'file.bin').touch()
        with self.assertRaisesRegex(ValueError,'Empty'):member(self.root,'file.bin')
    def test_missing_parent_is_reported(self):
        with self.assertRaisesRegex(ValueError,'DATA/index.ind'):member(self.root,'DATA/index.ind')
    def test_inventory_does_not_accept_changed_content(self):
        (self.root/'test.bin').write_bytes(b'x')
        inventory={'files':{'test.bin':{'bytes':1,'sha256':'0'*64}}}
        with self.assertRaisesRegex(ValueError,'SHA-256 mismatch'):verify_inventory(self.root,inventory)
    def test_inventory_cannot_escape_root(self):
        with self.assertRaisesRegex(ValueError,'Unsafe inventory'):verify_inventory(self.root,{'files':{'../secret':{}}})

if __name__=='__main__':unittest.main()
