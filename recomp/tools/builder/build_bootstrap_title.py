# SPDX-License-Identifier: GPL-3.0-only
"""Link the public instruction app through the captured template/RADV recipe.

Replaces only the template application's object list. No Xbox runtime, game
archive or generated game code is linked into this instruction executable.
"""
from pathlib import Path
import argparse
import json
import re
import shutil
import sys
sys.path.insert(0,str(Path(__file__).resolve().parents[2]))
from builder.core import Builder, BuildError, sha, verify_kit, verify_native_elf

def build(a):
    manifest=verify_kit(a.kit)
    work=a.output.resolve();work.mkdir(parents=True,exist_ok=False);(work/'tmp').mkdir()
    job=Builder(a.kit,work/'unused.iso',work/'unused');job.work=work
    commands=json.loads((a.foundation/'build/windows-ps5/compile_commands.json').read_text())
    objects=[Path(c['output']) for c in commands]
    if not objects or any(not x.is_file() for x in objects):raise BuildError('Incomplete CMake object inventory')
    argv=job.expand(manifest['link_argv'])
    first=next(i for i,x in enumerate(argv) if x.endswith('-app_crt.o'))
    last=next(i for i,x in enumerate(argv) if x.endswith('-libSceAgc.so'))
    if first>=last:raise BuildError('Unrecognized captured object ordering')
    argv=argv[:first+1]+[str(x.resolve()) for x in objects]+argv[last:]
    job.run(argv,'link-bootstrap',timeout=1800)
    verify_native_elf(work/'llvm-pie.elf')
    log=job.run([str(a.kit.resolve()/'tools/llvm-nm.exe'),'--defined-only',str(work/'llvm-pie.elf')],'defined-symbols')
    if re.search(r'__imp__sub_[0-9A-Fa-f]+|X86FuncMappings|X86Continuations',log.read_text()):
        raise BuildError('Game definitions in public bootstrap')
    log=job.run(job.expand(manifest['undefined_argv']),'undefined-imports')
    symbols=set(re.findall(r'\bU\s+(\S+)',log.read_text()))
    if symbols-set(manifest['allowed_imports']):raise BuildError('Unresolved bootstrap imports')
    title=work/'PPSA99145';title.mkdir()
    (title/'assets').mkdir()
    (title/'assets/README.txt').write_text('Required public instruction-screen resource directory; no game data included.\n')
    for directory in ('sce_sys','sce_module','licenses'):
        shutil.copytree(a.kit/'title'/directory,title/directory)
    for name in ('LICENSE',):
        shutil.copyfile(a.kit/'title'/name,title/name)
    shaders=title/'shaders/glsl/base';shaders.mkdir(parents=True)
    for file in (a.foundation/'shaders/glsl/base').glob('*.spv'):shutil.copyfile(file,shaders/file.name)
    shutil.copytree(a.foundation/'public-resources',title/'public-resources')
    shutil.copytree(a.foundation/'licenses/opensans',title/'licenses/opensans',dirs_exist_ok=True)
    job.run(job.expand(manifest['convert_argv']),'convert')
    job.run(job.expand(manifest['sign_argv']),'sign')
    job.run(job.expand(manifest['inspect_argv']),'inspect')
    result=dict(schema=1,kind='public_instruction_title_no_translated_game_code',
        status='BUILD_PASS_CONSOLE_NOT_TESTED',eboot_sha256=sha(title/'eboot.bin'),
        debug_elf_sha256=sha(work/'llvm-pie.elf'),application_objects=len(objects),
        generated_game_definitions_found=False,
        files={p.relative_to(title).as_posix():dict(bytes=p.stat().st_size,sha256=sha(p))
               for p in sorted(title.rglob('*')) if p.is_file()})
    (work/'build-manifest.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({k:v for k,v in result.items() if k!='files'},indent=2))

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    for k in ('kit','foundation','output'):p.add_argument('--'+k,type=Path,required=True)
    build(p.parse_args())
