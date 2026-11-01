# SPDX-License-Identifier: GPL-3.0-only
"""Asset-free regression for the constructor's required /app0/assets directory."""
from pathlib import Path
import importlib.util
import json
import tempfile
import unittest
import zipfile

ROOT=Path(__file__).resolve().parents[1]
SCRIPT=ROOT.parent/'PS5_Sims2Bootstrap/ps5/tools/stage-bootstrap.py'

class BootstrapPackage(unittest.TestCase):
    def test_instruction_zip_keeps_required_directory_without_game_data(self):
        spec=importlib.util.spec_from_file_location('bootstrap_stage',SCRIPT)
        module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
        with tempfile.TemporaryDirectory(prefix='bootstrap-package-') as temp:
            app=Path(temp)/'PPSA99145';(app/'licenses').mkdir(parents=True)
            (app/'eboot.bin').write_bytes(b'public-artifact-fixture')
            (app/'licenses/components.json').write_text(json.dumps([
                dict(id='title',licence='MIT',source=dict(path='/build-machine/tree'))]))
            module.stage(app)
            self.assertTrue((app/'assets').is_dir())
            self.assertTrue((app/'assets/README.txt').is_file())
            self.assertEqual({p.name for p in (app/'assets').iterdir()},{'README.txt'})
            self.assertTrue((app/'public-resources/OpenSans-Regular.ttf').is_file())
            archive=Path(temp)/'setup.zip'
            with zipfile.ZipFile(archive,'w') as z:
                for path in app.rglob('*'):
                    if path.is_file():z.write(path,'PPSA99145/'+path.relative_to(app).as_posix())
            with zipfile.ZipFile(archive) as z:
                self.assertIn('PPSA99145/assets/README.txt',z.namelist())
                self.assertIsNone(z.testzip())
                self.assertFalse(any(n.endswith(('.iso','.xbe','.arc','.xmv')) for n in z.namelist()))

if __name__=='__main__':unittest.main()
