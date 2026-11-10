#!/usr/bin/env python3
# Copyright (C) 2026 OpenAI Codex, for szampan
# SPDX-License-Identifier: GPL-3.0-only
"""Compile an authored PS5-target fixture and execute it on the Windows CPU.

This does NOT run the compiler on PS5 or load a console title. It checks a small
relocation-free ELF and a 32-bit-arithmetic oracle before the hardware gate.
The loader intentionally refuses imports/relocations: it is not a game loader.
"""
import argparse
import ctypes
from pathlib import Path
import json
import os
import struct
import subprocess
import time

ROOT=Path(__file__).resolve().parents[2]


def expected():
    value=0x12345678; result=0x811c9dc5; mask=0xffffffff
    for index in range(257):
        value=(value*1664525+1013904223)&mask
        rotated=((value<<7)|(value>>25))&mask
        chosen=rotated if value&0x80000000 else rotated^mask
        result=((result^((chosen+index)&mask))*16777619)&mask
    return result


def inspect(data):
    if len(data)<64 or data[:6]!=b'\x7fELF\x02\x01':raise ValueError('Not ELF64 LE')
    if struct.unpack_from('<HH',data,16)!=(3,62):raise ValueError('Not x86-64 ET_DYN')
    entry,offset=struct.unpack_from('<QQ',data,24)
    entsize,count=struct.unpack_from('<HH',data,54)
    if entsize!=56 or not 1<=count<=128 or offset+count*entsize>len(data):
        raise ValueError('Invalid program table')
    loads=[]
    for i in range(count):
        kind,flags,start,va,_,size,memory,alignment=struct.unpack_from('<IIQQQQQQ',data,offset+i*56)
        if start+size>len(data):raise ValueError('Segment outside file')
        if kind==1:
            if size>memory or va+memory>4*1024*1024 or alignment!=0x4000:
                raise ValueError('Unexpected load segment')
            if flags&2 and flags&1:raise ValueError('Writable executable segment')
            loads.append(dict(flags=flags,start=start,va=va,size=size,memory=memory))
        if kind==2:
            if size%16:raise ValueError('Invalid dynamic table')
            for pos in range(start,start+size,16):
                tag,value=struct.unpack_from('<QQ',data,pos)
                if tag==0:break
                if tag==1 or tag in (2,8,18) and value:
                    raise ValueError('Fixture must not need libraries or relocations')
    if not loads or not any(x['va']<=entry<x['va']+x['size'] and x['flags']&1 for x in loads):
        raise ValueError('Entry is outside code')
    return entry,loads


def execute(data,entry,loads,bridge_path):
    if os.name!='nt':raise RuntimeError('This host gate currently supports Windows only')
    kernel=ctypes.WinDLL('kernel32',use_last_error=True)
    kernel.VirtualAlloc.argtypes=[ctypes.c_void_p,ctypes.c_size_t,ctypes.c_ulong,ctypes.c_ulong]
    kernel.VirtualAlloc.restype=ctypes.c_void_p
    kernel.VirtualProtect.argtypes=[ctypes.c_void_p,ctypes.c_size_t,ctypes.c_ulong,ctypes.POINTER(ctypes.c_ulong)]
    kernel.VirtualProtect.restype=ctypes.c_int
    kernel.VirtualFree.argtypes=[ctypes.c_void_p,ctypes.c_size_t,ctypes.c_ulong]
    kernel.VirtualFree.restype=ctypes.c_int
    kernel.GetCurrentProcess.restype=ctypes.c_void_p
    kernel.FlushInstructionCache.argtypes=[ctypes.c_void_p,ctypes.c_void_p,ctypes.c_size_t]
    kernel.FlushInstructionCache.restype=ctypes.c_int
    extent=max(x['va']+x['memory'] for x in loads)
    base=kernel.VirtualAlloc(None,extent,0x3000,0x04)
    if not base:raise ctypes.WinError(ctypes.get_last_error())
    try:
        for item in loads:
            ctypes.memmove(base+item['va'],data[item['start']:item['start']+item['size']],item['size'])
        for item in loads:
            old=ctypes.c_ulong()
            protection=0x20 if item['flags']&1 else (0x04 if item['flags']&2 else 0x02)
            if not kernel.VirtualProtect(base+item['va'],item['memory'],protection,ctypes.byref(old)):
                raise ctypes.WinError(ctypes.get_last_error())
        if not kernel.FlushInstructionCache(kernel.GetCurrentProcess(),base,extent):
            raise ctypes.WinError(ctypes.get_last_error())
        bridge=ctypes.CDLL(str(bridge_path))
        bridge.sims2_probe_call.argtypes=[ctypes.c_void_p]
        bridge.sims2_probe_call.restype=ctypes.c_uint32
        return bridge.sims2_probe_call(base+entry)
    finally:
        if not kernel.VirtualFree(base,0,0x8000):raise ctypes.WinError(ctypes.get_last_error())


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--kit',required=True,type=Path)
    parser.add_argument('--output',default=ROOT/'out/console-compiler/host-probe',type=Path)
    a=parser.parse_args();kit=a.kit.resolve();out=a.output.resolve();out.mkdir(parents=True,exist_ok=True)
    result=dict(schema=1,compiler_runs_on='Windows',target='x86_64-sie-ps5',
                console_validation='NOT_TESTED',status='RUNNING',
                entry_abi='Windows bridge calling noinline SysV function',
                host_probe_red_zone=False)
    started=time.monotonic()
    try:
        commands=[
            [kit/'tools/clang-23.exe','-target','x86_64-sie-ps5','-O2','-std=c++17',
             '-fPIC','-fno-plt','-fno-stack-protector','-femulated-tls',
             '-fno-omit-frame-pointer','-fdenormal-fp-math=ieee','-march=x86-64-v2',
             '-fno-exceptions','-fno-rtti','-mno-red-zone',
             '-c',ROOT/'compiler/probe.cpp','-o',out/'probe.o'],
            [kit/'tools/ld.lld.exe','-shared','-e','sims2_compiler_probe','--no-undefined',
             '-z','max-page-size=0x4000',out/'probe.o','-o',out/'probe.elf'],
            [kit/'tools/clang-23.exe','-target','x86_64-pc-windows-msvc','-O2',
             '-fno-exceptions','-fno-rtti','-fno-stack-protector','-c',
             ROOT/'compiler/probe_windows_bridge.cpp','-o',out/'bridge.obj'],
            [kit/'tools/ld.lld.exe','-flavor','link','/dll','/noentry',
             '/out:'+str(out/'bridge.dll'),str(out/'bridge.obj')]]
        with (out/'build.log').open('w',encoding='utf-8') as log:
            for argv in commands:
                subprocess.run([str(x) for x in argv],stdout=log,stderr=subprocess.STDOUT,check=True,timeout=120)
        data=(out/'probe.elf').read_bytes();entry,loads=inspect(data)
        actual=execute(data,entry,loads,out/'bridge.dll');oracle=expected()
        result.update(actual=actual,expected=oracle,entry=entry,segments=loads,
                      status='PASS' if actual==oracle else 'FAIL')
        if actual!=oracle:raise RuntimeError('Compiled probe disagrees with integer oracle')
    except Exception as error:
        result.update(status='FAIL',error=str(error))
    result['seconds']=round(time.monotonic()-started,3)
    (out/'result.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result))
    if result['status']!='PASS':raise SystemExit(1)


if __name__=='__main__':main()
