// Copyright (C) 2026 OpenAI Codex, for szampan
// SPDX-License-Identifier: MIT
#pragma once
#include <algorithm>
#include <cstdint>
#include <span>
#include <vector>
#include <utility>
namespace gpu {
// Compare the complete serialized cache, including changes made by external
// Vulkan users. A failed disk write must never acknowledge a new snapshot.
class PipelineCacheSnapshot {
    std::vector<uint8_t> saved_;
    bool persisted_=false;
public:
    bool Matches(std::span<const uint8_t> bytes) const {
        return persisted_ && bytes.size()==saved_.size() && std::equal(bytes.begin(),bytes.end(),saved_.begin());
    }
    void Persisted(std::vector<uint8_t>&& bytes) {saved_=std::move(bytes);persisted_=true;}
    void WriteFailed() {persisted_=false;} // a failed truncating write may have damaged the old file
};
}
