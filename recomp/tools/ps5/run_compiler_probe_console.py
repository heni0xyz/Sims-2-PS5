#!/usr/bin/env python3
# Copyright (C) 2026 OpenAI Codex, for szampan
# SPDX-License-Identifier: GPL-3.0-only
"""Deploy/run the synthetic compiler probe using the existing PS5_Vulkan tools.

Connection settings are those tools' PS5_HOST/FTP_PORT/KLOG_PORT/PS5VKCTL_PORT.
The owner must have authorized the console. This never loads a payload, closes
an unrelated title, changes firmware, handles game data or publishes results.
"""
import argparse
import hashlib
import importlib.util
import io
import json
from pathlib import Path
import re
import shutil
import sys
from ftplib import FTP, error_perm

sys.path.insert(0, str(Path(__file__).resolve().parent))
from analyze_linker_probe import analyze


def module(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    loaded = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(loaded)
    return loaded


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--console-tools', type=Path, required=True)
    parser.add_argument('--result', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    sys.path.insert(0, str(args.console_tools.resolve()))
    import ps5_console
    deploy = module('sims2_upstream_deploy', args.console_tools/'deploy-title-folder.py')
    runner = module('sims2_upstream_runner', args.console_tools/'run-title.py')
    result = json.loads(args.result.read_text())
    manifest_path = Path(result['manifest'])
    manifest = json.loads(manifest_path.read_text())
    title = Path(result['title'])
    if (result['status'] != 'BUILD_PASS_CONSOLE_NOT_TESTED' or
            result['build_id'] != manifest['build_id'] or manifest['mode'] != 'compiler' or
            title.name != 'PPSA99147' or manifest['title_id'] != title.name):
        raise ValueError('Unexpected diagnostic identity')
    args.output.mkdir(parents=True, exist_ok=False)
    shutil.copyfile(manifest_path, args.output/'build-manifest.json')
    settings = ps5_console.load_settings()
    record = dict(build_id=manifest['build_id'], title_id=title.name, state='STARTING',
                  game_ready=False, game_code_executed=False, linked_fixture_executed=False)
    hook_staged = False

    def save(**values):
        record.update(values)
        (args.output/'run-summary.json').write_text(json.dumps(record, indent=2)+'\n')
        print(json.dumps(values), flush=True)

    def idle():
        answer = ps5_console.ps5vkctl_command(settings, 'procs', timeout=20)
        if ' count=0 ' not in answer+' ':
            raise RuntimeError('Console is running an application; no deployment/launch performed: '+answer)

    def ftp():
        connection = FTP()
        connection.connect(settings['host'], settings['ftp_port'], timeout=30)
        connection.login(settings['ftp_user'], settings['ftp_password'] or 'codex')
        connection.voidcmd('TYPE I')
        # ftpsrv v0.21.1 defaults to transforming SELF into ELF on RETR.
        # Its documented SELF command disables that for this connection only;
        # exact manifest hashes must match, rather than accepting a transformed
        # executable with an unexplained digest.
        reply = connection.sendcmd('SELF')
        if 'disabled' not in reply.lower():
            connection.close()
            raise RuntimeError('Cannot establish raw SELF readback mode')
        return connection

    try:
        idle()
        save(state='DEPLOY')
        argv = sys.argv
        try:
            sys.argv = [str(args.console_tools/'deploy-title-folder.py'), str(title), '--always', 'eboot.bin']
            deploy.main()
        finally:
            sys.argv = argv
        save(state='VALIDATE_DEPLOYMENT')
        checked = {}
        base = '/data/homebrew/'+title.name
        selected = ['eboot.bin', 'sce_sys/param.json'] + [
            'compiler-resources/'+item['name'] for item in manifest['compiler_resources']]
        native_profile = manifest.get('native_title_probe')
        if native_profile:
            selected += ['native-title-kit/'+item['name'] for item in native_profile['public_inputs']]
        with ftp() as connection:
            for name in selected:
                hasher = hashlib.sha256()
                connection.retrbinary('RETR '+base+'/'+name, hasher.update, blocksize=1 << 20)
                value = hasher.hexdigest()
                if value != manifest['files'][name]['sha256']:
                    raise ValueError('Remote readback digest mismatch: '+name)
                checked[name] = value
            idle()
            hook = b'frames 180\nscreenshot\noverlay on\nsamples sims2compilerdiagnostics\n'
            connection.storbinary('STOR '+base+'/test-run.txt', io.BytesIO(hook))
            hook_staged = True
        save(state='RUN_BOUNDED_TEST', remote_readback=checked)
        # run_title itself checks idle again and arms/drains klog before launch.
        test_timeout = manifest.get('watchdog_seconds', 120)
        if (test_timeout not in (120, 300, 1800) or
                (test_timeout == 1800 and manifest.get('game_unit_count') != 65) or
                (test_timeout == 300 and not native_profile)):
            raise ValueError('Unrecognized bounded-test profile')
        ended, log, _ = runner.run_title(title.name, r'\[Sims 2 Compiler Diagnostics\] ends: status [01]',
            test_timeout+30, output=args.output/'klog.log', echo_pattern=r'COMPILERPROBE.*(?:event=(?:verdict|compile_end|game_unit_end|game_unit_progress|link_end|io_verdict|shutdown)|text=.*error:)|ends: status',
            exit_grace=30)
        text = log.read_text(encoding='utf-8', errors='replace')
        capture = analyze(text, manifest['build_id'], manifest.get('execution_probe', False),
                          manifest.get('generation_probe', False), manifest.get('game_unit_probe', False),
                          manifest.get('game_unit_count') if manifest.get('game_unit_probe', False) else None,
                          bool(native_profile))
        if native_profile:
            latest = capture['runs'][-1] if capture['runs'] else {}
            if (latest.get('native_objects') != {item['name']:item['sha256'] for item in native_profile['objects']} or
                    latest.get('native_public') != {item['name']:item['sha256'] for item in native_profile['public_inputs']}):
                capture['status'] = 'FAIL'
                capture['native_identity_error'] = 'Console native inputs differ from the prepared manifest'
        (args.output/'capture-summary.json').write_text(json.dumps(capture, indent=2)+'\n')
        run_ids = {run['run_id'] for run in capture['runs'] if run['build_id'] == manifest['build_id']}
        if len(run_ids) != 1 or not re.fullmatch(r'[0-9]+-[0-9]+', next(iter(run_ids), '')):
            raise ValueError('Capture lacks one matching diagnostic run')
        run_id = next(iter(run_ids))
        roots = re.findall(r'event=output_ready path=(/\S+)', text)
        allowed = {'/data/sims2-compiler-probe/'+run_id, '/app0/compiler-runs/'+run_id}
        if not roots or roots[-1] not in allowed:
            raise ValueError('Unexpected diagnostic output path')
        remote = roots[-1].replace('/app0/', base+'/', 1)
        destination = args.output/'compiler-run'
        destination.mkdir()
        with ftp() as connection:
            connection.cwd(remote)
            entries = []
            connection.retrlines('LIST', entries.append)
            total = 0
            collection_errors = []
            for line in entries:
                fields = line.split(maxsplit=8)
                if len(fields) != 9 or not fields[0].startswith('-'):
                    continue
                name, size = fields[8], int(fields[4])
                if not re.fullmatch(r'[A-Za-z0-9_.-]+', name) or name in {'.', '..'}:
                    raise ValueError('Unsafe result name')
                total += size
                if size > (512 if native_profile else 256)*1024*1024 or total > (1536 if native_profile else 512)*1024*1024:
                    raise ValueError('Diagnostic result exceeds collection bound')
                try:
                    with (destination/name).open('wb') as sink:
                        connection.retrbinary('RETR '+remote+'/'+name, sink.write)
                except error_perm as error:
                    # Keep collecting logs/results after one unreadable file;
                    # a partial collection still cannot become a passing run.
                    (destination/name).unlink(missing_ok=True)
                    collection_errors.append(dict(file=name, error=str(error)))
            connection.cwd(base)
            try:
                leftover = connection.size(base+'/test-run.txt')
            except error_perm:
                leftover = None
            if leftover is not None:
                idle()
                existing = io.BytesIO()
                connection.retrbinary('RETR '+base+'/test-run.txt', existing.write)
                if existing.getvalue() == hook:
                    connection.sendcmd('DELE '+base+'/test-run.txt')
        idle()
        summary = json.loads((destination/'run-summary.json').read_text())
        good = (not collection_errors and ended == 'finished' and capture['status'] == 'PASS' and
                summary['build_id'] == manifest['build_id'] and summary['status'] == 'PASS')
        save(state='PASS' if good else 'FAIL', capture_status=capture['status'],
             run_id=run_id, runner_end=ended, console_idle_after=True,
             compiler_result=summary, collection_errors=collection_errors,
             linked_fixture_executed=summary.get('linked_code_executed', False))
        return 0 if good else 1
    except BaseException as error:
        if hook_staged:
            # A refused launch must not leave our frame-budget hook to affect
            # a later manual launch. Never delete a different/user-owned hook,
            # and never touch it while an application is running.
            try:
                idle()
                with ftp() as connection:
                    content = io.BytesIO()
                    try:
                        connection.retrbinary('RETR '+base+'/test-run.txt', content.write)
                    except error_perm:
                        pass
                    else:
                        if content.getvalue() == hook:
                            connection.sendcmd('DELE '+base+'/test-run.txt')
            except BaseException as cleanup_error:
                record['hook_cleanup_error'] = str(cleanup_error)
        save(state='FAIL', error=str(error))
        raise


if __name__ == '__main__':
    raise SystemExit(main())
