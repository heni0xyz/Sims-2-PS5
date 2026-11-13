// OpenAI Codex for szampan, 31 October 2026.
// USA 0MOD_head callback 0x147D60 dispatches actor->5EC virtual method +8
// (0x67D60). That method sets a separate normalized camera rectangle through
// 0x1CBC20, called at 0x67E67. It queues rendering on another game thread;
// a thread-local GPU annotation cannot follow that queue. Transform the
// rectangle before the original setter computes/copies the camera state.
// No extracted instructions, generated source or artwork are embedded.
#include "kernel/kernel.h"
#include "game/ui_aspect.h"
#include "game/ui_layout.h"
#include "game/hud_portrait.h"
#include "app/app.h"
#include "core/log.h"
#include "cpu/x86_ops.h"
#include <atomic>
#include <cstdlib>
#include <cstring>
#if defined(SIMS2_HEADLESS_RUNTIME) || defined(SIMS2_NATIVE_THUMBNAILS)
#include <algorithm>
#include <array>
#include <chrono>
#include <mutex>
#include <vector>
#include "xdk/d3d8.h"
#if defined(_WIN32)
#include <windows.h>
#endif
#if defined(XBE_VULKAN_HOST) || defined(SIMS2_NATIVE_THUMBNAILS)
namespace gpu {
void SetSavedPortraitProbe(uint32_t sim, const uint8_t* indices);
void BeginGeneratedPortrait();
void EndGeneratedPortrait(const uint8_t* indices);
void BeginCasPortrait();
void EndCasPortrait(std::vector<uint8_t> pixels);
}
namespace {
bool AutoSavedPortraits();
thread_local bool tracingSavedRender = false;
bool ReadGeneratedPortraitIndices(const X86Context& ctx, uint8_t* base, uint32_t bitmap,
                                  std::array<uint8_t, 1024>& indices);
std::vector<uint8_t> ReadGeneratedCasPortrait(const X86Context& ctx, uint8_t* base, uint32_t bitmap);
std::mutex generatedPortraitLock;
}
#else
namespace {
thread_local bool tracingSavedRender = false;
}
#endif
#endif
X86_EXTERN_FUNC(__imp__sub_001CBC20);
X86_FUNC(sub_001CBC20) {
    if(!sims2::ui::enabled || !sims2::ui::native_portrait || !sims2::ui::single_player_hud ||
       app::Get().aspect_ratio!="16:9" || kernel::R32(ctx.esp)!=0x00067E6C) {
        __imp__sub_001CBC20(ctx,base);return;
    }
    const uint32_t pointer=ctx.ecx;
    float original[4],mapped[4];
    std::memcpy(original,kernel::HostPtr(pointer),sizeof(original));
    std::memcpy(mapped,original,sizeof(mapped));
    if(!sims2::ui::MapPortraitRectangle(mapped,sims2::ui::edge_anchors)) {
        __imp__sub_001CBC20(ctx,base);return;
    }
    static std::atomic<bool> reported{false};
    if(!reported.exchange(true))LOGR("[SIMS2][UI][INFO] native portrait camera rect x=%.8f..%.8f -> %.8f..%.8f y=%.8f..%.8f anchor=%s",
        original[0],original[2],mapped[0],mapped[2],original[1],original[3],sims2::ui::edge_anchors?"left":"center");
    std::memcpy(kernel::HostPtr(pointer),mapped,sizeof(mapped));
    try {__imp__sub_001CBC20(ctx,base);}
    catch(...) {std::memcpy(kernel::HostPtr(pointer),original,sizeof(original));throw;}
    std::memcpy(kernel::HostPtr(pointer),original,sizeof(original));
}

