#!/usr/bin/env python3
# Copyright (C) 2026 OpenAI Codex, for szampan
# SPDX-License-Identifier: GPL-3.0-only
"""Bundle a validated console compiler diagnostic for a private tester."""
import argparse
import hashlib
import json
from pathlib import Path
import zipfile


def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--result', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    result = json.loads(args.result.read_text())
    manifest_path = Path(result['manifest'])
    manifest = json.loads(manifest_path.read_text())
    title = Path(result['title'])
    if (result['status'] != 'BUILD_PASS_CONSOLE_NOT_TESTED' or
            result['build_id'] != manifest['build_id'] or
            manifest['mode'] != 'compiler' or manifest['title_id'] != 'PPSA99147' or
            manifest['console_status'] != 'NOT_TESTED' or title.name != manifest['title_id']):
        raise ValueError('Unexpected probe identity/status')
    files = {p.relative_to(title).as_posix(): p for p in title.rglob('*') if p.is_file()}
    if files.keys() != manifest['files'].keys():
        raise ValueError('Title inventory differs from validated manifest')
    for name, path in files.items():
        if path.is_symlink() or '..' in Path(name).parts:
            raise ValueError('Unsafe title member: '+name)
        if path.suffix.lower() in {'.iso', '.xbe', '.arc', '.xmv', '.ngh'} or name.lower().startswith(('hdd/', 'movies/')):
            raise ValueError('Owner data in diagnostic: '+name)
        if name.startswith('compiler-resources/') and path.suffix.lower() in {'.o', '.obj', '.a'}:
            raise ValueError('Compiler probe must compile its own fixtures')
        expected = manifest['files'][name]
        if path.stat().st_size != expected['bytes'] or digest(path) != expected['sha256']:
            raise ValueError('Title digest mismatch: '+name)
    if digest(title/'eboot.bin') != manifest['eboot_sha256']:
        raise ValueError('eboot identity mismatch')
    debug = manifest_path.parent/'llvm-pie.elf'
    if digest(debug) != manifest['debug_elf_sha256']:
        raise ValueError('Debug ELF identity mismatch')
    args.output.mkdir(parents=True, exist_ok=False)
    execution = manifest.get('execution_probe', False)
    game_unit = manifest.get('game_unit_probe', False)
    inputs = ('This internal generator profile requires your own supported default.xbe in\n'
        '/data/sims2-compiler-inputs/default.xbe. The XBE is NOT included in this ZIP.\n'
        if manifest.get('generation_probe', False) else 'No Xbox ISO, game files or saves are needed.\n')
    output_behavior = ('The generated authored test ELF is loaded and called through the pinned\n'
        'PS5_RetroArch loader; results must match the 32-call arithmetic oracle.\n'
        if execution else 'The generated test ELF is validated but never loaded/executed by this title.\n')
    if manifest.get('generation_probe', False):
        output_behavior += ('The supported owner XBE is translated locally into private generated source;\n'
            'all 71 files must match the reviewed reference. Keep generated source private.\n')
    if game_unit:
        output_behavior += ('One real generated game source file is compiled at O2 on PS5. Its object\n'
            'is checked and kept private; game code is NOT linked or executed.\n')
    readme = f'''Sims 2 Console Compiler Diagnostics - {manifest['build_id']}
Title: PPSA99147. This is a development test, NOT a playable game or ISO installer.
{inputs}

1. Copy the ENTIRE PPSA99147 folder to your normal prepared homebrew setup.
   Include compiler-resources, assets/README.txt, font, shaders and sce_module.
   Do not merge it into the game or the earlier linker test.
2. Close other titles. Start full klogsrv capture BEFORE launching this title.
3. Press Cross (X) to start. Wait for PASS or FAIL, then Circle to exit.
   Maximum test time: 120 seconds. If display AND heartbeats freeze, close
   through the PS5 shell after two minutes and preserve the klog.
4. Return the full private klog and the generated per-run folder:
   /data/sims2-compiler-probe/<run>
   or fallback PPSA99147/compiler-runs/<run>
   Return the log even if no run folder appeared. Include a screen photo on FAIL.

The test compiles authored C++ on PS5, checks filesystem operations, handles an
expected syntax error, links, checks an expected unresolved symbol and retries.
I/O failure still means overall FAIL even if compilation later succeeds.
Do not publicly post unrelated kernel messages or private network information.
{output_behavior}
This particular build is NOT TESTED on console at delivery. Send results for analysis.
'''
    (args.output/'README-TESTER.txt').write_text(readme, encoding='utf-8')
    archive = args.output/f"Sims2-Console-Compiler-Test-{manifest['build_id']}.zip"
    with zipfile.ZipFile(archive, 'x', zipfile.ZIP_DEFLATED, compresslevel=1) as bundle:
        bundle.writestr('README-TESTER.txt', readme)
        for name, path in sorted(files.items()):
            bundle.write(path, title.name+'/'+name)
    with zipfile.ZipFile(archive) as bundle:
        if bundle.testzip() is not None:
            raise ValueError('ZIP CRC failure')
        if set(bundle.namelist()) != {'README-TESTER.txt'} | {title.name+'/'+name for name in files}:
            raise ValueError('ZIP inventory mismatch')
        for name, record in manifest['files'].items():
            content = bundle.read(title.name+'/'+name)
            if len(content) != record['bytes'] or hashlib.sha256(content).hexdigest() != record['sha256']:
                raise ValueError('ZIP member digest mismatch: '+name)
    receipt = dict(status='PASS_LOCAL_DELIVERY', build_id=manifest['build_id'],
        title_id=title.name, files_verified=len(files), zip=str(archive.resolve()),
        zip_bytes=archive.stat().st_size, zip_sha256=digest(archive),
        eboot_sha256=manifest['eboot_sha256'], debug_elf=str(debug.resolve()),
        debug_elf_sha256=manifest['debug_elf_sha256'], manifest=str(manifest_path.resolve()),
        owner_game_data_included=False, precompiled_fixture_objects_included=False,
        console_compilation='NOT_TESTED', linked_fixture_execution='NOT_TESTED',
        game_code_executed=False, execution_probe=execution,
        generation_probe=manifest.get('generation_probe', False), game_unit_probe=game_unit)
    (args.output/'delivery-manifest.json').write_text(json.dumps(receipt, indent=2)+'\n')
    print(json.dumps(receipt, indent=2))


if __name__ == '__main__':
    main()
