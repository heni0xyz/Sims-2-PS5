# Copyright (C) 2026 OpenAI Codex, for szampan
# SPDX-License-Identifier: GPL-3.0-only
"""Stage only public tools/resources and version boundaries for the PS5 installer."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import struct
import sys
import zipfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))
from builder.core import verify_kit, sha, BuildError


def fingerprint(value):
    return hashlib.sha256(json.dumps(value, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def prepare(settings, destination, title_id='PPSA99148'):
    if title_id not in ('PPSA99145','PPSA99148'):raise BuildError('Unexpected installer title id')
    if destination.exists():
        raise BuildError('Choose a fresh public resource directory')
    kit = Path(settings['kit'])
    manifest = verify_kit(kit)
    destination.mkdir(parents=True)
    resources = destination/'compiler-resources'
    shutil.copytree(Path(settings['llvm'])/'clang/lib/Headers', resources/'clang-resource/include')
    shutil.copytree(kit/'sdk/target/include', resources/'sdk/target/include')
    shutil.copytree(kit/'runtime/cpu', resources/'game-runtime/cpu')
    shutil.copytree(ROOT/'sdk/xdk/5849', resources/'generator/xdk/5849')
    (resources/'generator/config').mkdir()
    shutil.copyfile(ROOT/'config/game.toml', resources/'generator/config/game.toml')
    if sha(ROOT/'config/game.toml') != manifest['files']['config/game.toml']['sha256']:
        raise BuildError('Regenerate the reviewed generation oracle for the changed game config')
    shutil.copytree(kit/'native', destination/'native-title-kit/native')
    for name in ('public-resources', 'licenses'):
        shutil.copytree(kit/'title'/name, destination/name)
    shutil.copyfile(kit/'title/settings.toml', destination/'settings.toml')
    # Reuse the exact public 256px CC0 vectors already rendered for gameplay.
    # This only changes their storage from bottom-up BGRA to PNG; no original
    # game pixels or low-resolution game icons enter the installer.
    from PIL import Image
    pack=(kit/'title/public-resources/dualsense.s2tex').read_bytes()
    report=json.loads((kit/'title/public-resources/dualsense-manifest.json').read_text())
    magic,version,count,width,height,levels,index_size=struct.unpack_from('<8s6I',pack)
    if (magic,version,width,height,levels,index_size)!=(b'S2HDPACK',1,256,256,9,64):
        raise BuildError('Unexpected public HD prompt format')
    entries={}
    for i in range(count):
        _,_,offset,length,name=struct.unpack_from('<QQII32s',pack,32+i*index_size)
        entries[name.split(b'\0',1)[0].decode('ascii')]=(offset,length)
    for name,glyph in [('cross','playstation_button_cross'),('circle','playstation_button_circle'),('l1','playstation_trigger_l1'),('r1','playstation_trigger_r1'),('stick','playstation_stick_r'),('touchpad','playstation5_touchpad_press'),('dpad','playstation_dpad')]:
        resource=next(item['resource'] for item in report['textures'] if item['glyph']==glyph)
        offset,length=entries[resource]
        if offset+length>len(pack) or length<width*height*4:raise BuildError('Invalid HD prompt bounds')
        image=Image.frombytes('RGBA',(width,height),pack[offset:offset+width*height*4],'raw','BGRA').transpose(Image.Transpose.FLIP_TOP_BOTTOM)
        image.save(destination/'public-resources'/('installer-'+name+'.png'))
    shutil.copyfile(ROOT/'artwork/source/20261110/sims2.png', destination/'public-resources/installer-icon.png')
    shutil.copyfile(ROOT/'artwork/source/sims2bg.png', destination/'public-resources/installer-background.png')
    audio=ROOT/'artwork/audio'
    home=json.loads((audio/'home-sound.json').read_text())
    source=audio/'LTqj8VkTC8z-tTY7vztlF.mp3'
    if sha(source)!=home['source_sha256'] or sha(audio/'snd0.at9')!=home['snd0_sha256']:
        raise BuildError('Rebuild the reviewed Home sound; audio inputs changed')
    shutil.copyfile(source,destination/'public-resources/launcher-music.mp3')
    (destination/'sce_sys').mkdir(exist_ok=True)
    # The shell icon is separate from the large in-launcher atlas image.
    # Never inherit the template's tiny TS placeholder.
    with Image.open(ROOT/'artwork/source/20261110/sims2.png') as image:
        image.convert('RGB').resize((512,512),Image.Resampling.LANCZOS).save(destination/'sce_sys/icon0.png',optimize=True)
    shutil.copyfile(audio/'snd0.at9',destination/'sce_sys/snd0.at9')
    from prepare_home_background import prepare as prepare_background
    background=prepare_background(Path(settings['clang_bin'])/'clang++.exe')
    for name in ('pic0.dds','pic1.dds'):
        shutil.copyfile(background/name,destination/'sce_sys'/name)
    visual_notice=destination/'licenses/title-artwork';visual_notice.mkdir(exist_ok=True)
    shutil.copyfile(ROOT/'artwork/source/20261110/PROVENANCE.json',visual_notice/'20261110.json')
    shutil.copyfile(ROOT/'artwork/CC0-1.0.txt',visual_notice/'CC0-1.0.txt')
    notice=destination/'licenses/launcher-audio';notice.mkdir(parents=True)
    for name in ('README.md','source-metadata.json','home-sound.json'):
        shutil.copyfile(audio/name,notice/name)
    shutil.copyfile(ROOT/'compiler/third_party/minimp3-NOTICE.txt',notice/'minimp3-NOTICE.txt')
    # Optional, explicitly supplied release artwork. Missing images keep the
    # CC0 background placeholder; private game captures are never auto-bundled.
    for i in range(1,4):
        preview=ROOT/'artwork/installer'/f'preview-{i}.png'
        if preview.is_file():
            with Image.open(preview) as image:
                if abs(image.width/image.height-16/9)>.01:raise BuildError('Installer previews must be 16:9')
            shutil.copyfile(preview,destination/'public-resources'/preview.name)
    if (ROOT/'artwork/installer/previews-provenance.json').is_file():
        previews_notice=destination/'licenses/launcher-previews';previews_notice.mkdir()
        for name in ('PREVIEWS.md','previews-provenance.json'):
            shutil.copyfile(ROOT/'artwork/installer'/name,previews_notice/name)
    version = json.loads((ROOT/'config/installer-version.json').read_text())
    for name in ('ps5_native_tool', 'zlib_headers'):
        folder = destination/'licenses'/name; folder.mkdir(exist_ok=True)
        for file in ('LICENSE', 'ORIGIN.json'):
            shutil.copyfile(ROOT/'compiler/third_party'/name/file, folder/file)
    files = {p.relative_to(destination).as_posix():dict(bytes=p.stat().st_size, sha256=sha(p))
             for p in sorted(destination.rglob('*')) if p.is_file()}
    arguments = [arg.replace('{KIT}/runtime', '/app0/compiler-resources/game-runtime').replace('{KIT}', '/app0/compiler-resources')
                 for arg in manifest['compile_argv']]
    arguments[0] = '/app0/embedded-clang'
    conversion = manifest['convert_argv']
    stubs = [conversion[i+1].replace('{KIT}/', '') for i, arg in enumerate(conversion) if arg == '--stub']
    stubs += [p.relative_to(kit).as_posix() for p in sorted((kit/'native/stubs').glob('*.so'))]
    lock = json.loads((ROOT/'repositories.lock.json').read_text())
    generator_inputs = {n:r['sha256'] for n,r in files.items() if n.startswith('compiler-resources/generator/')}
    compile_inputs = {n:r['sha256'] for n,r in files.items() if n.startswith('compiler-resources/') and
                      not n.startswith('compiler-resources/generator/') and n.endswith(('.h', '__config', '__config_site'))}
    # Include every libc++ extensionless header too; a standard-library update
    # must invalidate code, even when the compiler revision did not change.
    compile_inputs.update({n:r['sha256'] for n,r in files.items() if n.startswith('compiler-resources/sdk/')})
    native_inputs = [dict(name=n.removeprefix('native-title-kit/'), **r) for n,r in files.items() if n.startswith('native-title-kit/')]
    native_sources = {n:sha(ROOT/'compiler'/n) for n in ('native_packager.cpp', 'native_packager.h','executable_file.h',
        'third_party/ps5_native_tool/ORIGIN.json')}
    result = dict(schema=1, kind='ps5_owner_iso_installer', title_id=title_id, version=version['version'],
        xbe_sha1=manifest['xbe_sha1'], files=files, generated_files=manifest['generated_files'],
        cpp_files=sorted(n for n in manifest['generated_files'] if n.endswith('.cpp')),
        compile_argv=arguments, native_inputs=native_inputs, link_argv=manifest['link_argv'],
        stub_files=stubs, allowed_imports=manifest['allowed_imports'],
        generator_fingerprint=fingerprint(dict(inputs=generator_inputs, sdk=lock['sdk']['revision'])),
        compile_fingerprint=fingerprint(dict(inputs=compile_inputs, argv=arguments,
            llvm='be5b58f51c93c67947b7e18cd45c2f9913f2c311', frontend=sha(ROOT/'compiler/embedded.cpp'))),
        native_fingerprint=fingerprint(dict(inputs=native_inputs, argv=manifest['link_argv'], sources=native_sources)),
        data_patch_fingerprint=sha(ROOT/'src/game/install_notices.cpp'))
    (destination/'installer-manifest.json').write_text(json.dumps(result, indent=2)+'\n')
    return result


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--settings', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    result = prepare(json.loads(args.settings.read_text()), args.output.resolve())
    print(json.dumps(dict(status='PASS_PUBLIC_RESOURCES', files=len(result['files']),
                          output=str(args.output), game_code_included=False)))
