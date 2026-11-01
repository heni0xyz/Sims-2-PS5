// PC-only diagnostics, OpenAI Codex for szampan. Captured XML stays private.
#if defined(SIMS2_HEADLESS_RUNTIME)
#include "kernel/kernel.h"
#include "core/log.h"
#include "gpu/shaders.h"
#include <mutex>
#include <string>
#include <unordered_set>
void TraceUiLayout(const X86Context& ctx,const std::string& operation,const std::string& xml) {
    if(xml.empty())return;
    if(operation.starts_with("Reflow") && xml.find("<x>")==xml.npos && xml.find("<w>")==xml.npos)return;
    static std::mutex lock;std::lock_guard held(lock);
    static std::unordered_set<uint64_t> seen;
    auto hash=gpu::Hash(xml.data(),xml.size());hash=gpu::Hash(operation.data(),operation.size(),hash);
    if(seen.size()>=512 || !seen.insert(hash).second)return;
    const unsigned record=unsigned(seen.size());
    constexpr size_t chunk=2500;
    const auto parts=(xml.size()+chunk-1)/chunk;
    for(size_t i=0;i<parts;i++)
        LOGR("[SIMS2][UI_LAYOUT][%s] record=%u part=%zu/%zu caller=%08x bytes=%zu xml=%.*s",
            operation.c_str(),record,i+1,parts,kernel::R32(ctx.esp),xml.size(),
            int(std::min(chunk,xml.size()-i*chunk)),xml.c_str()+i*chunk);
}
#endif
