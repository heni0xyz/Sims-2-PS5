# Copyright (C) 2026 OpenAI Codex, for szampan
# SPDX-License-Identifier: GPL-3.0-only
"""Apply the full public bundle to an isolated host fixture; never run its SELF."""
import argparse
from contextlib import contextmanager
import ctypes
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
from package_launcher_update import package, managed, digest, ASSET_NAME

ROOT = Path(__file__).resolve().parents[2]


@contextmanager
def fixture_directory(out):
    root = Path(tempfile.mkdtemp(prefix='fixture-', dir=out)).resolve()
    assert root.parent == out.resolve() and root.name.startswith('fixture-')
    try:
        yield root
    finally:
        # Cleanup uses the verified workspace path, with Win32 extended-path
        # syntax so deep SDK members cannot make rmtree stop halfway through.
        target = '\\\\?\\'+str(root) if os.name=='nt' else str(root)
        shutil.rmtree(target)


@contextmanager
def short_fixture_path(root):
    """A temporary, unused DOS drive alias avoids old Win32 MAX_PATH limits.

    Only the newly created workspace fixture is mapped. No registry setting or
    persistent drive changes; all cleanup uses the original workspace path.
    """
    if os.name != 'nt':
        yield root
        return
    mask = ctypes.windll.kernel32.GetLogicalDrives()
    drive = next((chr(n+65)+':' for n in range(25, 3, -1) if not mask & (1 << n)), None)
    if drive is None:
        raise RuntimeError('No unused drive letter for the isolated long-path fixture')
    subprocess.run(['subst', drive, str(root)], check=True, capture_output=True)
    try:
        target = ctypes.create_unicode_buffer(32768)
        if not ctypes.windll.kernel32.QueryDosDeviceW(drive, target, len(target)) or target.value != '\\??\\'+str(root):
            raise RuntimeError('Temporary fixture mapping differs')
        yield Path(drive+'\\')
    finally:
        subprocess.run(['subst', drive, '/D'], check=True, capture_output=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--package-result', type=Path, required=True)
    parser.add_argument('--driver', type=Path, required=True)
    args = parser.parse_args()
    result = json.loads(args.package_result.read_text())
    title, manifest = Path(result['title']), Path(result['manifest'])
    out = ROOT/'out/ps5-installer/update-bundle-tests'/result['build_id']
    out.mkdir(parents=True, exist_ok=True)
    archive = out/ASSET_NAME
    info = package(title, manifest, archive)
    private = {'hdd/E/synthetic-save': b'keep save', 'assets/DATA/owner-data': b'keep owner data',
               'game-'+'a'*64+'.bin': b'keep private code', '.sims2-build/cache.json': b'keep cache',
               '.sims2-launcher-prefs': b'keep preference', 'settings.toml': b'keep user settings',
               'owner.iso': b'synthetic source; no real ISO is copied here'}
    with fixture_directory(out) as root:
        assert root.is_relative_to(out.resolve())
        (root/'sce_sys').mkdir()
        (root/'sce_sys/param.json').write_text(json.dumps({'titleId':title.name}))
        (root/'installer-manifest.json').write_text('{"version":"0.0.0"}')
        shutil.copyfile(title/'eboot.bin', root/'eboot.bin')
        # This fixture is never executed. Force a different old digest so the
        # full-size eboot really crosses the backup/atomic-replacement path,
        # rather than being skipped as an unchanged public resource.
        with (root/'eboot.bin').open('ab') as old_launcher:
            old_launcher.write(b'HOST-ONLY-PREVIOUS-LAUNCHER-FIXTURE')
        shutil.copytree(title/'sce_module', root/'sce_module')
        for name, data in private.items():
            path = root/name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(data)
        with short_fixture_path(root) as native_root:
            for operation, arguments in [('prepare', [archive, info['sha256'], info['version'], '0.0.0']), ('apply', [])]:
                run = subprocess.run([str(args.driver.resolve()), operation, str(native_root), *map(str, arguments)],
                                     capture_output=True, text=True, timeout=240)
                (out/(operation+'.log')).write_text(run.stdout+run.stderr)
                if run.returncode:
                    raise SystemExit(f'{operation} failed: {run.stderr}')
        for name, record in json.loads(manifest.read_text())['files'].items():
            if managed(name):
                assert digest(root/name)==record['sha256'], name
        for name, data in private.items():
            assert (root/name).read_bytes()==data, name
        assert not (root/'.sims2-update/pending').exists()
    report = dict(status='PASS_HOST_FULL_PUBLIC_BUNDLE', build_id=result['build_id'], files=info['files'],
                  bytes=info['bytes'], sha256=info['sha256'], private_fixture_preserved=True,
                  native_eboot_replacement=True,
                  scope='Production bundle writer/staging/apply and full-size eboot replacement on host filesystem; no network, SELF execution or PS5 LoadExec')
    (out/'result.json').write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps(report))


if __name__ == '__main__':
    main()
