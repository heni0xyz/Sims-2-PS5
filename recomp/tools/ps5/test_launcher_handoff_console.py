# Copyright (C) 2026 OpenAI Codex, for szampan
# SPDX-License-Identifier: GPL-3.0-only
"""Test an already deployed launcher's cached-game handoff on an owner PS5.

No deployment, extraction or compilation is performed. Reuses the template's
idle check, capture-before-launch, watchdog and shutdown handling. Requires an
authorized, configured console and a complete owner-prepared game/cache.
Boot/shutdown evidence must not be presented as gameplay or performance proof.
"""
import argparse
import datetime
from ftplib import FTP, error_perm
import hashlib
import importlib.util
import io
import json
from pathlib import Path
import re
import sys


def classify(text, run_id):
    own = [line for line in text.splitlines() if 'run='+run_id+' ' in line]
    return dict(
        game_boot=any('milestone=BOOT ' in line for line in own),
        guest_memory=any('milestone=MEM ' in line for line in own),
        own_shutdown=any('milestone=SHUTDOWN ' in line for line in own),
        initial_play=bool(re.search(r'event=initial_check_complete phase=8 section=0\b', text)),
        # A generic 0x80aa001a also accompanies errno22 from the template's
        # normal LoadExec("exit"). Do not confuse it with errno13/EACCES.
        executable_access_denied=bool(re.search(r'error=0x80aa001a errno=13\b', text)),
        template_exit_einval='Exit spawn failed : 0x80aa001a ELF path=exit' in text,
        fatal=bool(re.search(r'fatal signal|GPU_FAULT|VK_ERROR_DEVICE_LOST|\[SIMS2\]\[CRASH\]', text, re.I)))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--console-tools', type=Path, required=True)
    parser.add_argument('--package-result', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--seconds', type=int, default=60)
    args = parser.parse_args()
    if not 30 <= args.seconds <= 600:
        parser.error('seconds must be 30..600')
    package = json.loads(args.package_result.read_text())
    manifest = json.loads(Path(package['manifest']).read_text())
    title = manifest['title_id']
    if not re.fullmatch(r'PPSA\d{5}', title) or manifest.get('kind') != 'ps5_owner_iso_installer':
        raise ValueError('Expected a native public launcher manifest')
    sys.path.insert(0, str(args.console_tools.resolve()))
    import ps5_console
    spec = importlib.util.spec_from_file_location('handoff_runner', args.console_tools/'run-title.py')
    runner = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(runner)
    settings = ps5_console.load_settings()
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    run_id = 'handoff-'+datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%SZ')
    base = '/data/homebrew/'+title
    result = dict(title_id=title, launcher_build=package['build_id'], run_id=run_id,
                  state='PREFLIGHT', gameplay_verified=False, performance_verified=False)

    def save(**values):
        result.update(values)
        (out/'run-summary.json').write_text(json.dumps(result, indent=2)+'\n')
        print(json.dumps(values), flush=True)

    def idle():
        reply = ps5_console.ps5vkctl_command(settings, 'procs', timeout=20)
        if ' count=0 ' not in reply+' ':
            raise RuntimeError('Console busy; refusing to close another application')

    def ftp():
        f = FTP()
        f.connect(settings['host'], settings['ftp_port'], timeout=30)
        f.login(settings['ftp_user'], settings['ftp_password'] or 'codex')
        f.voidcmd('TYPE I')
        if 'disabled' not in f.sendcmd('SELF').lower():
            f.close()
            raise RuntimeError('Raw signed SELF transfer unavailable')
        return f

    def remote_hash(f, path):
        h = hashlib.sha256()
        f.retrbinary('RETR '+path, h.update, blocksize=1 << 20)
        return h.hexdigest()

    def read(f, path):
        b = io.BytesIO()
        f.retrbinary('RETR '+path, b.write)
        return b.getvalue()

    hooks = {'installer-autotest.txt': b'SIMS2-INSTALLER-AUTOTEST-1\n',
             'sims2-test-run.txt': f'run {run_id}\nseconds {args.seconds}\n'.encode()}
    staged = []
    try:
        idle()
        with ftp() as f:
            if remote_hash(f, base+'/eboot.bin') != manifest['files']['eboot.bin']['sha256']:
                raise RuntimeError('Deployed launcher differs from intended build')
            if read(f, base+'/.sims2-build/owner.txt') != b'SIMS2-OWNER-BUILD-CACHE-1\n':
                raise RuntimeError('Unknown cache owner marker')
            receipt = json.loads(read(f, base+'/.sims2-build/cache.json'))
            identity, expected = receipt['link_id'], receipt['game_sha256']
            if not all(re.fullmatch('[0-9a-f]{64}', n) for n in (identity, expected)):
                raise RuntimeError('Malformed private game identity')
            game = base+'/game-'+identity+'.bin'
            if remote_hash(f, game) != expected:
                raise RuntimeError('Private game digest differs from cache receipt')
            f.cwd(base)
            # The pinned console FTP server supports LIST, but not NLST.
            listing = []
            f.retrlines('LIST', listing.append)
            names = {line.split()[-1] for line in listing if line.split()}
            if any(name in names for name in (*hooks, 'test-run.txt')):
                raise RuntimeError('Existing test hook; refusing to overwrite it')
            idle()
            for name, data in hooks.items():
                staged.append(name)
                f.storbinary('STOR '+base+'/'+name, io.BytesIO(data))
        save(state='CAPTURE_AND_RUN', private_game_sha256=expected)
        ended, log, _ = runner.run_title(title,
            r'\[SIMS2\]\[SHUTDOWN\]\[INFO\].*run='+re.escape(run_id)+r' .*milestone=SHUTDOWN',
            args.seconds+120, output=out/'klog.raw.log', exit_grace=30,
            echo_pattern=r'event=executable_mode|event=initial_check_complete|event=play_handoff|milestone=(BOOT|MEM|SHUTDOWN)|fatal signal')
        idle()
        facts = classify(log.read_text(errors='replace'), run_id)
        with ftp() as f:
            facts['game_bytes_unchanged'] = remote_hash(f, game) == expected
        passed = ended == 'finished' and all(facts[n] for n in ('game_boot', 'guest_memory', 'own_shutdown', 'initial_play', 'game_bytes_unchanged'))
        passed = passed and not facts['executable_access_denied'] and not facts['fatal']
        save(state='PASS_BOOT_AND_SHUTDOWN' if passed else 'FAIL', runner_end=ended, **facts)
        return 0 if passed else 1
    except BaseException as error:
        save(state='FAIL', error=str(error))
        raise
    finally:
        if staged:
            idle()  # Never remove a hook while a title could still consume it.
            with ftp() as f:
                for name in staged:
                    try:
                        f.sendcmd('DELE '+base+'/'+name)
                    except error_perm as error:
                        if not str(error).startswith('550'):
                            raise


if __name__ == '__main__':
    raise SystemExit(main())
