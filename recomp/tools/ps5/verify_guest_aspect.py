"""Run the actual generated USA aspect function in a standalone native harness.

Needs the owner's verified XBE and existing generated sources. The extracted
function and compiled harness stay in the explicitly supplied private output
directory; neither is a redistributable source artifact. No source is edited.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess


def constant_bits(xbe):
    if hashlib.sha1(xbe).hexdigest() != '16f9e41ce26c315a758f25d43ba1fa9d3809d1ad':
        raise ValueError('Unsupported XBE revision')
    if xbe[:4] != b'XBEH':
        raise ValueError('Invalid XBE signature')
    base = struct.unpack_from('<I', xbe, 0x104)[0]
    count, table = struct.unpack_from('<II', xbe, 0x11C)
    for i in range(count):
        _, va, _, raw, size = struct.unpack_from('<5I', xbe, table - base + 56 * i)
        if va <= 0x3162D4 <= va + size - 4:
            return struct.unpack_from('<I', xbe, raw + 0x3162D4 - va)[0]
    raise ValueError('Guest aspect constant is not backed by the XBE')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    bits = constant_bits((root / 'assets/default.xbe').read_bytes())
    function = None
    for source in (root / 'generated/independent').glob('x86_recomp.*.cpp'):
        text = source.read_text(encoding='utf-8')
        start = text.find('X86_FUNC(__imp__sub_001C32C0) {')
        if start >= 0:
            end = text.index('X86_WEAK_FUNC(sub_001C32C0, __imp__sub_001C32C0)', start)
            function = text[start:end]
            break
    if function is None:
        raise ValueError('Generate the supported game sources first')
    args.output.mkdir(parents=True, exist_ok=True)
    harness = '''#include "cpu/x86_ops.h"
#include <cstdio>
#include <vector>
#include <cstdlib>
static unsigned calls=0;
void x86_icall(X86Context& ctx,uint8_t*,uint32_t target) {
    if(target==0x1010) ctx.eax=640;
    else if(target==0x1020) ctx.eax=480;
    else std::abort();
    ++calls; ctx.esp+=4;
}
''' + function + '''
int main() {
    std::vector<uint8_t> memory(0x400000);
    auto* base=memory.data();
    X86Context ctx{};
    X86_MEM32(0x3162D4)=CONSTANT_BITS;
    X86_MEM32(0x20000)=0x21000;
    X86_MEM32(0x21000+0xA0)=0x1010;
    X86_MEM32(0x21000+0xA4)=0x1020;
    double ratios[2]{};
    for(unsigned wide=0;wide<2;++wide) {
        ctx={};ctx.esp=0x10000;ctx.ecx=0x20000;ctx.esi=0xFEED1234;
        X86_MEM8(0x20000+748)=uint8_t(wide);
        calls=0;
        __imp__sub_001C32C0(ctx,base);
        ratios[wide]=ctx.st[ctx.top];
        // The first push ecx reserves a local, later popped into caller-saved ECX.
        // Only ESI is a saved nonvolatile register; ECX receives the float bits.
        uint32_t return_bits;float f=float(ratios[wide]);std::memcpy(&return_bits,&f,4);
        if(ctx.esp!=0x10004 || ctx.ecx!=return_bits || ctx.esi!=0xFEED1234 || ctx.top!=7) return 2;
        if(calls!=(wide?0u:2u)) return 3;
    }
    if(std::abs(ratios[0]-4.0/3.0)>0.000001 || std::abs(ratios[1]-16.0/9.0)>0.000001) return 4;
    std::printf("{\\"status\\":\\"PASS\\",\\"normal_aspect\\":%.9f,\\"wide_aspect\\":%.9f,\\"stack_and_saved_registers\\":\\"PASS\\"}\\n",ratios[0],ratios[1]);
}
'''.replace('CONSTANT_BITS', f'0x{bits:08x}u')
    cpp = args.output / 'guest-aspect-private.cpp'
    binary = args.output / ('guest-aspect.exe' if os.name == 'nt' else 'guest-aspect')
    cpp.write_text(harness, encoding='utf-8')
    environment = dict(os.environ, PATH=str(args.compiler.parent) + os.pathsep + os.environ['PATH'])
    commands = [[str(args.compiler), '-std=c++20', '-O2', '-fno-strict-aliasing',
                 '-I' + str(root / 'runtime-sdk/runtime'), str(cpp), '-o', str(binary)], [str(binary)]]
    outputs = []
    for command in commands:
        result = subprocess.run(command, capture_output=True, text=True, env=environment, timeout=120)
        outputs.append(result.stdout + result.stderr)
        (args.output / 'harness.log').write_text('\n'.join(outputs), encoding='utf-8')
        if result.returncode:
            raise RuntimeError(f'Aspect harness failed ({result.returncode}); see harness.log')
    result = json.loads(outputs[-1])
    result.update({'function': '001C32C0', 'constant': '003162D4', 'display_flag_offset': 748,
                   'scope': 'actual generated function with synthetic display/vtable; not a PS5 camera/HUD test',
                   'function_sha256': hashlib.sha256(function.encode()).hexdigest()})
    (args.output / 'result.json').write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result))


if __name__ == '__main__':
    main()