#if defined(SIMS2_HEADLESS_RUNTIME) || defined(SIMS2_NATIVE_THUMBNAILS)
// Local evidence for the USA thumbnail builder, including its nested 32x32 path.
// Diagnostic only. Authored by OpenAI Codex for szampan.
X86_EXTERN_FUNC(__imp__sub_00068EE0);
X86_FUNC(sub_00068EE0) {
#if defined(SIMS2_NATIVE_THUMBNAILS)
    constexpr bool trace = true;
    constexpr bool traceArtwork = false;
#else
    static const bool trace = std::getenv("SIMS2_UI_LAYOUT_TRACE") != nullptr;
    static const bool traceArtwork = std::getenv("SIMS2_ARTWORK_RASTER_TRACE") != nullptr;
#endif
    if (!trace) {
        __imp__sub_00068EE0(ctx, base);
        return;
    }
    const auto caller = kernel::R32(ctx.esp);
    const auto width = kernel::R32(ctx.ecx);
    const auto height = kernel::R32(ctx.ecx + 4);
    const auto bits = kernel::R32(ctx.ecx + 8);
    auto pass = d3d::PortraitRaster::None;
    // The detached character renderer also produces indexed 32px thumbnails.
    if ((caller == 0x0003b771 || caller == 0x001885e4) && width == 32 && height == 32 && bits == 8) {
        pass = d3d::PortraitRaster::Thumbnail;
    } else if (caller == 0x0011d0d0 && width == 128 && height == 128 && bits == 32) {
        pass = d3d::PortraitRaster::CasPortrait;
    } else if (traceArtwork && caller == 0x00069cfc && width == 128 && height == 128 && bits == 16) {
        pass = d3d::PortraitRaster::Artwork;
    }
    struct PortraitScope {
        d3d::PortraitRaster pass;
        explicit PortraitScope(d3d::PortraitRaster pass) : pass(pass) {
            d3d::BeginPortraitRaster(pass);
        }
        ~PortraitScope() { d3d::EndPortraitRaster(pass); }
    } scope{pass};
#if defined(XBE_VULKAN_HOST) || defined(SIMS2_NATIVE_THUMBNAILS)
    if (pass == d3d::PortraitRaster::CasPortrait && AutoSavedPortraits()) {
        std::lock_guard lock(generatedPortraitLock);
        struct CasPortrait {
            CasPortrait() { gpu::BeginCasPortrait(); }
            ~CasPortrait() { gpu::EndCasPortrait(std::move(pixels)); }
            std::vector<uint8_t> pixels;
        } portrait;
        __imp__sub_00068EE0(ctx, base);
        portrait.pixels = ReadGeneratedCasPortrait(ctx, base, ctx.eax);
        return;
    }
    if (pass == d3d::PortraitRaster::Thumbnail && !tracingSavedRender && AutoSavedPortraits()) {
        std::lock_guard lock(generatedPortraitLock);
        struct GeneratedPortrait {
            GeneratedPortrait() { gpu::BeginGeneratedPortrait(); }
            ~GeneratedPortrait() { gpu::EndGeneratedPortrait(valid ? indices.data() : nullptr); }
            std::array<uint8_t, 1024> indices{};
            bool valid{};
        } generated;
        __imp__sub_00068EE0(ctx, base);
        generated.valid = ReadGeneratedPortraitIndices(ctx, base, ctx.eax, generated.indices);
        return;
    }
#endif
    __imp__sub_00068EE0(ctx, base);
    LOGR("[SIMS2][PORTRAIT][TRACE] caller=%08x request=%ux%u bits=%u resource=%08x",
        caller, width, height, bits, ctx.eax);
}

X86_EXTERN_FUNC(__imp__sub_0002A300);
X86_FUNC(sub_0002A300) {
    const uint32_t settings = ctx.eax;
    __imp__sub_0002A300(ctx, base);
    static const bool skip = std::getenv("SIMS2_TEST_SKIP_STARTUP_MOVIES") != nullptr;
    if (skip && settings) {
        *static_cast<uint8_t*>(kernel::HostPtr(settings + 0x381)) = 1;
        LOG_FIRST(1, "[SIMS2][PORTRAIT][TRACE] enabled guest skip_startup_movies for replay");
    }
}

