"""Patch English console notices in a NEW private USA QuickData archive.

The equal-length UTF-16 replacement preserves every record/string offset and
length. Game identifiers, save paths, original files and attribution stay intact.
Authored by OpenAI Codex for szampan.
"""
from pathlib import Path
import argparse,hashlib,json,os,struct
from patch_prompts import read_index,digest
SOURCE_SHA256='1fa759e2e274bc92eb40ac84d7ac70977fa9eba2e86a27b1152ac0240201d7e7'
NEEDLE='Xbox console.'.encode('utf-16le')
REPLACEMENT='PlayStation. '.encode('utf-16le')
STORAGE_RULES=(
    ("Checking hard disk.  Please don't turn off your PlayStation. ","Checking storage device. Don't turn off your PlayStation.",10),
    ('No saved game found on hard disk.','No save found on storage device.',4),
    ('Load saved game from the hard disk?','Load save from the storage device?',3),
)

def patch_storage_bytes(data,counts=None):
    counts=counts if counts is not None else [r[2] for r in STORAGE_RULES]
    if len(counts)!=len(STORAGE_RULES):raise ValueError('Unexpected storage rule inventory')
    result=data
    for (old,new,_),expected in zip(STORAGE_RULES,counts):
        if len(new)>len(old):raise ValueError('Storage notice exceeds its original string field')
        before=old.encode('utf-16le');after=new.ljust(len(old)).encode('utf-16le')
        if result.count(before)!=expected:raise ValueError('Unexpected storage notice count')
        result=result.replace(before,after)
    if len(result)!=len(data):raise ValueError('Storage patch changed record length')
    return result

def patch_notice_bytes(data,expected):
    if len(NEEDLE)!=len(REPLACEMENT):raise ValueError('Replacement changes string length')
    if data.count(NEEDLE)!=expected:raise ValueError('Unexpected console notice count')
    result=data.replace(NEEDLE,REPLACEMENT)
    if len(result)!=len(data) or NEEDLE in result:raise ValueError('Notice patch verification failed')
    return result

def apply(source,output):
    source=Path(os.path.abspath(source));output=Path(os.path.abspath(output))
    if source==output or output.exists():raise ValueError('Use a new output file; preserve the original')
    if digest(source)!=SOURCE_SHA256:raise ValueError('Expected untouched supported USA quickdat.arc')
    with source.open('rb') as stream:entries=read_index(stream)
    offset,size=entries['urbzui'];data=source.read_bytes()
    record=data[offset:offset+size];patched=patch_storage_bytes(patch_notice_bytes(record,22))
    result=data[:offset]+patched+data[offset+size:]
    if result[:offset]!=data[:offset] or result[offset+size:]!=data[offset+size:]:raise ValueError('Unrelated archive bytes changed')
    output.parent.mkdir(parents=True,exist_ok=True)
    with output.open('xb') as stream:stream.write(result)
    if digest(output)!=hashlib.sha256(result).hexdigest():raise ValueError('Written archive differs')
    return {'status':'PASS_UI_TEXT_PATCH','source_sha256':SOURCE_SHA256,'output_sha256':digest(output),
        'bytes':len(result),'source_resource':'urbzui','replacements':22,'encoding':'UTF-16LE',
        'string_and_record_lengths_preserved':True,
        'storage_replacements':17,'storage_rule_counts':{r[0]:r[2] for r in STORAGE_RULES},
        'coverage':'English console notices: Xbox console. -> PlayStation.; 17 checking/missing-save/load notices: hard disk -> storage device.',
        'not_covered':'Other Xbox references/languages and direct ISO streaming text are not claimed replaced'}

def main():
    p=argparse.ArgumentParser();p.add_argument('source',type=Path);p.add_argument('output',type=Path);p.add_argument('--report',type=Path);a=p.parse_args()
    report=apply(a.source,a.output)
    if a.report:a.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))
if __name__=='__main__':main()
