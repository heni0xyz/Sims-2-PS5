// Copyright (C) 2026 OpenAI Codex, for szampan
// SPDX-License-Identifier: GPL-3.0-only
#include "gpu/pipeline_cache_snapshot.h"
#include <cstdio>
#include <cstdlib>
void Check(bool ok,const char* text){if(!ok){std::fprintf(stderr,"FAIL %s\n",text);std::exit(1);}}
int main(){
    gpu::PipelineCacheSnapshot cache;
    const std::vector<uint8_t> original{1,0,2,255},changed{1,0,3,255},shorter{1,0,2};
    Check(!cache.Matches(original),"initial cache must be persisted");
    Check(!cache.Matches(original),"failed/unacknowledged write remains retryable");
    cache.Persisted(std::vector<uint8_t>(original));
    Check(cache.Matches(original),"successful identical cache skips rewrite");
    Check(!cache.Matches(changed),"changed bytes including external updates are persisted");
    Check(!cache.Matches(shorter),"different cache extent is not equal");
    cache.WriteFailed();
    Check(!cache.Matches(original) && !cache.Matches(changed),"failed truncating write invalidates even the old acknowledged bytes");
    cache.Persisted(std::vector<uint8_t>(changed));
    Check(cache.Matches(changed) && !cache.Matches(original),"only successful new write advances snapshot");
    std::puts("PIPELINE_CACHE_SNAPSHOT_PASS (host policy; real disk timing requires console)");
}
