// Copyright (C) 2026 OpenAI Codex, for szampan
// SPDX-License-Identifier: GPL-3.0-only
// console_probe.h is included at file scope by the foundation's prelude.h.
#include "vulkanexamplebase.h"
#include "ps5_samples.h"

// A timed-out worker is not cancelled inside LLVM. State has process lifetime;
// the foundation asks the shell to close the title after draining Vulkan.
static Sims2LinkerProbe probe;
#if defined(SIMS2_PROBE_NATIVE_TITLE)
static constexpr bool nativeTitleMode = true;
#else
static constexpr bool nativeTitleMode = false;
#endif
static constexpr unsigned testLimitSeconds = nativeTitleMode ? 300 : (SIMS2_PROBE_GAME_UNIT_COUNT > 1 ? 1800 : 120);
#if defined(SIMS2_PROBE_GAME_UNIT)
static constexpr bool gameUnitMode = true;
#else
static constexpr bool gameUnitMode = false;
#endif
#if defined(SIMS2_PROBE_GENERATE)
static constexpr bool generationMode = true;
#else
static constexpr bool generationMode = false;
#endif
#if defined(SIMS2_PROBE_EXECUTE)
static constexpr bool executionMode = true;
#else
static constexpr bool executionMode = false;
#endif
#if defined(SIMS2_PROBE_COMPILER)
static constexpr const char *probeTag = "COMPILERPROBE";
static constexpr bool compilerMode = true;
#else
static constexpr const char *probeTag = "LINKPROBE";
static constexpr bool compilerMode = false;
#endif
class VulkanExample : public VulkanExampleBase {
    double lastHeartbeat = 0;
    bool joined = false;
    bool timedOut = false;
    bool shutdownReported = false;
public:
    VulkanExample() {
        title = compilerMode ? "Sims 2 Compiler Diagnostics" : "Sims 2 Linker Diagnostics";
        apiVersion = VK_API_VERSION_1_3;
        useDynamicRendering = true;
        requiresStencil = true;
        deviceCreatepNextChain = &baseDynamicRenderingFeatures;
        ps5.ownOverlay = true;
        settings.overlay = true;
        defaultClearColor = {{.025f, .033f, .055f, 1.f}};
    }
    void prepare() override {
        VulkanExampleBase::prepare();
        prepared = true;
        say("[SIMS2][%s][INFO] build=%s event=vulkan_ready width=%u height=%u",
            probeTag, SIMS2_PROBE_BUILD_ID, width, height);
    }
    void render() override {
        if (!prepared) return;
        int phase = probe.phase.load();
        const auto pressed = ps5_pad().pressed;
        if (!phase && ((pressed & PAD_CROSS) || ps5.frameBudget)) probe.start();
        phase = probe.phase.load();
        if (phase == 3 && !joined) {
            probe.finish(); joined = true;
            if (ps5.frameBudget && probe.verdict.load() != 0)
                vks::tools::exitFatal("Console preparation probe failed; inspect klog", 1);
        }
        double elapsed = now_seconds() - probe.began.load();
        if (phase && phase != 3 && now_seconds() - lastHeartbeat >= 2) {
            lastHeartbeat = now_seconds();
            say("[SIMS2][%s][INFO] build=%s event=heartbeat phase=%d elapsed=%.3f units_done=%u",
                probeTag, SIMS2_PROBE_BUILD_ID, phase, elapsed, probe.units_done.load());
        }
        const double operationBegan = probe.operation_began.load();
        if (phase && phase != 3 && (elapsed > testLimitSeconds ||
                (phase == 8 && operationBegan > 0 && now_seconds()-operationBegan > 120))) {
            timedOut = true;
            say("[SIMS2][%s][ERROR] build=%s event=watchdog_timeout elapsed=%.3f status=FAIL",
                probeTag, SIMS2_PROBE_BUILD_ID, elapsed);
            if (ps5.frameBudget)
                vks::tools::exitFatal("Console linker probe watchdog expired", 1);
            quit = true;
            return;
        }
        if (!ps5.frameBudget && (pressed & (PAD_CIRCLE | PAD_OPTIONS)) && (!phase || phase == 3)) {
            say("[SIMS2][%s][INFO] build=%s event=shutdown verdict=%d timed_out=%d",
                probeTag, SIMS2_PROBE_BUILD_ID, probe.verdict.load(), timedOut);
            quit = true;
            return;
        }
        // A test harness must not turn a frame-budget expiry into success while
        // the linker is still busy. Re-arm the budget until the worker ends;
        // the independent wall-clock watchdog bounds a stalled run.
        if (ps5.frameBudget && phase != 3 && ps5.framesDrawn + 2 >= ps5.frameBudget)
            ps5.frameBudget += 120;
        if (ps5.frameBudget && phase == 3 && !shutdownReported &&
                ps5.framesDrawn + 1 >= ps5.frameBudget) {
            say("[SIMS2][%s][INFO] build=%s event=shutdown verdict=%d timed_out=0 mode=frame-budget",
                probeTag, SIMS2_PROBE_BUILD_ID, probe.verdict.load());
            shutdownReported = true;
        }
        prepareFrame();
        VkCommandBuffer cmd = drawCmdBuffers[currentBuffer];
        auto begin = vks::initializers::commandBufferBeginInfo();
        VK_CHECK_RESULT(vkBeginCommandBuffer(cmd, &begin));
        beginDynamicRendering(cmd); drawUI(cmd); endDynamicRendering(cmd);
        VK_CHECK_RESULT(vkEndCommandBuffer(cmd));
        submitFrame();
    }
    void OnUpdateUIOverlay(vks::UIOverlay *) override {
        const ImVec2 display = ImGui::GetIO().DisplaySize;
        ImGui::SetNextWindowPos(ImVec2(display.x * .5f, display.y * .5f), ImGuiCond_Always, ImVec2(.5f, .5f));
        ImGui::SetNextWindowSize(ImVec2(display.x * .82f, 0));
        const auto flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize;
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
        ImGui::Begin("Linker diagnostics", nullptr, flags);
        auto centered = [](const char *text) {
            ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize(text).x) * .5f);
            ImGui::TextUnformatted(text);
        };
        centered(compilerMode ? "Sims 2 - Console Compiler Test" : "Sims 2 - Console Linker Test");
        centered(nativeTitleMode ? "Creates a private executable from verified console objects. No game is launched." :
            gameUnitMode ? "Compiles private game source. No game is launched." :
            generationMode ? "Includes verified owner-XBE source generation. No game is launched." :
            "Synthetic test only. Your game and saves are not changed.");
        ImGui::Spacing(); ImGui::Spacing();
        const int phase = probe.phase.load();
        if (!phase) {
            centered("Start klog capture before continuing.");
            centered(executionMode ? "Press Cross to compile, link and execute the test fixture." :
                compilerMode ? "Press Cross to compile C++ and link on this PS5." : "Press Cross to test linking on this PS5.");
        } else if (phase != 3) {
            centered(phase == 1 ? "Checking files and writable storage..." :
                phase == 4 ? "Compiling C++ on PS5..." : phase == 5 ? "Testing file operations..." :
                phase == 6 ? "Executing the locally compiled test code..." :
                phase == 7 ? "Generating and verifying private game source..." :
                phase == 8 ? "Compiling real game source files..." :
                phase == 9 ? "Linking and packaging a private native title..." : "Linking on PS5...");
            if (phase == 8) ImGui::Text("Compiled: %u / %u", probe.units_done.load(), unsigned(SIMS2_PROBE_GAME_UNIT_COUNT));
            centered(nativeTitleMode ? "Please wait. Watchdog: 5 minutes." :
                testLimitSeconds > 120 ? "Watchdog: 120 seconds per unit, 30 minutes total." : "Please wait. Timeout: 120 seconds.");
        } else {
            centered(probe.verdict.load() == 0 ? (nativeTitleMode ? "PASS - Private native executable created; runtime NOT TESTED" :
                gameUnitMode ? "PASS - Private game source generation and requested compilation" :
                generationMode ? "PASS - Compiler, execution and private source generation" :
                executionMode ? "PASS - Compilation, linking, execution and I/O" :
                compilerMode ? "PASS - Compilation, linking and I/O completed" : "PASS - Console linking completed") : "FAIL - Please send the complete klog");
            centered(compilerMode ? "Results: /data/sims2-compiler-probe or compiler-runs" : "Results and linked test ELF files: linker-runs or /data/sims2-linker-probe");
            centered(nativeTitleMode ? "The private game executable has not been launched." :
                gameUnitMode ? "The complete game has not been linked or executed." :
                "This test does not compile or run The Sims 2.");
        }
        ImGui::Spacing(); ImGui::Spacing();
        if (!phase || phase == 3) centered("Press Circle to exit");
        centered("Build: " SIMS2_PROBE_BUILD_ID);
        ImGui::End(); ImGui::PopStyleColor();
    }
};
VULKAN_EXAMPLE_MAIN()