X86_EXTERN_FUNC(__imp__sub_000466C0);
X86_FUNC(sub_000466C0) {
    static const bool trace = std::getenv("SIMS2_UI_LAYOUT_TRACE") != nullptr;
    const bool missingThumbnail = trace && kernel::R32(ctx.esp) == 0x000d22e1;
    const uint32_t record = ctx.eax;
    if (missingThumbnail) {
        LOGR("[SIMS2][PORTRAIT][TRACE] missing saved thumbnail record=%08x fallback entered", record);
    }
    __imp__sub_000466C0(ctx, base);
    if (missingThumbnail) {
        LOGR("[SIMS2][PORTRAIT][TRACE] missing saved thumbnail record=%08x regenerated=%u",
            record, ctx.eax & 0xff);
    }
}

X86_EXTERN_FUNC(__imp__sub_000D27E0);
X86_EXTERN_FUNC(__imp__sub_000A4F40);
X86_EXTERN_FUNC(__imp__sub_00048850);
X86_EXTERN_FUNC(__imp__sub_001805D0);
X86_EXTERN_FUNC(__imp__sub_00068A50);
X86_EXTERN_FUNC(__imp__sub_00068D80);
X86_EXTERN_FUNC(__imp__sub_00068B40);
X86_EXTERN_FUNC(__imp__sub_00069ED0);
X86_EXTERN_FUNC(__imp__sub_00069FF0);
X86_EXTERN_FUNC(__imp__sub_0006A060);
X86_EXTERN_FUNC(__imp__sub_0006A400);
X86_EXTERN_FUNC(__imp__sub_0006B3E0);
X86_EXTERN_FUNC(__imp__sub_0006C010);
X86_EXTERN_FUNC(__imp__sub_0006BB40);
X86_EXTERN_FUNC(__imp__sub_0006A7F0);
X86_EXTERN_FUNC(__imp__sub_0006AED0);

