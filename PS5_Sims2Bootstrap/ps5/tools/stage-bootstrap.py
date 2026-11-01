#!/usr/bin/env python3
# Copyright (C) 2026 OpenAI Codex, for szampan
# SPDX-License-Identifier: GPL-3.0-only
"""Stage public instruction resources after the unmodified native link recipe."""
from pathlib import Path
import json
import shutil
import sys

def stage(app):
    root=Path(__file__).resolve().parents[2]
    identity=json.loads((root/'ps5/sce_sys/param.json').read_text())
    if app.name!=identity['titleId'] or not (app/'eboot.bin').is_file():
        raise ValueError('Expected the already packaged instruction title')
    # The foundation validates this directory in its base constructor, before
    # our own program or font override runs. This is public documentation only.
    (app/'assets').mkdir(exist_ok=True)
    (app/'assets/README.txt').write_text('Required empty resource directory for the public instruction screen.\n'
        'No Xbox game files belong here until your private builder output is copied.\n')
    shutil.copytree(root/'public-resources',app/'public-resources',dirs_exist_ok=True)
    shutil.copytree(root/'licenses',app/'licenses',dirs_exist_ok=True)
    for source,name in (('COPYING','LICENSE'),):
        shutil.copyfile(root/source,app/name)
    components=app/'licenses/components.json'
    records=json.loads(components.read_text())
    def clean(value):
        if isinstance(value,list):return [clean(x) for x in value]
        if isinstance(value,dict):return {k:clean(v) for k,v in value.items() if k!='path' and not (
            k=='remote' and isinstance(v,str) and not v.startswith(('http://','https://')))}
        return value
    records=clean(records)
    for record in records:
        if record.get('id')=='title':
            record['name']='PS5 Vulkan foundation and bundled libraries'
    components.write_text(json.dumps(records,indent=2)+'\n')

if __name__=='__main__':stage(Path(sys.argv[1]).resolve())
