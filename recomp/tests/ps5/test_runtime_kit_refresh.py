# Copyright (C) 2026 OpenAI Codex, for szampan
# SPDX-License-Identifier: GPL-3.0-only
import hashlib,json,sys,tempfile,unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[2]))
from tools.builder.assemble_windows_kit import refresh_native
class RuntimeKitRefresh(unittest.TestCase):
    def temporary(self):
        parent=Path(__file__).resolve().parents[2]/'out/runtime-kit-refresh-tests'
        parent.mkdir(parents=True,exist_ok=True)
        temporary=tempfile.TemporaryDirectory(dir=parent)
        assert Path(temporary.name).resolve().is_relative_to(parent.resolve())
        return temporary
    def fixture(self,root):
        kit=root/'kit';native=root/'native'
        old='native/inputs/runtime-000-old.cpp.o';new='native/inputs/runtime-001-new.cpp.o'
        for base,name,data in ((kit,old,b'old'),(kit,'public-resources/keep.txt',b'keep'),(native,new,b'new')):
            path=base/name;path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes(data)
        def record(path):return dict(bytes=path.stat().st_size,sha256=hashlib.sha256(path.read_bytes()).hexdigest())
        (kit/'kit.json').write_text(json.dumps(dict(files={n:record(kit/n) for n in (old,'public-resources/keep.txt')})))
        (native/'native-recipe.json').write_text(json.dumps(dict(sdk_wrapper_evaluated=True,
            files={new:record(native/new)['sha256']},link_argv=['{KIT}/'+new])))
        return kit,native,old,new
    def test_obsolete_runtime_is_removed_and_unrelated_files_preserved(self):
        with self.temporary() as folder:
            kit,native,old,new=self.fixture(Path(folder));refresh_native(native,kit)
            self.assertFalse((kit/old).exists());self.assertEqual((kit/new).read_bytes(),b'new')
            self.assertEqual((kit/'public-resources/keep.txt').read_bytes(),b'keep')
            self.assertNotIn(old,json.loads((kit/'kit.json').read_text())['files'])
    def test_modified_obsolete_object_blocks_refresh_before_mutation(self):
        with self.temporary() as folder:
            kit,native,old,new=self.fixture(Path(folder));(kit/old).write_bytes(b'user modification')
            before=(kit/'kit.json').read_bytes()
            with self.assertRaisesRegex(ValueError,'modified'):refresh_native(native,kit)
            self.assertEqual((kit/old).read_bytes(),b'user modification')
            self.assertFalse((kit/new).exists());self.assertEqual((kit/'kit.json').read_bytes(),before)
if __name__=='__main__':unittest.main()