namespace {
using PortraitClock = std::chrono::steady_clock;
struct SavedPortraitRequest {
    uint32_t neighborhood{};
    uint32_t sim{};
    std::array<uint8_t, 1024> indices{};
};
std::mutex savedPortraitLock;
std::vector<SavedPortraitRequest> savedPortraitQueue;
PortraitClock::time_point nextSavedPortrait{};
size_t savedPortraitCursor = 0;
bool savedPortraitInFlight = false;
bool AutoSavedPortraits() {
#if defined(SIMS2_NATIVE_THUMBNAILS)
    return true;
#elif defined(XBE_VULKAN_HOST)
    static const bool enabled = std::getenv("SIMS2_TEST_AUTO_FAMILY_PORTRAITS") != nullptr;
    return enabled;
#else
    return false;
#endif
}

void SwizzlePortraitIndices(const uint8_t* linear, uint32_t pitch,
                            std::array<uint8_t, 1024>& indices) {
    for (unsigned y = 0; y < 32; ++y) {
        for (unsigned x = 0; x < 32; ++x) {
            unsigned offset = 0;
            for (unsigned bit = 0; bit < 5; ++bit) {
                offset |= (((x >> bit) & 1u) | (((y >> bit) & 1u) << 1)) << (2 * bit);
            }
            indices[offset] = linear[y * pitch + x];
        }
    }
}

void QueueSavedPortrait(uint32_t neighborhood, uint32_t sim, uint32_t chunk) {
    if (!neighborhood || !chunk) return;
    const uint32_t allocatedBytes = kernel::R32(chunk);
    const uint32_t payload = kernel::R32(chunk + 4);
    constexpr uint32_t count = 32 * 32;
    constexpr uint32_t headerBytes = 12;
    constexpr uint32_t bitmapThumbnail = 0x74686d62;
    if (!payload || allocatedBytes < headerBytes + count ||
        payload > UINT32_MAX - headerBytes - count ||
        !mem::IsCommitted(payload) || !mem::IsCommitted(payload + headerBytes + count - 1) ||
        kernel::R32(payload + 8) != bitmapThumbnail) return;
    const auto* linear = static_cast<const uint8_t*>(kernel::HostPtr(payload + headerBytes));
    SavedPortraitRequest request{neighborhood, sim};
    SwizzlePortraitIndices(linear, 32, request.indices);
    std::lock_guard lock(savedPortraitLock);
    if (!savedPortraitQueue.empty() && savedPortraitQueue.front().neighborhood != neighborhood) {
        savedPortraitQueue.clear();
        savedPortraitCursor = 0;
    }
    if (savedPortraitQueue.size() >= 256 ||
        std::any_of(savedPortraitQueue.begin(), savedPortraitQueue.end(),
            [&request](const SavedPortraitRequest& entry) {
                return entry.sim == request.sim && entry.indices == request.indices;
            })) return;
    savedPortraitQueue.push_back(request);
    nextSavedPortrait = PortraitClock::now() + std::chrono::seconds(2);
}

std::atomic<uint32_t> pendingSavedPortrait{0};

#if defined(_WIN32)
LONG CALLBACK TraceSavedPortraitFault(EXCEPTION_POINTERS* fault) {
    if (fault->ExceptionRecord->ExceptionCode != EXCEPTION_ACCESS_VIOLATION) return EXCEPTION_CONTINUE_SEARCH;
    const auto pc = reinterpret_cast<uintptr_t>(fault->ExceptionRecord->ExceptionAddress);
    const auto image = reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
    LOGR("[SIMS2][PORTRAIT][TRACE] Windows fault image-offset=%llx guest-base=%p address=%llx",
        static_cast<unsigned long long>(pc - image), kernel::g_base,
        static_cast<unsigned long long>(fault->ExceptionRecord->ExceptionInformation[1]));
    return EXCEPTION_CONTINUE_SEARCH;
}
#endif

void PortraitGuestPush(X86Context& state, uint32_t value) {
    state.esp -= 4;
    kernel::W32(state.esp, value);
}

void PortraitGuestCall(X86Func* function, X86Context& state, uint8_t* base, uint32_t returnPc) {
    PortraitGuestPush(state, returnPc);
    function(state, base);
}

#if defined(XBE_VULKAN_HOST) || defined(SIMS2_NATIVE_THUMBNAILS)
template<class Read>
bool ReadGeneratedPortrait(const X86Context& original, uint8_t* base, uint32_t bitmap,
                           uint32_t rowBytes, uint32_t rows, Read read) {
    if (!bitmap) return false;
    auto state = original;
    state.esp -= 8;
    const uint32_t pitchSlot = state.esp;
    const uint32_t auxiliarySlot = pitchSlot + 4;
    kernel::W32(pitchSlot, 0);
    kernel::W32(auxiliarySlot, 0);
    const uint32_t methods = kernel::R32(bitmap);
    const auto invoke = [&](uint32_t method, uint32_t returnPc) {
        state.ecx = bitmap;
        PortraitGuestPush(state, returnPc);
        x86_icall(state, base, kernel::R32(methods + method));
    };
    PortraitGuestPush(state, 1);
    invoke(0x18, 0x000d2738);
    PortraitGuestPush(state, auxiliarySlot);
    PortraitGuestPush(state, pitchSlot);
    PortraitGuestPush(state, 0);
    invoke(0x1c, 0x000d274d);
    const uint32_t pixels = state.eax;
    const uint32_t pitch = kernel::R32(pitchSlot);
    const uint32_t lastByte = pitch <= 4096 ? pitch * (rows - 1) + rowBytes - 1 : 0;
    const bool readable = pixels && pitch >= rowBytes && lastByte &&
        pixels <= UINT32_MAX - lastByte &&
        mem::IsCommitted(pixels) && mem::IsCommitted(pixels + lastByte);
    if (readable) read(static_cast<const uint8_t*>(kernel::HostPtr(pixels)), pitch);
    invoke(0x34, 0x000d27be);
    LOGR("[SIMS2][PORTRAIT][TRACE] generated bitmap=%08x pitch=%u readable=%d", bitmap, pitch, int(readable));
    return readable;
}

bool ReadGeneratedPortraitIndices(const X86Context& ctx, uint8_t* base, uint32_t bitmap,
                                  std::array<uint8_t, 1024>& indices) {
    return ReadGeneratedPortrait(ctx, base, bitmap, 32, 32, [&](const uint8_t* pixels, uint32_t pitch) {
        SwizzlePortraitIndices(pixels, pitch, indices);
    });
}

std::vector<uint8_t> ReadGeneratedCasPortrait(const X86Context& ctx, uint8_t* base, uint32_t bitmap) {
    std::vector<uint8_t> image;
    constexpr uint32_t rows = 128, rowBytes = 128 * 4;
    ReadGeneratedPortrait(ctx, base, bitmap, rowBytes, rows, [&](const uint8_t* pixels, uint32_t pitch) {
        image.resize(rows * rowBytes);
        for (uint32_t y = 0; y < rows; ++y) {
            std::memcpy(image.data() + y * rowBytes, pixels + y * pitch, rowBytes);
        }
    });
    return image;
}
#endif

void ReleaseTemporaryPortrait(X86Context& state, uint8_t* base, uint32_t bitmap) {
    if (!bitmap) return;
    const uint32_t manager = kernel::R32(0x003b8124);
    const uint32_t methods = kernel::R32(manager);
    const auto invoke = [&](uint32_t method, uint32_t returnPc, bool withBitmap) {
        state.ecx = manager;
        if (withBitmap) PortraitGuestPush(state, bitmap);
        PortraitGuestPush(state, returnPc);
        x86_icall(state, base, kernel::R32(methods + method));
    };
    invoke(0x70, 0x00069dcb, true);
    if (state.eax & 0xff) invoke(0x1c, 0x00069dda, false);
    invoke(0x5c, 0x00069de6, true);
}

void ProbeSavedPortrait(const X86Context& original, uint8_t* base, uint32_t sim,
                        const uint8_t* indices = nullptr) {
    const uint32_t neighborhood = kernel::R32(0x003b7ee4);
    if (!neighborhood || kernel::R32(neighborhood) != 0x002f9e20) return;
    auto state = original;
    state.esi = neighborhood;
    state.edx = 0x55736572; // User
    PortraitGuestCall(__imp__sub_000A4F40, state, base, 0x000a45ac);

    const uint32_t previousChunk = kernel::R32(neighborhood + 0x17c);
    const uint32_t appearance = (original.esp - 0x130) & ~15u;
    uint32_t node = state.eax ? kernel::R32(state.eax) : 0;
    for (uint32_t slot = 1; node && slot <= 128; ++slot, node = kernel::R32(node)) {
        state = original;
        state.esp = appearance;
        state.edi = appearance;
        PortraitGuestCall(__imp__sub_00048850, state, base, 0x0017e2cf);
        state.eax = appearance;
        state.esi = slot;
        PortraitGuestCall(__imp__sub_001805D0, state, base, 0x0017e2d8);
        kernel::W32(neighborhood + 0x17c, previousChunk);
        if (!(state.eax & 0xff) || kernel::R32(appearance) != sim) continue;

        LOGR("[SIMS2][PORTRAIT][TRACE] saved appearance match sim=%08x slot=%u", sim, slot);
        static const bool render = std::getenv("SIMS2_TEST_RENDER_SAVED_APPEARANCE") != nullptr;
        if (!render && !indices) return;

        const uint32_t descriptor = (appearance - 0x240) & ~15u;
        const uint32_t models = (descriptor - 0x180) & ~15u;
        state = original;
        state.esp = models;
#if defined(_WIN32)
        const auto faultHandler = AddVectoredExceptionHandler(1, TraceSavedPortraitFault);
#endif
        PortraitGuestPush(state, models);
        PortraitGuestCall(__imp__sub_00069ED0, state, base, 0x0011d75a);
        LOGR("[SIMS2][PORTRAIT][TRACE] initialized saved portrait model container");
        PortraitGuestPush(state, 1);
        PortraitGuestPush(state, appearance + 4);
        PortraitGuestPush(state, models);
        PortraitGuestCall(__imp__sub_0006A060, state, base, 0x0011d776);
        LOGR("[SIMS2][PORTRAIT][TRACE] attached saved appearance to model container");
        // Stock detached model setup prepares the material variants before assembling body parts.
        const bool female = *static_cast<const uint8_t*>(kernel::HostPtr(appearance + 4 + 0x84)) != 0;
        PortraitGuestPush(state, female ? 0x00305b7c : 0x00305b88);
        PortraitGuestPush(state, models);
        PortraitGuestCall(__imp__sub_0006B3E0, state, base, 0x0011d781);
        state.esi = models;
        PortraitGuestCall(__imp__sub_0006A400, state, base, 0x0011d786);
        PortraitGuestPush(state, models);
        PortraitGuestCall(__imp__sub_0006C010, state, base, 0x0011d78c);
        state.eax = models;
        PortraitGuestCall(__imp__sub_0006BB40, state, base, 0x0011d793);
        state.eax = models;
        PortraitGuestCall(__imp__sub_0006A7F0, state, base, 0x0011d79a);
        state.eax = models;
        PortraitGuestCall(__imp__sub_0006AED0, state, base, 0x0011d7a1);
        state.esi = descriptor;
        PortraitGuestCall(__imp__sub_00068A50, state, base, 0x0018856b);
        state.esi = descriptor;
        state.edx = models;
        *static_cast<uint8_t*>(kernel::HostPtr(descriptor + 0x84)) =
            *static_cast<const uint8_t*>(kernel::HostPtr(appearance + 4 + 0x84));
        PortraitGuestCall(__imp__sub_00068D80, state, base, 0x001885a0);
        state.ecx = descriptor;
        LOGR("[SIMS2][PORTRAIT][TRACE] rendering saved appearance sim=%08x", sim);
        bool exerciseNewBitmap = false;
        uint32_t renderCaller = 0x001885e4;
#if defined(SIMS2_HEADLESS_RUNTIME) && defined(XBE_VULKAN_HOST)
        // Exercise the normal generator and indexed bitmap readback before
        // loading a lot. The next saved portraits still use their saved keys.
        static const bool exerciseGenerated = std::getenv("SIMS2_TEST_GENERATED_PORTRAIT_IN_MENU") != nullptr;
        static bool generatedExercised = false;
        exerciseNewBitmap = indices && exerciseGenerated && !generatedExercised;
        static const bool exerciseCas = std::getenv("SIMS2_TEST_CAS_PORTRAIT_IN_MENU") != nullptr;
        if (indices && exerciseCas && !generatedExercised) {
            kernel::W32(descriptor, 128);
            kernel::W32(descriptor + 4, 128);
            kernel::W32(descriptor + 8, 32);
            renderCaller = 0x0011d0d0;
            exerciseNewBitmap = true;
        }
        if (exerciseNewBitmap) generatedExercised = true;
#endif
        if (exerciseNewBitmap) {
            PortraitGuestCall(sub_00068EE0, state, base, renderCaller);
        } else {
#if defined(XBE_VULKAN_HOST) || defined(SIMS2_NATIVE_THUMBNAILS)
            std::unique_lock renderLock(generatedPortraitLock, std::defer_lock);
            if (indices) {
                renderLock.lock();
                gpu::SetSavedPortraitProbe(sim, indices);
            }
#endif
            struct SavedRenderScope {
                const uint8_t* indices;
                explicit SavedRenderScope(const uint8_t* bytes) : indices(bytes) {
                    tracingSavedRender = true;
                }
                ~SavedRenderScope() {
                    tracingSavedRender = false;
#if defined(XBE_VULKAN_HOST) || defined(SIMS2_NATIVE_THUMBNAILS)
                    if (indices) gpu::SetSavedPortraitProbe(0, nullptr);
#endif
                }
            } saved(indices);
            PortraitGuestCall(sub_00068EE0, state, base, 0x001885e4);
        }
#if defined(_WIN32)
        if (faultHandler) RemoveVectoredExceptionHandler(faultHandler);
#endif
        LOGR("[SIMS2][PORTRAIT][TRACE] saved appearance renderer resource=%08x", state.eax);
        if (indices) ReleaseTemporaryPortrait(state, base, state.eax);
        PortraitGuestPush(state, descriptor);
        PortraitGuestCall(__imp__sub_00068B40, state, base, 0x0011d0eb);
        PortraitGuestPush(state, models);
        PortraitGuestCall(__imp__sub_00069FF0, state, base, 0x00038902);
        return;
    }
    LOGR("[SIMS2][PORTRAIT][TRACE] no User chunk for sim=%08x", sim);
}
} // namespace

