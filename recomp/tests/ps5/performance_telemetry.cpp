// Copyright (C) 2026 OpenAI Codex, for szampan
// SPDX-License-Identifier: GPL-3.0-only
// Asset-free telemetry math/lifecycle test, never a console FPS benchmark.
#include "core/deep_profile.h"
#include "core/guest_profile.h"
#include "core/log.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <thread>
#include <chrono>
#include <limits>
namespace logx {
void Write(Level,const char* format,...) {
    va_list args;va_start(args,format);std::vprintf(format,args);va_end(args);std::puts("");
}
}
void Require(bool pass,const char* text) {
    if(!pass){std::fprintf(stderr,"FAIL: %s\n",text);std::exit(1);}
}
int main() {
    std::puts("SYNTHETIC HOST UNIT TEST: no game, GPU or console measurements");
    double ns=0;
    Require(deep_profile::TimestampNs(250,5,8,2,ns) && ns==22,"timestamp counter wrap");
    Require(deep_profile::TimestampNs(UINT64_MAX-1,2,64,1,ns) && ns==4,"64-bit counter wrap");
    Require(!deep_profile::TimestampNs(0,1,0,1,ns),"unsupported timestamps rejected");
    Require(!deep_profile::TimestampNs(0,1,32,std::numeric_limits<double>::quiet_NaN(),ns),"invalid period rejected");
    deep_profile::RecordFrameInterval(10000000);
    deep_profile::RecordFrameInterval(20000000);
    deep_profile::RecordFrameInterval(100000000);
    const auto frames=deep_profile::DrainFrames();
    Require(frames.count==3 && frames.p50_ms==20 && frames.p99_ms==100 && frames.max_ms==100,"slow frame survives percentile aggregation");
    Require(std::fabs(frames.mean_ms-130./3)<1e-6 && frames.low1_fps==10,"frame-weighted mean and slowest one percent");
    Require(deep_profile::DrainFrames().count==0,"report drains samples exactly once");
    deep_profile::RegisterThread(0x1234);
    {
        deep_profile::Scope scope(deep_profile::Metric::Kernel,0,true);
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    Require(deep_profile::LocalStats(deep_profile::Metric::Kernel).calls==1,"enabled scopes record completed calls");
    Require(guest_profile::Record(guest_profile::Kind::Indirect,0x123456,1000,0x1234),"guest sample accepted");
    Require(guest_profile::Record(guest_profile::Kind::Indirect,0x123456,3000,0x1234),"second guest sample accepted");
    const auto guest=guest_profile::Drain();
    Require(guest.samples==2 && guest.top_count==1 && guest.top[0].wall_ns==4000 && guest.top[0].max_ns==3000,"sample totals and maximum retained");
    Require(guest_profile::Drain().samples==0,"guest report drains once");
    std::this_thread::sleep_for(std::chrono::milliseconds(5100));
    deep_profile::Poll();deep_profile::ReportIfDue();
    std::puts("PERFORMANCE_TELEMETRY_PASS (synthetic host test only)");
}
