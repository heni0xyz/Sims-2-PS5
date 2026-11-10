// Copyright (C) 2026 OpenAI Codex, for szampan
// SPDX-License-Identifier: GPL-3.0-only
#include "core/perf_clock.h"
#include <cstdio>
#include <cstdlib>
#include <thread>
void Check(bool ok,const char* message) {if(!ok){std::fprintf(stderr,"FAIL %s\n",message);std::exit(1);}}
int main() {
    using perf_clock::TicksToNs;
    Check(TicksToNs(0,1596300201)==0,"zero elapsed");
    Check(TicksToNs(1596300201,1596300201)==1000000000ull,"frequency conversion");
    Check(TicksToNs(5,3)==1666666666ull,"fractional nanoseconds truncated");
    Check(TicksToNs(UINT64_MAX,UINT64_MAX)==1000000000ull,"wide multiplication does not wrap");
    Check(TicksToNs(UINT64_MAX,1)==uint64_t(INT64_MAX),"chrono duration saturates safely");
    Check(TicksToNs(1,0)==0,"unavailable frequency guarded");
    const auto begin=perf_clock::Clock::now();
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
    Check(perf_clock::Clock::now()>=begin,"host fallback monotonic");
    std::puts("PERF_CLOCK_PASS (conversion and host fallback; not a console benchmark)");
}