X86_EXTERN_FUNC(__imp__sub_001E3B10);
X86_FUNC(sub_001E3B10) {
    if (tracingSavedRender) {
        const auto pointer = kernel::R32(ctx.esp + 4);
        LOGR("[SIMS2][PORTRAIT][TRACE] updating pose caller=%08x pointer=%08x committed=%d",
            kernel::R32(ctx.esp), pointer, int(mem::IsCommitted(pointer)));
    }
    __imp__sub_001E3B10(ctx, base);
}

X86_EXTERN_FUNC(__imp__sub_0006D4E0);
X86_FUNC(sub_0006D4E0) {
    static const bool trace = std::getenv("SIMS2_UI_LAYOUT_TRACE") != nullptr;
    if (trace && kernel::R32(ctx.esp) == 0x0006d491) {
        for (uint32_t slot = 0; slot < 26; ++slot) {
            const uint32_t resource = kernel::R32(ctx.edi + slot * 4);
            if (!resource) continue;
            const uint32_t poseField = resource + 0x118;
            const uint32_t pose = mem::IsCommitted(poseField) ? kernel::R32(poseField) : 0;
            LOG_FIRST(192, "[SIMS2][PORTRAIT][TRACE] pose source saved=%d slot=%u model=%08x pose=%08x mapped=%d",
                int(tracingSavedRender), slot, resource, pose, int(pose && mem::IsCommitted(pose)));
        }
    }
    __imp__sub_0006D4E0(ctx, base);
}

