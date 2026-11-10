# Copyright (C) 2026 OpenAI Codex, for szampan
# SPDX-License-Identifier: GPL-3.0-only
"""Bounded installer GUI smoke test using the existing console tools.

This profile only checks the missing-data screen. It neither installs game data
nor counts as a passing complete ISO workflow. Screenshots and klog stay private.
"""
import argparse
import hashlib
import importlib.util
import io
import json
from pathlib import Path
import re
import sys
from ftplib import FTP


def module(name,path):
    spec=importlib.util.spec_from_file_location(name,path);value=importlib.util.module_from_spec(spec);spec.loader.exec_module(value);return value


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--console-tools',type=Path,required=True)
    parser.add_argument('--result',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();sys.path.insert(0,str(args.console_tools.resolve()))
    import ps5_console
    deploy=module('installer_upstream_deploy',args.console_tools/'deploy-title-folder.py')
    runner=module('installer_upstream_run',args.console_tools/'run-title.py')
    result=json.loads(args.result.read_text());manifest=json.loads(Path(result['manifest']).read_text());title=Path(result['title'])
    if (result['status']!='BUILD_PASS_CONSOLE_NOT_TESTED' or manifest['kind']!='ps5_owner_iso_installer' or
            title.name!='PPSA99148' or manifest['title_id']!=title.name or result['build_id']!=manifest['build_id']):
        raise ValueError('Unexpected installer identity')
    args.output.mkdir(parents=True,exist_ok=False);settings=ps5_console.load_settings()
    record=dict(build_id=result['build_id'],title_id=title.name,state='STARTING',complete_iso_workflow=False,game_code_executed=False)
    def save(**values):
        record.update(values);(args.output/'run-summary.json').write_text(json.dumps(record,indent=2)+'\n');print(json.dumps(values),flush=True)
    def idle():
        answer=ps5_console.ps5vkctl_command(settings,'procs',timeout=20)
        if ' count=0 ' not in answer+' ':raise RuntimeError('Another application is running; launch refused')
    def ftp():
        value=FTP();value.connect(settings['host'],settings['ftp_port'],timeout=30);value.login(settings['ftp_user'],settings['ftp_password'] or 'codex')
        if 'disabled' not in value.sendcmd('SELF').lower():raise RuntimeError('Cannot read raw title bytes')
        return value
    try:
        idle();save(state='DEPLOY');argv=sys.argv
        try:sys.argv=[str(args.console_tools/'deploy-title-folder.py'),str(title),'--always','eboot.bin'];deploy.main()
        finally:sys.argv=argv
        save(state='VERIFY');base='/data/homebrew/'+title.name
        names=['eboot.bin','sce_sys/param.json','installer-manifest.json','public-resources/OpenSans-Regular.ttf',
               'public-resources/installer-icon.png','public-resources/installer-background.png']
        with ftp() as connection:
            for name in names:
                digest=hashlib.sha256();connection.retrbinary('RETR '+base+'/'+name,digest.update,blocksize=1<<20)
                if digest.hexdigest()!=manifest['files'][name]['sha256']:raise ValueError('Remote title differs: '+name)
            idle();connection.storbinary('STOR '+base+'/test-run.txt',io.BytesIO(b'frames 180\nscreenshot\noverlay on\nsamples thesims2\n'))
        save(state='RUN');ended,log,files=runner.run_title(title.name,r'\[The Sims 2\] ends: status [01]',120,
            output=args.output/'klog.log',fetch=('screenshots/thesims2.ppm','test-results.txt'),
            echo_pattern=r'INSTALLER|FAILED|fatal signal|ends: status|screenshot',elf=Path(result['manifest']).parent/'llvm-pie.elf')
        idle();text=log.read_text(encoding='utf-8',errors='replace')
        screenshot=next(p for p in files if p.suffix=='.ppm')
        from PIL import Image,ImageStat
        with Image.open(screenshot) as image:
            size=list(image.size);spread=max(ImageStat.Stat(image.convert('RGB')).stddev)
            image.save(args.output/'installer.png')
        good=(ended=='finished' and 'event=launcher_ready' in text and manifest['build_id'] in text and
              '[The Sims 2] ends: status 0' in text and not re.search(r'fatal signal|VK_ERROR_DEVICE_LOST|watchdog_timeout|FAILED',text,re.I) and
              size==[3840,2160] and spread>2)
        save(state='PASS_GUI_SMOKE' if good else 'FAIL',console_idle_after=True,runner_end=ended,image_size=size,image_spread=spread,
             screenshot=str(args.output/'installer.png'))
        return 0 if good else 1
    except BaseException as error:save(state='FAIL',error=str(error));raise


if __name__=='__main__':raise SystemExit(main())
