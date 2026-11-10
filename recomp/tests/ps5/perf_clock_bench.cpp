// Copyright (C) 2026 OpenAI Codex, for szampan
// SPDX-License-Identifier: GPL-3.0-only
// Asset-free bounded payload. Measures clocks, never game performance.
#include <chrono>
#include <cstdio>
#include <cstdint>
#include <ps5platform/kernel.h>
#include "core/perf_clock.h"
extern "C" int sceKernelUsleep(unsigned);
int main() {
    const uint64_t hz=sceKernelGetTscFrequency(),base=sceKernelReadTsc();
    if(!hz){std::puts("CLOCK_BENCH_FAIL unavailable_frequency");return 1;}
    const auto ns=[&](uint64_t tick){return uint64_t(static_cast<unsigned __int128>(tick-base)*1000000000ull/hz);};
    constexpr unsigned count=100000;uint64_t checksum=0;
    const auto begin=sceKernelReadTsc();
    for(unsigned i=0;i<count;++i)checksum^=std::chrono::steady_clock::now().time_since_epoch().count();
    const auto steady_end=sceKernelReadTsc();
    for(unsigned i=0;i<count;++i)checksum^=perf_clock::Clock::now().time_since_epoch().count();
    const auto tsc_end=sceKernelReadTsc();
    const auto wall_start=std::chrono::steady_clock::now();const auto tick_start=sceKernelReadTsc();
    const auto fast_start=perf_clock::Clock::now();
    sceKernelUsleep(100000);
    const auto fast_stop=perf_clock::Clock::now();
    const auto tick_stop=sceKernelReadTsc();const auto wall_stop=std::chrono::steady_clock::now();
    const auto wall_ns=std::chrono::duration_cast<std::chrono::nanoseconds>(wall_stop-wall_start).count();
    const uint64_t tsc_ns=ns(tick_stop)-ns(tick_start);
    const auto fast_ns=(fast_stop-fast_start).count();
    std::printf("CLOCK_BENCH hz=%llu count=%u steady_read_ns=%.3f tsc_convert_read_ns=%.3f elapsed_steady_ns=%lld elapsed_tsc_ns=%llu elapsed_fast_ns=%lld checksum=%llu\n",
        (unsigned long long)hz,count,double(ns(steady_end)-ns(begin))/count,double(ns(tsc_end)-ns(steady_end))/count,
        (long long)wall_ns,(unsigned long long)tsc_ns,(long long)fast_ns,(unsigned long long)checksum);
    const bool good=wall_ns>0 && tsc_ns>uint64_t(wall_ns)*98/100 && tsc_ns<uint64_t(wall_ns)*102/100 &&
        fast_ns>wall_ns*98/100 && fast_ns<wall_ns*102/100;
    std::puts(good?"CLOCK_BENCH_PASS":"CLOCK_BENCH_FAIL clock_disagreement");std::fflush(stdout);return good?0:1;
}
