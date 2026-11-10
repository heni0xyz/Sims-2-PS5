// Copyright (C) 2026 OpenAI Codex, for szampan
// SPDX-License-Identifier: GPL-3.0-only
#include "gpu/cache_file.h"
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <future>
#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif
using namespace std::chrono_literals;
void Check(bool ok,const char* text){if(!ok){std::fprintf(stderr,"FAIL %s\n",text);std::exit(1);}}
int main(int argc,char** argv){
    Check(argc==2,"supply a fresh owned temporary directory");
    const std::filesystem::path root=argv[1];Check(!std::filesystem::exists(root),"temporary directory must be fresh");
    std::filesystem::create_directories(root);const auto file=root/"pipeline.bin";
    const std::vector<uint8_t> a{1,0,2,255},b{8,7,0,9};
    const auto read=[&]{std::ifstream f(file,std::ios::binary);return std::vector<uint8_t>(std::istreambuf_iterator<char>(f),{});};
    Check(gpu::WriteCacheFile(file,a) && read()==a,"first atomic persistence");
    Check(gpu::WriteCacheFile(file,b) && read()==b,"atomic replacement of existing file");
#if defined(_WIN32)
    HANDLE held=CreateFileW(file.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    Check(held!=INVALID_HANDLE_VALUE,"lock destination against rename");
    Check(!gpu::WriteCacheFile(file,a),"failed promotion is reported");CloseHandle(held);
    Check(read()==b,"failed promotion preserves old file");
#endif
    std::mutex mutex;std::condition_variable cv;bool release=false;std::promise<void> started;
    auto startedFuture=started.get_future();std::vector<unsigned> writes;
    gpu::PipelineCacheWriter writer(file,[&](const auto&,std::span<const uint8_t> data){
        writes.push_back(data[0]);if(writes.size()==1){started.set_value();std::unique_lock lock(mutex);cv.wait(lock,[&]{return release;});}return true;});
    Check(writer.Submit({1})==gpu::PipelineCacheWriter::Outcome::Queued,"first snapshot queued");
    Check(startedFuture.wait_for(2s)==std::future_status::ready,"background write reached gate");
    Check(writer.Submit({1})==gpu::PipelineCacheWriter::Outcome::Unchanged,"in-flight duplicate skipped");
    Check(writer.Submit({2})==gpu::PipelineCacheWriter::Outcome::Queued,"render submit proceeds while disk blocked");
    Check(writer.Submit({3})==gpu::PipelineCacheWriter::Outcome::Queued,"newest pending snapshot replaces older one");
    auto stopped=std::async(std::launch::async,[&]{writer.Stop();});
    Check(stopped.wait_for(10ms)==std::future_status::timeout,"shutdown drains rather than abandoning active write");
    {std::lock_guard lock(mutex);release=true;}cv.notify_one();
    Check(stopped.wait_for(2s)==std::future_status::ready,"shutdown completed after disk released");stopped.get();
    Check(writes==std::vector<unsigned>({1,3}),"bounded latest-only queue preserves first and newest snapshots");
    const auto stats=writer.ReadStats();Check(stats.writes==2 && stats.coalesced==1 && stats.failures==0,"write outcomes recorded");
    Check(writer.Submit({4})==gpu::PipelineCacheWriter::Outcome::Stopped,"closed writer rejects new work");
    std::atomic<unsigned> attempts{0};
    gpu::PipelineCacheWriter retry(file,[&](const auto&,auto){return ++attempts>1;});retry.Submit({7});
    const auto deadline=std::chrono::steady_clock::now()+2s;
    while(!retry.ReadStats().failures && std::chrono::steady_clock::now()<deadline)std::this_thread::yield();
    Check(retry.ReadStats().failures==1,"failed write reported without spinning");
    Check(retry.Submit({7})==gpu::PipelineCacheWriter::Outcome::Queued,"failed snapshot remains retryable");
    retry.Stop();Check(retry.ReadStats().writes==1 && attempts==2,"retry persisted exactly once");
    std::filesystem::remove(file);Check(std::filesystem::is_empty(root),"no abandoned temporary files");std::filesystem::remove(root);
    std::puts("ASYNC_CACHE_PASS: atomic replacement, failure preservation, nonblocking producer, bounded coalescing, drain and retry");
}
