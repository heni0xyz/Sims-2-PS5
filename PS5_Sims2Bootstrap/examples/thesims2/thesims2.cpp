// Copyright (C) 2026 OpenAI Codex, for szampan
// SPDX-License-Identifier: GPL-3.0-only
// Public instruction title; contains no translated Xbox game functions.
#include "vulkanexamplebase.h"
#include "ps5_samples.h"

class VulkanExample : public VulkanExampleBase {
public:
    VulkanExample() {
        title = "The Sims 2";
        apiVersion = VK_API_VERSION_1_3;
        useDynamicRendering = true;
        requiresStencil = true;
        deviceCreatepNextChain = &baseDynamicRenderingFeatures;
        ps5.ownOverlay = true;
        settings.overlay = true;
        defaultClearColor = {{0.025f, 0.033f, 0.055f, 1.0f}};
    }
    void prepare() override {
        VulkanExampleBase::prepare();
        prepared = true;
        say("[SIMS2][BOOTSTRAP][INFO] Public preparation screen; no game code linked");
    }
    void render() override {
        if (!prepared) return;
        if (!ps5.frameBudget && (ps5_pad().pressed & (PAD_CIRCLE | PAD_OPTIONS))) {
            quit = true;
            return; // Template drains the device and exits through its CRT.
        }
        prepareFrame();
        VkCommandBuffer cmd = drawCmdBuffers[currentBuffer];
        VkCommandBufferBeginInfo begin = vks::initializers::commandBufferBeginInfo();
        VK_CHECK_RESULT(vkBeginCommandBuffer(cmd, &begin));
        beginDynamicRendering(cmd);
        drawUI(cmd);
        endDynamicRendering(cmd);
        VK_CHECK_RESULT(vkEndCommandBuffer(cmd));
        submitFrame();
    }
    void OnUpdateUIOverlay(vks::UIOverlay *) override {
        const ImVec2 display = ImGui::GetIO().DisplaySize;
        ImGui::SetNextWindowPos(ImVec2(display.x * .5f, display.y * .5f), ImGuiCond_Always, ImVec2(.5f, .5f));
        ImGui::SetNextWindowSize(ImVec2(display.x * .76f, 0));
        const auto flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize;
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
        ImGui::Begin("Preparation", nullptr, flags);
        auto centered = [](const char *text) {
            ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize(text).x) * .5f);
            ImGui::TextUnformatted(text);
        };
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(.55f, .92f, .74f, 1));
        centered("The Sims 2");
        ImGui::PopStyleColor();
        ImGui::Spacing(); ImGui::Spacing();
        centered("Game preparation required");
        ImGui::Spacing(); ImGui::Spacing();
        centered("Prepare your Xbox USA copy on a Windows PC.");
        ImGui::Spacing();
        centered("1. Download The Sims 2 Builder from:");
        centered("https://github.com/heni0xyz/Sims-2-PS5");
        centered("2. Select your ISO and choose Build.");
        centered("3. Close this title and copy the generated PPSA99145 folder to PS5.");
        ImGui::Spacing();
        centered("Keep your existing hdd folder and saves when updating.");
        centered("You only need to prepare the game once.");
        ImGui::Spacing(); ImGui::Spacing();
        centered("Press Circle to exit");
        ImGui::End();
        ImGui::PopStyleColor();
    }
};
VULKAN_EXAMPLE_MAIN()
