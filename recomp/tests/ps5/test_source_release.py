# SPDX-License-Identifier: GPL-3.0-only
import importlib.util
from pathlib import Path
import unittest
ROOT=Path(__file__).resolve().parents[2]
spec=importlib.util.spec_from_file_location('source_release',ROOT/'tools/prepare_source_release.py')
release=importlib.util.module_from_spec(spec);spec.loader.exec_module(release)
class SourceRelease(unittest.TestCase):
    def test_excludes_game_and_executable_payloads(self):
        for name,data in [('assets/DATA/textures.arc',b'x'),('private/notes.txt',b'x'),
                          ('generated/independent/guest.cpp',b'x'),('hdd/save.dat',b'x'),
                          ('source.cpp',b'XBEHhidden'),('readme.txt',b'\x7fELFhidden'),
                          ('artwork/source/foo.png',b'MZhidden'),('eboot.bin',b'x'),('../escape.txt',b'x')]:
            with self.subTest(name=name),self.assertRaises(ValueError):release.inspect(name,data)
        for name in ('C:/private.txt','src\\escape.cpp','.env','config/id_ed25519','keys/console.pem',''):
            with self.subTest(name=name),self.assertRaises(ValueError):release.inspect(name,b'not a credential')
    def test_accepts_authored_source_and_art(self):
        for name,data in [('src/game/disc_boot.cpp',b'// GPL-3.0-only'),('assets/README.md',b'owner data excluded'),
                          ('artwork/source/sims2.pxz',b'PKlocal editable art'),('artwork/source/sims2.png',b'\x89PNG'),
                          ('LICENSE',b'GPL')]:release.inspect(name,data)
if __name__=='__main__':unittest.main()
