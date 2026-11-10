# Copyright (C) 2026 OpenAI Codex, for szampan
# SPDX-License-Identifier: GPL-3.0-only
"""Build/run the launcher navigation, transaction and mocked-HTTP host checks.

Uses the existing LLVM-MinGW/LLVM host build recorded in --settings. This is a
host test, not a substitute for PS5 transport, haptics, LoadExec or power-loss QA.
"""
import argparse
import datetime
import json
import os
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--settings', type=Path, required=True)
    parser.add_argument('--foundation', type=Path, default=ROOT.parent/'PS5_Sims2Installer')
    parser.add_argument('--resource-state', type=Path, default=ROOT/'out/ps5-installer/resource-state.json')
    args = parser.parse_args()
    settings = json.loads(args.settings.read_text())
    stamp = datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%SZ')
    out = ROOT/'out/ps5-installer/host-validation'/stamp
    out.mkdir(parents=True)
    env = dict(os.environ, PATH=settings['clang_bin']+os.pathsep+os.environ['PATH'])
    compiler = Path(settings['clang_bin'])/'clang++.exe'
    include = ['-Icompiler', '-Isrc', '-I'+str(Path(settings['llvm'])/'llvm/include'),
               '-I'+str(Path(settings['output'])/'host/include')]
    libs = [str(Path(settings['output'])/'host/lib'/name) for name in ('libLLVMSupport.a','libLLVMDemangle.a')]
    libs += ['-lpsapi','-lshell32','-lole32','-luuid','-ladvapi32','-lws2_32','-lntdll']
    records = []

    def run(label, command, timeout=180):
        with (out/(label+'.log')).open('wb') as log:
            result = subprocess.run(list(map(str, command)), cwd=ROOT, env=env, stdout=log,
                                    stderr=subprocess.STDOUT, timeout=timeout)
        records.append(dict(stage=label, status='PASS' if result.returncode==0 else 'FAIL', command=list(map(str,command))))
        (out/'result.json').write_text(json.dumps(dict(scope='Host only', stages=records), indent=2)+'\n')
        print(label, records[-1]['status'], flush=True)
        if result.returncode:
            raise SystemExit(f'Failed: {out/(label+".log")}')

    common = [compiler, '-std=c++20', '-O1', '-static', '-pthread', *include]
    obj = out/'public-update.o'
    run('compile-transaction', [*common, '-c', 'compiler/public_update.cpp', '-o', obj])
    driver = out/'update-test.exe'
    run('link-transaction', [*common, 'tests/ps5/public_update_driver.cpp', obj, *libs, '-o', driver])
    run('transaction', [settings['python'], 'tests/ps5/test_public_update.py', '--driver', driver, '-v'])
    network = out/'http-test.exe'
    run('link-http', [*common, 'tests/ps5/update_network.cpp', 'compiler/installer_updates.cpp', obj, *libs, '-o', network])
    run('http', [network])
    imgui = args.foundation/'external/imgui'
    tinygltf = args.foundation/'external/tinygltf'
    preview = out/'preview.exe'
    run('compile-ui', [compiler, '-std=c++20', '-O2', '-static', '-Icompiler', '-I'+str(imgui), '-I'+str(tinygltf),
                      'tools/ps5/preview_installer_ui.cpp', 'compiler/installer_view.cpp', 'compiler/installer_ui_assets.cpp',
                      *[imgui/name for name in ('imgui.cpp','imgui_draw.cpp','imgui_widgets.cpp')], '-o', preview])
    resource = json.loads(args.resource_state.read_text())
    run('ui', [preview, Path(resource['path'])/'public-resources', out/'screenshots'])
    music = out/'music-test.exe'
    run('compile-music', [compiler, '-std=c++20', '-O2', '-static', '-Icompiler',
                          'tests/ps5/installer_music.cpp', 'compiler/installer_music.cpp', '-o', music])
    run('music', [music, ROOT/'artwork/audio/LTqj8VkTC8z-tTY7vztlF.mp3'])
    run('extraction-contracts', [settings['python'], 'tools/test_install.py', '--clang', compiler])
    (ROOT/'out/ps5-installer/host-validation/latest.json').write_text(json.dumps(dict(path=str(out)), indent=2)+'\n')
    print('PASS_HOST_LAUNCHER', out)


if __name__ == '__main__':
    main()
