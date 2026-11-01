#if defined(__PROSPERO__) || defined(SIMS2_PS5_MEMORY_MODEL)
#include "platform/ps5_memory.h"
#include "core/log.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <vector>
#include <ps5platform/shm.h>

namespace mem::ps5host {
namespace {
constexpr uint64_t kSize = 1ull << 32;
constexpr uint32_t kGuestPage = 0x1000, kHostPage = 0x4000;
constexpr uint32_t kPhysical = 0x80000000, kMirror = 0xF0000000, kRam = 64u << 20;
uint8_t* base = nullptr;
ps5_shm ram{};
bool physical_mapped = false, mirror_mapped = false;
bool initialized = false;
std::array<uint8_t, kSize / kGuestPage> committed{};

bool Physical(uint64_t va) {
    return (va >= kPhysical && va < uint64_t(kPhysical) + kRam) ||
           (va >= kMirror && va < uint64_t(kMirror) + kRam);
}

bool Bounds(uint32_t va, uint32_t bytes, uint64_t& first, uint64_t& end) {
    first = uint64_t(va) & ~uint64_t(kGuestPage - 1);
    end = (uint64_t(va) + bytes + kGuestPage - 1) & ~uint64_t(kGuestPage - 1);
    return base && bytes && end <= kSize;
}
}

bool Shutdown() {
    initialized = false;
    if (!base) return true;
    // Never release backing until its views have been unmapped successfully.
    if (mirror_mapped) {
        if (ps5_shm_unmap(base + kMirror, kRam, PS5_SHM_KEEP_RESERVED)) return false;
        mirror_mapped = false;
    }
    if (physical_mapped) {
        if (ps5_shm_unmap(base + kPhysical, kRam, PS5_SHM_KEEP_RESERVED)) return false;
        physical_mapped = false;
    }
    ps5_shm_destroy(&ram);
    if (ps5_vrange_release(base, kSize)) return false;
    base = nullptr;
    committed.fill(0);
    return true;
}

uint8_t* Init() {
    if (base) return initialized ? base : nullptr;
    void* range = nullptr;
    if (ps5_vrange_reserve(kSize, nullptr, 0x10000, &range)) return nullptr;
    base = static_cast<uint8_t*>(range);
    if (ps5_shm_create(kRam, &ram)) { Shutdown(); return nullptr; }
    void* view = nullptr;
    int result = ps5_shm_map(&ram, 0, kRam, base + kPhysical,
                            PS5_SHM_READ | PS5_SHM_WRITE, PS5_SHM_FIXED, &view);
    physical_mapped = result == 0;
    if (result || view != base + kPhysical) { Shutdown(); return nullptr; }
    result = ps5_shm_map(&ram, 0, kRam, base + kMirror,
                        PS5_SHM_READ | PS5_SHM_WRITE, PS5_SHM_FIXED, &view);
    mirror_mapped = result == 0;
    if (result || view != base + kMirror) { Shutdown(); return nullptr; }
    initialized = true;
    return base;
}

bool Commit(uint32_t va, uint32_t bytes) {
    if (!bytes) return base != nullptr;
    uint64_t first, end;
    if (!Bounds(va, bytes, first, end)) return false;
    // Native calls operate on 16 KiB pages. Record successful pages as we go:
    // if a later allocation fails, rollback only the pages newly committed by
    // this request, preserving already committed neighbours and their data.
    std::vector<uint64_t> newly_backed;
    for (uint64_t host = first & ~uint64_t(kHostPage - 1); host < end; host += kHostPage) {
        if (Physical(host)) continue;
        bool backed = false;
        for (uint64_t page = host; page < host+kHostPage; page += kGuestPage)
            backed |= committed[page/kGuestPage] != 0;
        if (ps5_vrange_commit(base + host, kHostPage, PS5_SHM_READ | PS5_SHM_WRITE)) {
            for (uint64_t rollback : newly_backed)
                if (ps5_vrange_decommit(base+rollback, kHostPage))
                    LOGE("memory: PS5 rollback failed at %08X; backing retained until shutdown",
                         static_cast<uint32_t>(rollback));
            // If the kernel refuses a rollback, no guest page is marked
            // committed; Shutdown still owns and releases the native range.
            return false;
        }
        if (!backed) newly_backed.push_back(host);
    }
    for (uint64_t host = first & ~uint64_t(kHostPage - 1); host < end; host += kHostPage) {
        if (Physical(host)) continue;
        // Previously uncommitted 4 KiB slices may contain writes from a sibling
        // due to native-page coalescing. New guest commits always read zero.
        for (uint64_t page = std::max(first, host); page < std::min(end, host+kHostPage); page += kGuestPage) {
            if (!committed[page / kGuestPage]) std::memset(base + page, 0, kGuestPage);
            committed[page / kGuestPage] = 1;
        }
    }
    return true;
}

bool Decommit(uint32_t va, uint32_t bytes) {
    if (!bytes) return base != nullptr;
    uint64_t first, end;
    if (!Bounds(va, bytes, first, end)) return false;
    for (uint64_t host = first & ~uint64_t(kHostPage - 1); host < end; host += kHostPage) {
        if (Physical(host)) continue;
        bool survives = false;
        for (uint64_t page = host; page < host+kHostPage; page += kGuestPage)
            survives |= committed[page/kGuestPage] && !(page >= first && page < end);
        if (!survives) {
            if (ps5_vrange_decommit(base+host, kHostPage)) return false;
        } else {
            for (uint64_t page = std::max(first, host); page < std::min(end, host+kHostPage); page += kGuestPage)
                if (committed[page/kGuestPage]) std::memset(base+page, 0, kGuestPage);
        }
        for (uint64_t page = std::max(first, host); page < std::min(end, host+kHostPage); page += kGuestPage)
            committed[page/kGuestPage] = 0;
    }
    return true;
}

bool IsCommitted(uint32_t va) { return base && (Physical(va) || committed[va/kGuestPage]); }
}
#endif
