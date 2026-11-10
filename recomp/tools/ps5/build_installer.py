# Copyright (C) 2026 OpenAI Codex, for szampan
# SPDX-License-Identifier: GPL-3.0-only
"""Build the player-facing ISO installer with the existing native/RADV packager.

No game image, translated source or guest object is a release input. Create its
foundation with the official template factory before running this command.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import sys
from types import SimpleNamespace

ROOT=Path(__file__).resolve().parents[2]
sys.path[:0]=[str(ROOT),str(Path(__file__).resolve().parent)]
from builder.core import BuildError, sha
from build_console_compiler import run
import build_linker_probe as proven
from native_title_probe import source_inputs
from prepare_installer_resources import prepare as stage, fingerprint


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--settings',type=Path,required=True)
    parser.add_argument('--platform-headers',type=Path,required=True)
    parser.add_argument('--phase',choices=('prepare','package'),required=True)
    parser.add_argument('--title-id',choices=('PPSA99145','PPSA99148'),default='PPSA99148')
    parser.add_argument('--foundation',type=Path,default=ROOT.parent/'PS5_Sims2Installer')
    parser.add_argument('--output',type=Path,default=ROOT/'out/ps5-installer')
    args=parser.parse_args();settings=json.loads(args.settings.read_text());output=args.output.resolve();output.mkdir(parents=True,exist_ok=True)
    foundation=args.foundation.resolve();compiler=ROOT/'compiler';kit=Path(settings['kit'])
    param=json.loads((foundation/'ps5/sce_sys/param.json').read_text())
    if param['titleId'] not in ('PPSA99145','PPSA99148'):raise BuildError('Unexpected installer foundation identity')
    os.environ['PATH']=settings['clang_bin']+os.pathsep+os.environ.get('PATH','')
    pointer=output/'resource-state.json'
    if args.phase=='prepare':
        sources=['installer_pipeline.cpp','installer_pipeline.h','installer_app.cpp','installer_ui_assets.cpp','installer_ui_assets.h',
                 'installer_view.cpp','installer_view.h','installer_music.cpp','installer_music.h',
                 'third_party/minimp3.h','third_party/minimp3_ex.h',
                 'installer_updates.cpp','installer_updates.h',
                 'public_update.cpp','public_update.h','launcher_version.h',
                 'executable_file.h',
                 'launcher_preferences.cpp','launcher_preferences.h',
                 'native_packager.cpp','native_packager.h','embedded.cpp','generator.cpp',
                 '../src/game/install.cpp','../src/game/install.h','../src/game/install_notices.cpp','../src/game/install_filesystem.h',
                 '../src/game/disc_boot.cpp','../src/game/disc_boot.h','../runtime-sdk/runtime/kernel/disc.cpp',
                 '../runtime-sdk/runtime/kernel/disc.h','../runtime-sdk/runtime/kernel/crypto.cpp',
                 '../platform/ps5/filesystem.cpp','../platform/ps5/filesystem.h']
        sources+=source_inputs(ROOT)
        sources=sorted(set(sources))
        seed=fingerprint(dict(title_id=args.title_id,kit=sha(kit/'kit.json'),config=sha(ROOT/'config/game.toml'),
            embedded=sha(compiler/'embedded.cpp'),native=sha(compiler/'native_packager.cpp'),
            native_header=sha(compiler/'native_packager.h'),notices=sha(ROOT/'src/game/install_notices.cpp'),
            version=sha(ROOT/'config/installer-version.json'),sdk=json.loads((ROOT/'repositories.lock.json').read_text())['sdk']['revision'],
            title_identity=sha(ROOT/'config/ps5-titles.json'),
            resource_stager=sha(Path(__file__).with_name('prepare_installer_resources.py')),build_driver=sha(Path(__file__)),
            background_tool=sha(Path(__file__).with_name('prepare_home_background.py')),
            background_sources={p.name:sha(p) for p in (ROOT/'tools/branding').rglob('*') if p.is_file()},
            artwork={str(p.relative_to(ROOT)):sha(p) for p in sorted((ROOT/'artwork').rglob('*.png'))},
            audio={str(p.relative_to(ROOT)):sha(p) for p in sorted((ROOT/'artwork/audio').iterdir()) if p.is_file()},
            audio_notice=sha(compiler/'third_party/minimp3-NOTICE.txt')))
        resource=output/('resources-'+seed[:16])
        if not resource.exists():stage(settings,resource,title_id=args.title_id)
        manifest=json.loads((resource/'installer-manifest.json').read_text())
        for name,item in manifest['files'].items():
            if sha(resource/name)!=item['sha256']:raise BuildError('Installer resource changed: '+name)
        pointer.write_text(json.dumps(dict(path=str(resource),fingerprint=seed),indent=2)+'\n')
        build_id=hashlib.sha256((seed+''.join(sha(compiler/name) for name in sources)).encode()).hexdigest()[:16]
        run([settings['ninja'],'-C',Path(settings['output'])/'ps5','-j','2','sims2-console-generator'],output,'check-installer-generator')
        sample=foundation/'examples/thesims2/thesims2.cpp';shutil.copyfile(compiler/'installer_app.cpp',sample)
        prelude=foundation/'ps5/src/prelude.h';text=prelude.read_text()
        for line in ('#include <fstream>','#include <cmath>','#include "ps5_samples.h"','#include "installer_pipeline.h"','#include "installer_ui_assets.h"','#include "installer_music.h"','#include "launcher_preferences.h"'):
            if line not in text:text+='\n'+line+'\n'
        prelude.write_text(text)
        samples=foundation/'ps5/src/samples.cpp';samples.write_text(samples.read_text().replace('SAMPLE(thesims2, true,','SAMPLE(thesims2, false,'))
        # Title-local optional hooks. No other installed title or upstream
        # foundation copy changes. Upstream submission remains explicitly deferred.
        overlay=foundation/'base/VulkanUIOverlay.cpp';text=overlay.read_text()
        declaration='extern void ps5InstallerAtlas(ImFontAtlas *) __attribute__((weak));'
        if declaration not in text:text=text.replace('#include "VulkanUIOverlay.h"','#include "VulkanUIOverlay.h"\n#if defined(VK_EXAMPLE_PS5)\n'+declaration+'\n#endif')
        needle='\t\tio.Fonts->GetTexDataAsRGBA32(&fontData, &texWidth, &texHeight);'
        if 'if (ps5InstallerAtlas)' not in text:
            if needle not in text:raise BuildError('Unknown font-atlas upload boundary')
            text=text.replace(needle,'#if defined(VK_EXAMPLE_PS5)\n        if (ps5InstallerAtlas) ps5InstallerAtlas(io.Fonts);\n#endif\n'+needle)
        overlay.write_text(text)
        platform=foundation/'ps5/src/platform.c';text=platform.read_text()
        declaration='extern const char *ps5NextExecutable(void) __attribute__((weak));'
        if declaration not in text:text=text.replace('void\ncatchReturnFromMain(int status)',declaration+'\nvoid\ncatchReturnFromMain(int status)')
        if 'event=handoff' not in text:
            needle='   (void)sceSystemServiceLoadExec("exit", NULL);'
            if needle not in text:raise BuildError('Unknown native shutdown boundary')
            text=text.replace(needle,'''   const char *next = status == 0 && ps5NextExecutable ? ps5NextExecutable() : NULL;
   if (next) {
      say("[SIMS2][INSTALLER][INFO] event=handoff path=%s", next);
      int result = sceSystemServiceLoadExec(next, NULL);
      if (result < 0) {
         say("[SIMS2][INSTALLER][ERROR] event=handoff_failed result=%d", result);
         (void)sceSystemServiceLoadExec("exit", NULL);
      }
   } else (void)sceSystemServiceLoadExec("exit", NULL);''')
        platform.write_text(text)
        # The updater executes a copy of the trusted launcher and publishes the
        # root eboot only from that helper. Forward only the controlled argv hook.
        text=platform.read_text()
        declaration='extern const char *const *ps5NextExecutableArguments(void) __attribute__((weak));'
        if declaration not in text:
            text=text.replace('extern const char *ps5NextExecutable(void) __attribute__((weak));',
                              'extern const char *ps5NextExecutable(void) __attribute__((weak));\n'+declaration)
        text=text.replace('sceSystemServiceLoadExec(next, NULL)',
                          'sceSystemServiceLoadExec(next, ps5NextExecutableArguments ? ps5NextExecutableArguments() : NULL)')
        platform.write_text(text)
        main_source=foundation/'ps5/src/main.cpp';text=main_source.read_text()
        if 'ps5InstallerArguments' not in text:
            if 'int main()' not in text:raise BuildError('Unknown title main entry point')
            text=text.replace('int main()',
                'extern "C" bool ps5InstallerArguments(int, char **) __attribute__((weak));\nint main(int argc, char **argv)')
            text=text.replace('platform_init(PS5_TITLE_NAME);',
                'platform_init(PS5_TITLE_NAME);\n\tif (ps5InstallerArguments && !ps5InstallerArguments(argc, argv)) return 1;')
        main_source.write_text(text)
        for name in ('public-resources','licenses/opensans'):
            shutil.copytree(resource/name,foundation/name,dirs_exist_ok=True)
        cmake=foundation/'ps5/CMakeLists.txt';text=cmake.read_text();start='# BEGIN SIMS2 INSTALLER\n'
        if start in text:text=text[:text.index(start)]
        text=text.replace('"SHELL:-include ${CMAKE_CURRENT_SOURCE_DIR}/third_party/volk/volk.h"','-include "${CMAKE_CURRENT_SOURCE_DIR}/third_party/volk/volk.h"')
        text+=start+'''target_sources(samples PRIVATE
 "${SIMS2_COMPILER_SOURCE}/installer_pipeline.cpp"
 "${SIMS2_COMPILER_SOURCE}/installer_ui_assets.cpp"
 "${SIMS2_COMPILER_SOURCE}/installer_view.cpp"
 "${SIMS2_COMPILER_SOURCE}/installer_music.cpp"
 "${SIMS2_COMPILER_SOURCE}/installer_updates.cpp"
 "${SIMS2_COMPILER_SOURCE}/public_update.cpp"
 "${SIMS2_COMPILER_SOURCE}/launcher_preferences.cpp"
 "${SIMS2_COMPILER_SOURCE}/native_packager.cpp"
 "${SIMS2_COMPILER_SOURCE}/third_party/ps5_native_tool/elf_object.cpp"
 "${SIMS2_COMPILER_SOURCE}/third_party/ps5_native_tool/sce_module_writer.cpp"
 "${SIMS2_COMPILER_SOURCE}/third_party/ps5_native_tool/self_container.cpp"
 "${SIMS2_GAME_SOURCE}/src/game/install.cpp"
 "${SIMS2_GAME_SOURCE}/src/game/install_notices.cpp"
 "${SIMS2_GAME_SOURCE}/src/game/disc_boot.cpp"
 "${SIMS2_GAME_SOURCE}/runtime-sdk/runtime/kernel/disc.cpp"
 "${SIMS2_GAME_SOURCE}/runtime-sdk/runtime/kernel/crypto.cpp"
 "${SIMS2_GAME_SOURCE}/platform/ps5/filesystem.cpp")
target_include_directories(samples PRIVATE "${SIMS2_COMPILER_SOURCE}"
 "${SIMS2_GAME_SOURCE}/src" "${SIMS2_GAME_SOURCE}/runtime-sdk/runtime"
 "${SIMS2_COMPILER_SOURCE}/third_party/zlib_headers" "${SIMS2_PLATFORM_HEADERS}"
 "${SIMS2_LLVM_SOURCE}/llvm/include" "${SIMS2_LLVM_BUILD}/include")
set_source_files_properties("${SIMS2_COMPILER_SOURCE}/installer_pipeline.cpp"
 "${CMAKE_BINARY_DIR}/wrap/thesims2.cpp" PROPERTIES COMPILE_DEFINITIONS
 "SIMS2_INSTALLER_BUILD_ID=\\"${SIMS2_INSTALLER_BUILD_ID}\\";SIMS2_INSTALLER_VERSION=\\"${SIMS2_INSTALLER_VERSION}\\";SIMS2_INSTALLER_TITLE_ID=\\"${SIMS2_INSTALLER_TITLE_ID}\\"")
'''
        cmake.write_text(text)
        build=foundation/'build/windows-ps5'
        run([settings['cmake'],'-S',foundation/'ps5','-B',build,'-G','Ninja',
            '-DCMAKE_MAKE_PROGRAM='+settings['ninja'],'-DCMAKE_BUILD_TYPE=Release','-DCMAKE_EXPORT_COMPILE_COMMANDS=ON',
            '-DCMAKE_TOOLCHAIN_FILE='+str(compiler/'ps5-toolchain.cmake'),
            '-DSIMS2_COMPILER_BIN='+Path(settings['clang_bin']).as_posix(),'-DSIMS2_PUBLIC_KIT='+kit.as_posix(),
            '-DSIMS2_COMPILER_SOURCE='+compiler.as_posix(),'-DSIMS2_GAME_SOURCE='+ROOT.as_posix(),
            '-DSIMS2_LLVM_SOURCE='+Path(settings['llvm']).as_posix(),'-DSIMS2_LLVM_BUILD='+(Path(settings['output'])/'ps5').as_posix(),
            '-DSIMS2_PLATFORM_HEADERS='+args.platform_headers.resolve().as_posix(),'-DSIMS2_INSTALLER_BUILD_ID='+build_id,
            '-DSIMS2_INSTALLER_VERSION='+manifest['version'],'-DSIMS2_INSTALLER_TITLE_ID='+args.title_id],output,'configure-installer')
        run([settings['ninja'],'-C',build,'-j','2','samples'],output,'build-installer-objects')
        inventory=dict(build_id=build_id,title_id=args.title_id,mode='compiler',fixtures=[],compiler_resources=[],
            execution_probe=False,generation_probe=False,game_unit_probe=False,game_unit_count=0,
            authored_inputs={name:sha(compiler/name) for name in sources},console_status='NOT_TESTED',
            kind='ps5_owner_iso_installer',version=manifest['version'])
        (resource/'probe-inputs.json').write_text(json.dumps(inventory,indent=2)+'\n')
        print(json.dumps(dict(status='PASS_INSTALLER_OBJECTS',build_id=build_id)))
    else:
        resource=Path(json.loads(pointer.read_text())['path'])
        if json.loads((resource/'installer-manifest.json').read_text())['title_id']!=args.title_id:
            raise BuildError('Prepared resources belong to a different title')
        proven.TITLE_ID=args.title_id;proven.SAMPLE='thesims2'
        package_args=SimpleNamespace(output=resource,mode='compiler',execute=False,generate=False,
            game_unit=False,all_game_units=False,native_title_input=None,foundation=foundation)
        proven.package(package_args,settings)
        result=json.loads((resource/'package-result.json').read_text());title=Path(result['title']);manifest=json.loads((resource/'installer-manifest.json').read_text())
        for name in manifest['files']:
            dest=title/name;dest.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(resource/name,dest)
        param_file=title/'sce_sys/param.json';param=json.loads(param_file.read_text())
        previous_id=param['titleId'];param['titleId']=args.title_id
        identity=json.loads((ROOT/'config/ps5-titles.json').read_text())[args.title_id]
        param['contentId']=identity['content_id'];param['conceptId']=identity['concept_id']
        # The title-selection track is independently normalized to PS5's Home level.
        param.setdefault('pubtools',{})['loudnessSnd0']=format(json.loads((ROOT/'artwork/audio/home-sound.json').read_text())['loudness_lufs'],'.2f')
        param_file.write_text(json.dumps(param,indent=2)+'\n')
        shutil.copyfile(resource/'installer-manifest.json',title/'installer-manifest.json')
        (title/'assets/README.txt').write_bytes(b'SIMS2-INSTALLER-PLACEHOLDER-1\n')
        if (title/'assets/README.txt').read_bytes()!=b'SIMS2-INSTALLER-PLACEHOLDER-1\n':
            raise BuildError('Invalid public placeholder bytes')
        shutil.copyfile(ROOT/'CREDITS.txt',title/'CREDITS.txt')
        shutil.copyfile(ROOT/'LICENSE',title/'LICENSE')
        native=json.loads(Path(result['manifest']).read_text());native['kind']='ps5_owner_iso_installer';native['mode']='installer'
        native['files']={p.relative_to(title).as_posix():dict(bytes=p.stat().st_size,sha256=sha(p)) for p in sorted(title.rglob('*')) if p.is_file()}
        Path(result['manifest']).write_text(json.dumps(native,indent=2)+'\n')
        (output/'package-result.json').write_text(json.dumps(result,indent=2)+'\n')
        print(json.dumps(dict(status='BUILD_PASS_CONSOLE_NOT_TESTED',title=str(title),build_id=result['build_id'])))


if __name__=='__main__':main()
