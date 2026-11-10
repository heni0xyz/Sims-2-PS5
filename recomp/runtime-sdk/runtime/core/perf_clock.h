// Copyright (C) 2026 OpenAI Codex, for szampan
// SPDX-License-Identifier: MIT
#pragma once
#include <chrono>
#include <cstdint>
#include <limits>
#if defined(__PROSPERO__)
#include <ps5platform/kernel.h>
#endif

// Observation clock only. Never use this epoch for guest time, deadlines,
// simulation, condition-variable waits or frame pacing.
namespace perf_clock {
inline uint64_t TicksToNs(uint64_t ticks,uint64_t frequency) noexcept {
    if(!frequency)return 0;
    constexpr uint64_t limit=uint64_t(std::numeric_limits<int64_t>::max());
#if defined(__SIZEOF_INT128__)
    const auto value=static_cast<unsigned __int128>(ticks)*1000000000ull/frequency;
    return value>limit?limit:uint64_t(value);
#else
    const auto seconds=ticks/frequency,remainder=ticks%frequency;
    if(seconds>limit/1000000000ull)return limit;
    const uint64_t whole=seconds*1000000000ull;
    const uint64_t fraction=uint64_t(static_cast<long double>(remainder)*1000000000.0L/frequency);
    return fraction>limit-whole?limit:whole+fraction;
#endif
}
struct Clock {
    using duration=std::chrono::nanoseconds;
    using rep=duration::rep;
    using period=duration::period;
    using time_point=std::chrono::time_point<Clock>;
    static constexpr bool is_steady=true;
    static time_point now() noexcept {
#if defined(__PROSPERO__)
        struct Epoch {uint64_t frequency=sceKernelGetTscFrequency(),base=sceKernelReadTsc();};
        static const Epoch epoch;
        if(epoch.frequency)return time_point(duration(TicksToNs(sceKernelReadTsc()-epoch.base,epoch.frequency)));
#endif
        return time_point(std::chrono::duration_cast<duration>(std::chrono::steady_clock::now().time_since_epoch()));
    }
};
}
