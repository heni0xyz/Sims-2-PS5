import sys,unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[2]/'tools/ps5'))
from patch_ui_text import patch_notice_bytes,patch_storage_bytes,STORAGE_RULES,NEEDLE,REPLACEMENT

class NoticePatch(unittest.TestCase):
    def test_unicode_lengths_offsets_and_unrelated_data(self):
        text='Saving: Xbox console.\0'
        encoded=text.encode('utf-16le')
        fixture=b'Xbox/save/path\0'+len(text).to_bytes(4,'little')+encoded+b'next-record\0'
        changed=patch_notice_bytes(fixture,1)
        self.assertEqual(len(fixture),len(changed))
        self.assertEqual(changed[:19],fixture[:19])
        self.assertEqual(changed[-12:],fixture[-12:])
        self.assertIn('PlayStation. \0'.encode('utf-16le'),changed)
        self.assertIn(b'Xbox/save/path',changed)
    def test_all_occurrences_and_counts(self):
        self.assertEqual(patch_notice_bytes(NEEDLE+b'\0\0'+NEEDLE,2),REPLACEMENT+b'\0\0'+REPLACEMENT)
        with self.assertRaises(ValueError):patch_notice_bytes(NEEDLE,22)
        with self.assertRaises(ValueError):patch_notice_bytes(REPLACEMENT,1)
    def test_storage_notices_preserve_offsets_and_path_identifiers(self):
        originals=[r[0] for r in STORAGE_RULES]
        strings=b''.join(s.encode('utf-16le')+b'\0\0' for s in originals)
        fixture=b'Xbox/hard disk/save/path\0'+strings+b'untouched-tail'
        patched=patch_storage_bytes(fixture,[1,1,1])
        self.assertEqual(len(patched),len(fixture))
        self.assertTrue(patched.startswith(b'Xbox/hard disk/save/path\0'))
        self.assertTrue(patched.endswith(b'untouched-tail'))
        at=len(b'Xbox/hard disk/save/path\0')
        for old,new,_ in STORAGE_RULES:
            size=len(old.encode('utf-16le'))
            self.assertEqual(patched[at:at+size].decode('utf-16le').rstrip(),new)
            self.assertEqual(patched[at+size:at+size+2],b'\0\0');at+=size+2
        with self.assertRaises(ValueError):patch_storage_bytes(fixture)
if __name__=='__main__':unittest.main()
