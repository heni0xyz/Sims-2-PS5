# Copyright (C) 2026 OpenAI Codex, for szampan
# SPDX-License-Identifier: GPL-3.0-only
"""Bounded owner-game performance capture; preserves data and restores raw eboot.

Reuses the stack's process checks, capture-before-launch, watchdog and exit path.
Gameplay navigation remains manual; this tool never labels menu FPS as gameplay.
"""
import argparse,hashlib,importlib.util,io,json,re,sys
from pathlib import Path
from ftplib import FTP,error_perm

def digest(path):
    with path.open('rb') as stream:return hashlib.file_digest(stream,'sha256').hexdigest()

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--console-tools',type=Path,required=True)
    p.add_argument('--build',type=Path,required=True,help='Private game-build.json')
    p.add_argument('--output',type=Path,required=True)
    p.add_argument('--seconds',type=int,default=300)
    a=p.parse_args()
    if not 30<=a.seconds<=600:raise ValueError('Bounded test must be 30..600 seconds')
    out=a.output.resolve();out.mkdir(parents=True,exist_ok=False)
    sys.path.insert(0,str(a.console_tools.resolve()))
    import ps5_console
    spec=importlib.util.spec_from_file_location('game_perf_runner',a.console_tools/'run-title.py')
    runner=importlib.util.module_from_spec(spec);spec.loader.exec_module(runner)
    build=json.loads(a.build.read_text());title=Path(build['title']);eboot=title/'eboot.bin'
    if title.name!='PPSA99145' or build['status']!='BUILD_PASS_RUNTIME_NOT_TESTED':raise ValueError('Unexpected private title')
    if digest(eboot)!=build['files']['eboot.bin']['sha256']:raise ValueError('Local build changed')
    settings=ps5_console.load_settings();base='/data/homebrew/PPSA99145';backup=out/'original-eboot.bin'
    state=dict(build_id=build['runtime_build_id'],title_id=title.name,state='STARTING',
        gameplay_verified=False,seconds_requested=a.seconds,restored=False)
    changed=False;hook=False
    def save(**values):
        state.update(values);(out/'run-summary.json').write_text(json.dumps(state,indent=2)+'\n');print(json.dumps(values),flush=True)
    def idle():
        reply=ps5_console.ps5vkctl_command(settings,'procs',timeout=20)
        if ' count=0 ' not in reply+' ':raise RuntimeError('Console is not idle; no title will be closed: '+reply)
    def ftp():
        f=FTP();f.connect(settings['host'],settings['ftp_port'],timeout=30)
        f.login(settings['ftp_user'],settings['ftp_password'] or 'codex');f.voidcmd('TYPE I')
        if 'disabled' not in f.sendcmd('SELF').lower():raise RuntimeError('Raw SELF transfer unavailable')
        return f
    def remote_hash(f,name):
        h=hashlib.sha256();f.retrbinary('RETR '+name,h.update,blocksize=1<<20);return h.hexdigest()
    def delete_hook(f):
        try:f.sendcmd('DELE '+base+'/sims2-test-run.txt')
        except error_perm as error:
            if not str(error).startswith('550'):raise
    try:
        idle();save(state='BACKUP_AND_PREFLIGHT')
        with ftp() as f:
            f.cwd(base);listing=[];f.retrlines('LIST',listing.append)
            (out/'original-root-listing.txt').write_text('\n'.join(listing)+'\n')
            if any(line.split()[-1:] == ['sims2-test-run.txt'] for line in listing):
                raise RuntimeError('Existing test hook belongs to another run; refusing overwrite')
            with backup.open('wb') as sink:f.retrbinary('RETR '+base+'/eboot.bin',sink.write,blocksize=1<<20)
            if backup.read_bytes()[:4]==b'\x7fELF':raise RuntimeError('Backup is decoded ELF, not raw signed SELF')
            config=io.BytesIO();f.retrbinary('RETR '+base+'/settings.toml',config.write)
            (out/'settings.toml').write_bytes(config.getvalue())
            xbe=hashlib.sha1();f.retrbinary('RETR '+base+'/assets/default.xbe',xbe.update,blocksize=1<<20)
            if xbe.hexdigest()!='16f9e41ce26c315a758f25d43ba1fa9d3809d1ad':raise RuntimeError('Wrong owner XBE revision')
            save(original_eboot_sha256=digest(backup),settings_sha256=hashlib.sha256(config.getvalue()).hexdigest(),
                 owner_xbe_verified=True)
            idle();save(state='DEPLOY')
            changed=True
            with eboot.open('rb') as source:f.storbinary('STOR '+base+'/eboot.bin',source,blocksize=1<<20)
            if remote_hash(f,base+'/eboot.bin')!=digest(eboot):raise RuntimeError('Deployed eboot digest mismatch')
            run_id=out.name
            if not re.fullmatch(r'[A-Za-z0-9_-]{1,96}',run_id):raise ValueError('Invalid run ID')
            request=f'run {run_id}\nseconds {a.seconds}\n'.encode()
            idle();hook=True
            f.storbinary('STOR '+base+'/sims2-test-run.txt',io.BytesIO(request))
        save(state='CAPTURE_AND_RUN',deployed_eboot_sha256=digest(eboot))
        ended,log,_=runner.run_title(title.name,r'\[SIMS2\]\[SHUTDOWN\]\[INFO\].*milestone=SHUTDOWN',
            a.seconds+180,output=out/'klog.raw.log',echo_pattern=r'build-config|deep-gpu-config|deep-config|perf: [\d.]+ fps|deep-frames|CRASH|fatal signal|Bounded test|SHUTDOWN',
            elf=Path(build['debug_elf']),exit_grace=30)
        text=log.read_text(errors='replace');idle()
        (out/'sims2.log').write_text('\n'.join(line for line in text.splitlines() if '[Sims2Independent]' in line)+'\n')
        faults=bool(re.search(r'fatal signal|GPU_FAULT|gpu fault|VK_ERROR_DEVICE_LOST|\[SIMS2\]\[CRASH\]',text,re.I))
        matching=bool(re.search(r'build='+re.escape(build['runtime_build_id'])+r'\b',text))
        save(state='CAPTURED' if matching and not faults else 'FAIL',runner_end=ended,
             matching_build_observed=matching,faults=faults,console_idle_after=True,
             own_shutdown_observed='milestone=SHUTDOWN' in text)
    except BaseException as error:
        save(state='FAIL',error=str(error));raise
    finally:
        if changed or hook:
            try:
                idle()
                with ftp() as f:
                    if hook:delete_hook(f)
                    if changed:
                        with backup.open('rb') as source:f.storbinary('STOR '+base+'/eboot.bin',source,blocksize=1<<20)
                        if remote_hash(f,base+'/eboot.bin')!=digest(backup):raise RuntimeError('Restore digest mismatch')
                save(restored=True)
            except BaseException as error:save(restore_error=str(error));raise

if __name__=='__main__':main()