void ProbePendingFamilyPortrait(const X86Context& ctx, uint8_t* base, bool requested) {
    if (AutoSavedPortraits()) {
        SavedPortraitRequest request;
        {
            std::lock_guard lock(savedPortraitLock);
            if (savedPortraitInFlight || savedPortraitCursor >= savedPortraitQueue.size() ||
                PortraitClock::now() < nextSavedPortrait) return;
            request = savedPortraitQueue[savedPortraitCursor++];
            savedPortraitInFlight = true;
        }
        struct FinishSavedPortrait {
            ~FinishSavedPortrait() {
                std::lock_guard lock(savedPortraitLock);
                savedPortraitInFlight = false;
                nextSavedPortrait = std::max(nextSavedPortrait,
                    PortraitClock::now() + std::chrono::milliseconds(350));
            }
        } finish;
        if (kernel::R32(0x003b7ee4) == request.neighborhood) {
            ProbeSavedPortrait(ctx, base, request.sim, request.indices.data());
        }
        return;
    }
    if (!requested) return;
    if (!pendingSavedPortrait.load(std::memory_order_acquire)) return;
    // Wait for a fresh host-side input edge after the family panel is visible.
    static const bool onDemand = std::getenv("SIMS2_TEST_RENDER_SAVED_APPEARANCE") != nullptr;
    if (!onDemand || kernel::R32(0x003b7ee4) == 0) return;
    const uint32_t sim = pendingSavedPortrait.exchange(0);
    if (!sim) return;
    LOGR("[SIMS2][PORTRAIT][TRACE] processing saved appearance on main thread sim=%08x", sim);
    ProbeSavedPortrait(ctx, base, sim);
}

