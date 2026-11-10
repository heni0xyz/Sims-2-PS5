// Copyright (C) 2026 OpenAI Codex, for szampan
// SPDX-License-Identifier: MIT
#include "gpu/cache_file.h"
#include "core/perf_clock.h"
#include <atomic>
#include <fstream>
#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <process.h>
#else
#include <unistd.h>
#endif
namespace gpu {
bool WriteCacheFile(const std::filesystem::path& path,std::span<const uint8_t> bytes) {
    std::error_code error;
    std::filesystem::create_directories(path.parent_path(),error);
    if(error)return false;
    auto temporary=path;
    static std::atomic<uint64_t> sequence{0};
#if defined(_WIN32)
    temporary+=".tmp."+std::to_string(static_cast<long long>(_getpid()));
#else
    temporary+=".tmp."+std::to_string(static_cast<long long>(getpid()));
#endif
    temporary+="."+std::to_string(sequence.fetch_add(1,std::memory_order_relaxed));
    bool good=false;
    {
        std::ofstream stream(temporary,std::ios::binary|std::ios::trunc);
        if(stream){stream.write(reinterpret_cast<const char*>(bytes.data()),std::streamsize(bytes.size()));
            stream.flush();good=stream.good();stream.close();good=good && !stream.fail();}
    }
    if(!good){std::filesystem::remove(temporary,error);return false;}
#if defined(_WIN32)
    good=MoveFileExW(temporary.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING)!=0;
#else
    std::filesystem::rename(temporary,path,error);good=!error;
#endif
    if(!good)std::filesystem::remove(temporary,error);
    return good;
}
PipelineCacheWriter::PipelineCacheWriter(std::filesystem::path path,Write write)
    :path_(std::move(path)),write_(std::move(write)),thread_([this]{Run();}) {}
PipelineCacheWriter::~PipelineCacheWriter(){Stop();}
PipelineCacheWriter::Outcome PipelineCacheWriter::Submit(std::vector<uint8_t>&& bytes) {
    std::lock_guard lock(mutex_);
    if(stopping_)return Outcome::Stopped;
    // A newer pending/active write takes priority over a matching old file.
    const bool same=pending_?*pending_==bytes:active_?*active_==bytes:saved_.Matches(bytes);
    if(same){++stats_.skipped;return Outcome::Unchanged;}
    if(pending_)++stats_.coalesced;
    pending_=std::make_shared<std::vector<uint8_t>>(std::move(bytes));++stats_.queued;
    condition_.notify_one();return Outcome::Queued;
}
PipelineCacheWriter::Stats PipelineCacheWriter::ReadStats(){std::lock_guard lock(mutex_);return stats_;}
void PipelineCacheWriter::Stop(){
    {std::lock_guard lock(mutex_);stopping_=true;}
    condition_.notify_one();if(thread_.joinable())thread_.join();
}
void PipelineCacheWriter::Run(){
    for(;;){
        std::shared_ptr<std::vector<uint8_t>> blob;
        {
            std::unique_lock lock(mutex_);condition_.wait(lock,[&]{return stopping_ || pending_;});
            if(!pending_)return;
            active_=std::exchange(pending_,{});blob=active_;
        }
        const auto start=perf_clock::Clock::now();const bool good=write_(path_,*blob);
        const double ms=std::chrono::duration<double,std::milli>(perf_clock::Clock::now()-start).count();
        {
            std::lock_guard lock(mutex_);stats_.last_write_ms=ms;
            if(good){saved_.Persisted(std::move(*blob));++stats_.writes;}
            else {saved_.WriteFailed();++stats_.failures;}
            active_.reset();
        }
    }
}
}
