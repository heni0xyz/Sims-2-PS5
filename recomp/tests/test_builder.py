# SPDX-License-Identifier: GPL-3.0-only
import json
import struct
from pathlib import Path
import sys
import tempfile
import threading
import time
import unittest
from builder.core import Builder, BuildError, Cancelled, EXPECTED_XBE, relative, sha, verify_kit, verify_native_elf


class BuilderSafety(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory(prefix='builder path with spaces ')
        self.root=Path(self.temp.name).resolve()
        self.kit=self.root/'kit';self.kit.mkdir()
        self.file=self.kit/'tool.txt';self.file.write_text('public tool fixture')
        self.manifest=dict(schema=1,kind='public_owner_builder',xbe_sha1=EXPECTED_XBE,
                           title_id='PPSA99145',files={'tool.txt':dict(bytes=self.file.stat().st_size,sha256=sha(self.file))})
        self.write_manifest()
    def tearDown(self):self.temp.cleanup()
    def write_manifest(self):
        (self.kit/'kit.json').write_text(json.dumps(self.manifest))
    def test_kit_tampering_is_detected_before_executing_tools(self):
        self.assertEqual(verify_kit(self.kit)['title_id'],'PPSA99145')
        self.file.write_text('changed')
        with self.assertRaisesRegex(BuildError,'Damaged'):verify_kit(self.kit)
    def test_extra_files_are_rejected(self):
        (self.kit/'unexpected.exe').write_bytes(b'fixture')
        with self.assertRaisesRegex(BuildError,'unexpected'):verify_kit(self.kit)
    def test_manifest_cannot_escape_kit(self):
        for name in ('../outside','/absolute','C:/outside','bad\\path'):
            with self.subTest(name=name),self.assertRaises(BuildError):relative(self.kit,name)
    def test_existing_output_and_saves_survive(self):
        output=self.root/'PPSA99145';(output/'hdd').mkdir(parents=True)
        save=output/'hdd/save';save.write_bytes(b'owner save')
        with self.assertRaisesRegex(BuildError,'NEW output'):
            Builder(self.kit,self.root/'owner.iso',output).build()
        self.assertEqual(save.read_bytes(),b'owner save')
    def test_wrong_title_identity_is_rejected(self):
        self.manifest['title_id']='PPSA00000';self.write_manifest()
        with self.assertRaisesRegex(BuildError,'identity'):verify_kit(self.kit)
    def test_paths_are_separate_argv_elements(self):
        job=Builder(self.kit,self.root/'My USA copy.iso',self.root/'new')
        job.work=self.root/'work with spaces'
        result=job.expand(['tool','{ISO}','-I{GENERATED}','{OBJECT}'],OBJECT='my object.o')
        self.assertEqual(len(result),4)
        self.assertEqual(result[1],str(job.iso))
        self.assertEqual(result[-1],'my object.o')
    def test_cancel_terminates_only_our_child_and_keeps_source(self):
        source=self.root/'owner.iso';source.write_bytes(b'owner source fixture')
        cancel=threading.Event();job=Builder(self.kit,source,self.root/'new',cancel=cancel)
        job.work=self.root/'work';job.work.mkdir();(job.work/'tmp').mkdir()
        timer=threading.Timer(.3,cancel.set);timer.start();start=time.monotonic()
        try:
            with self.assertRaises(Cancelled):job.run([sys.executable,'-c','import time; time.sleep(30)'],'cancel-child')
        finally:timer.cancel()
        self.assertLess(time.monotonic()-start,8)
        self.assertEqual(source.read_bytes(),b'owner source fixture')
    def test_timeout_preserves_diagnostics(self):
        job=Builder(self.kit,self.root/'owner.iso',self.root/'new')
        job.work=self.root/'work';job.work.mkdir();(job.work/'tmp').mkdir()
        with self.assertRaisesRegex(BuildError,'Timed out'):
            job.run([sys.executable,'-c','import time; print("stage entered",flush=True); time.sleep(30)'],'timeout',timeout=.3)
        self.assertIn('stage entered',(job.work/'timeout.log').read_text())
    def test_native_layout_requires_pie_and_16k_load_alignment(self):
        image=bytearray(120)
        image[:6]=b'\x7fELF\x02\x01'
        struct.pack_into('<HH',image,16,3,62)
        struct.pack_into('<QQ',image,24,0x100,64)
        struct.pack_into('<HH',image,54,56,1)
        struct.pack_into('<IIQQQQQQ',image,64,1,5,0,0,0,120,0x1000,0x4000)
        elf=self.root/'test.elf';elf.write_bytes(image)
        verify_native_elf(elf)
        struct.pack_into('<H',image,16,2);elf.write_bytes(image)
        with self.assertRaisesRegex(BuildError,'position-independent'):verify_native_elf(elf)
        struct.pack_into('<H',image,16,3);struct.pack_into('<Q',image,112,0x1000);elf.write_bytes(image)
        with self.assertRaisesRegex(BuildError,'16 KiB'):verify_native_elf(elf)


if __name__=='__main__':unittest.main()