X86_FUNC(sub_000D27E0) {
    static const bool trace = std::getenv("SIMS2_UI_LAYOUT_TRACE") != nullptr;
    const bool savedThumbnail = (trace || AutoSavedPortraits()) && kernel::R32(ctx.esp) == 0x000d22d5;
    const uint32_t record = ctx.ebx;
    const uint32_t chunk = savedThumbnail && AutoSavedPortraits() ? kernel::R32(ctx.esp + 8) : 0;
    __imp__sub_000D27E0(ctx, base);
    if (savedThumbnail) {
        const uint32_t info = kernel::R32(record + 0x34);
        const uint32_t sim = info ? kernel::R32(info + 0x1c) : 0;
        const uint32_t registry = kernel::R32(0x00348b10);
        const bool knownRegistry = registry && kernel::R32(registry) == 0x002fa5a8;
        const uint32_t begin = knownRegistry ? kernel::R32(registry + 0x2048) : 0;
        const uint32_t end = knownRegistry ? kernel::R32(registry + 0x204c) : 0;
        const bool validRange = knownRegistry && end >= begin && (end - begin) % 4 == 0;
        LOG_FIRST(32, "[SIMS2][PORTRAIT][TRACE] saved thumbnail record=%08x resource=%08x sim=%08x actor_count=%d",
            record, kernel::R32(record + 0x90), sim, validRange ? int((end - begin) / 4) : -1);

        if (chunk && sim && validRange && begin == end) {
            QueueSavedPortrait(kernel::R32(0x003b7ee4), sim, chunk);
        }
        static const bool probe = std::getenv("SIMS2_TEST_SAVED_APPEARANCE") != nullptr;
        static const bool render = std::getenv("SIMS2_TEST_RENDER_SAVED_APPEARANCE") != nullptr;
        static std::atomic<bool> attempted{false};
        if (probe && sim && validRange && begin == end && !attempted.exchange(true)) {
            if (render) pendingSavedPortrait.store(sim);
            else ProbeSavedPortrait(ctx, base, sim);
        }
    }
}

