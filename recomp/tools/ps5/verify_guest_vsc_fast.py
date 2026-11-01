"""Exercise the freshly generated USA fast-constant wrapper with a tail-call stub.
Extracted game code stays in private out/, never in reusable source control.
"""
from pathlib import Path
import argparse,hashlib,json,os,subprocess
ROOT=Path(__file__).resolve().parents[2]

def main():
    p=argparse.ArgumentParser();p.add_argument('--compiler',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    if hashlib.sha1((ROOT/'assets/default.xbe').read_bytes()).hexdigest()!='16f9e41ce26c315a758f25d43ba1fa9d3809d1ad':raise ValueError('Unsupported XBE')
    function=None
    for source in (ROOT/'generated/independent').glob('x86_recomp.*.cpp'):
        text=source.read_text();marker='X86_FUNC(__imp__sub_00272FD0) {'
        if marker in text:function=text[text.index(marker):text.index('X86_WEAK_FUNC(sub_00272FD0,',text.index(marker))];break
    if function is None:raise ValueError('Fresh generated body is unavailable')
    a.output.mkdir(parents=True,exist_ok=False)
    harness=r'''#include "cpu/x86_ops.h"
#include <vector>
#include <cstdio>
#include <cstring>
#include <cstdlib>
uint32_t seenReg,seenSource,seenCount;
void Require(bool ok,const char* text){if(!ok){std::fprintf(stderr,"FAIL %s\n",text);std::exit(1);}}
X86_FUNC(sub_00272EF0){seenReg=ctx.ecx;seenSource=ctx.edx;seenCount=X86_MEM32(ctx.esp+4);ctx.esp+=8;}
'''+function+r'''
int main(){
    std::vector<uint8_t> memory(4*1024*1024);uint8_t* base=memory.data();
    for(unsigned flag:{0u,16u})for(unsigned reg:{0u,97u,121u,149u})for(unsigned count:{4u,8u}){
        X86Context ctx{};ctx.esp=0x3f0000;ctx.ecx=reg;ctx.edx=0x1000;
        ctx.ebx=0x12345;ctx.esi=0x23456;ctx.edi=0x34567;
        const uint32_t saved=ctx.esp;
        X86_MEM32(saved)=0xfeedface;X86_MEM32(saved+4)=count;
        X86_MEM8(0x281b78)=flag;
        for(unsigned i=0;i<count;i++)X86_MEM32(0x1000+i*4)=0x3f000000+i;
        std::memset(base+0x280ab0+reg*16,0xCD,count*4);
        __imp__sub_00272FD0(ctx,base);
        Require(seenReg==reg && seenSource==0x1000 && seenCount==count,"fastcall registers plus stack dword count");
        Require(ctx.esp==saved+8,"ret 4 stack cleanup");
        Require(ctx.ebx==0x12345 && ctx.esi==0x23456 && ctx.edi==0x34567,"callee-saved registers");
        for(unsigned i=0;i<count;i++)Require(X86_MEM32(0x280ab0+reg*16+i*4)==(flag?0xcdcdcdcd:0x3f000000+i),"conditional scratch copy matches tail-call contract");
    }
    std::puts("PASS_VSC_FAST_ABI: 16 cases, ecx/edx/stack, ret4, scratch flags, saved registers");
}
'''
    source=a.output/'actual-vsc-fast.cpp';source.write_text(harness)
    exe=a.output/('probe.exe' if os.name=='nt' else 'probe')
    env=dict(os.environ,PATH=str(a.compiler.parent)+os.pathsep+os.environ['PATH'])
    command=[str(a.compiler),'-std=c++20','-O2','-march=x86-64-v2','-I'+str(ROOT/'runtime-sdk/runtime'),str(source),'-o',str(exe)]
    result=subprocess.run(command,env=env,capture_output=True,text=True,timeout=120)
    (a.output/'compile.log').write_text(result.stdout+result.stderr)
    if result.returncode:print(result.stderr);return result.returncode
    result=subprocess.run([str(exe)],env=env,capture_output=True,text=True,timeout=15)
    (a.output/'run.log').write_text(result.stdout+result.stderr)
    report={'status':'PASS' if result.returncode==0 else 'FAIL','function':'00272FD0','callee':'00272EF0',
        'register_arguments':['ecx=register','edx=source'],'stack_arguments':['dword_count'],'return_pop_bytes':4,
        'function_sha256':hashlib.sha256(function.encode()).hexdigest(),'cases':16,
        'scope':'actual generated wrapper ABI; native GPU/HLE integration is a separate test'}
    (a.output/'result.json').write_text(json.dumps(report,indent=2)+'\n');print(result.stdout+result.stderr,end='');return result.returncode
if __name__=='__main__':raise SystemExit(main())
