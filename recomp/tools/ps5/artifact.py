"""Native artifact validator reused from the prior Codex platform tooling.
Source attribution: docs/PLATFORM_IMPORTS.json. No deployment or game logic.
"""
from pathlib import Path
import hashlib,json,re,subprocess
def digest(path):
    with Path(path).open('rb') as stream:return hashlib.file_digest(stream,'sha256').hexdigest()
def write_json(path,value):Path(path).write_text(json.dumps(value,indent=2)+'\n',encoding='utf-8')

def validate_artifact(title, vulkan, output, elf_override=None, kind="platform_foundation_only", build_id=None,param_override=None):
    title, vulkan, output = Path(title).resolve(), Path(vulkan).resolve(), Path(output)
    param = json.loads((Path(param_override) if param_override else title / "ps5/sce_sys/param.json").read_text())
    title_id = param["titleId"]
    if not re.fullmatch(r"PPSA\d{5}", title_id):
        raise ValueError("invalid title ID")
    package = title / "dist" / title_id
    elf = Path(elf_override).resolve() if elf_override is not None else title / "build/ps5/link/llvm-pie.elf"
    files = [package / "eboot.bin", package / "sce_sys/param.json", package / "sce_module/libc.prx", elf]
    for file in files:
        if not file.is_file() or not file.stat().st_size:
            raise ValueError("required artifact is missing: " + file.name)
    if json.loads((package / "sce_sys/param.json").read_text()) != param:
        raise ValueError("package identity differs from configured identity")
    header = elf.read_bytes()[:20]
    if header[:6] != b"\x7fELF\x02\x01" or header[18:20] != b"\x3e\x00":
        raise ValueError("debug file is not an ELF64 little-endian x86-64 image")
    inspector = vulkan / "build/host/ps5-native-tool"
    result = subprocess.run([str(inspector), "self", "--inspect", "--file", str(files[0])],
                            capture_output=True, text=True, timeout=30, check=True)
    (output / "artifact-inspect.log").write_text(result.stdout+result.stderr, encoding="utf-8")
    manifest = {"schema": 1, "title_id": title_id, "kind": kind, "build_id": build_id,
                "eboot_sha256": digest(files[0]), "debug_elf_sha256": digest(elf),
                "files": {p.relative_to(package).as_posix(): {"sha256": digest(p), "bytes": p.stat().st_size}
                          for p in sorted(package.rglob("*")) if p.is_file()}}
    write_json(output / "build-manifest.json", manifest)
    return manifest, package, elf