X86_EXTERN_FUNC(__imp__sub_00067D60);
X86_EXTERN_FUNC(__imp__sub_0003B6E0);
X86_FUNC(sub_00067D60) {
    // Exercise the stock thumbnail generator once the test lot has a live Sim.
    // This leaves the original HUD call and its guest register results intact.
    static const bool regenerate = std::getenv("SIMS2_TEST_REGENERATE_THUMBNAIL") != nullptr;
    static std::atomic<bool> attempted{false};
    uint32_t actor = 0;
    uint32_t caller = 0;
    if (regenerate && !attempted.load(std::memory_order_relaxed)) {
        caller = kernel::R32(ctx.esp);
        if (caller == 0x00147dba || caller == 0x00147dc6) {
            actor = kernel::R32(ctx.ecx + 0x358);
        }
    }
    __imp__sub_00067D60(ctx, base);
    if (actor && !attempted.exchange(true)) {
        auto thumbnailContext = ctx;
        thumbnailContext.ecx = actor;
        thumbnailContext.esp -= 4;
        kernel::W32(thumbnailContext.esp, caller);
        LOGR("[SIMS2][PORTRAIT][TRACE] regenerating live Sim actor=%08x", actor);
        __imp__sub_0003B6E0(thumbnailContext, base);
        LOGR("[SIMS2][PORTRAIT][TRACE] live Sim regeneration returned");
    }
}
#endif
