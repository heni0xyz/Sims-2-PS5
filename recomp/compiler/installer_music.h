// Copyright (C) 2026 OpenAI Codex, for szampan
// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <cstdint>
#include <cstddef>
#include <vector>
#include <atomic>

// Decode once before AudioOut starts. Fill has no allocation, I/O or locks.
// Lifetime: stop/join the foundation audio thread before destroying this object.
class Sims2InstallerMusic {
    std::vector<int16_t> pcm;
    double position=0;
    int rate=0,channels=0;
    std::atomic<bool> enabled{true};
    double gain=.10;
public:
    void SetEnabled(bool value) {enabled.store(value,std::memory_order_relaxed);}
    bool Enabled() const {return enabled.load(std::memory_order_relaxed);}
    bool Load(const char *path);
    void Fill(int16_t *stereo,int frames);
    size_t Frames() const {return channels?pcm.size()/channels:0;}
    int Rate() const {return rate;}
    static void Callback(int16_t *output,int frames,void *user) {
        static_cast<Sims2InstallerMusic*>(user)->Fill(output,frames);
    }
};
