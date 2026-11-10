// Copyright (C) 2026 OpenAI Codex, for szampan
// SPDX-License-Identifier: MIT
#pragma once
#include "gpu/pipeline_cache_snapshot.h"
#include <condition_variable>
#include <filesystem>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
namespace gpu {
// Checked same-directory temporary + atomic replacement; keeps an existing
// cache intact if writing or promotion fails. No Vulkan calls on the worker.
bool WriteCacheFile(const std::filesystem::path&,std::span<const uint8_t>);
class PipelineCacheWriter {
public:
    using Write=std::function<bool(const std::filesystem::path&,std::span<const uint8_t>)>;
    enum class Outcome {Queued,Unchanged,Stopped};
    struct Stats {uint64_t queued=0,coalesced=0,skipped=0,writes=0,failures=0;double last_write_ms=0;};
    explicit PipelineCacheWriter(std::filesystem::path,Write=WriteCacheFile);
    ~PipelineCacheWriter();
    Outcome Submit(std::vector<uint8_t>&&);
    Stats ReadStats();
    void Stop(); // drains active + latest pending snapshot, idempotent
private:
    void Run();
    std::filesystem::path path_;
    Write write_;
    std::mutex mutex_;
    std::condition_variable condition_;
    bool stopping_=false;
    PipelineCacheSnapshot saved_;
    std::shared_ptr<std::vector<uint8_t>> pending_,active_;
    Stats stats_;
    std::thread thread_;
};
}
