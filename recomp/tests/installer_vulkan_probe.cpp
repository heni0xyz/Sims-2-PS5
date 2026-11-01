// SPDX-License-Identifier: GPL-3.0-only
// Real offscreen Vulkan first-run screens, followed by a second device init.
#include "game/install_display.h"
#include "app/app.h"
#include "core/log.h"
#include "kernel/kernel.h"
#include <fstream>
#include <cstdio>
#include <stdexcept>
void InitializeHostVulkan();
int main(){try{
    logx::Init((app::ExeDir()/"logs").string(),"installer-probe");
    app::Get().width=3840;app::Get().height=2160;app::Get().aspect_ratio="16:9";
    if(!mem::Init())throw std::runtime_error("memory init failed");
    InitializeHostVulkan();
    const auto initial_memory=mem::Query();
    {
        sims2::install::Display display;
        for(auto screen:{sims2::install::Screen::Missing,sims2::install::Screen::Copying,sims2::install::Screen::Error,sims2::install::Screen::DeleteQuestion,sims2::install::Screen::DeleteError}){
            std::ofstream(app::ExeDir()/"snap").close();
            sims2::install::View view{screen,37,true,"Not enough free storage for extracted game data."};
            display.Draw(view);display.Draw(view);
        }
    }
    if(mem::Query().contiguous_used!=initial_memory.contiguous_used || mem::Query().system_used!=initial_memory.system_used)throw std::runtime_error("Installer guest framebuffer was not released");
    // The real title initializes its game hooks only after the installer
    // backend is gone. Their registration must still be available here.
    if(app::kGame.init)app::kGame.init();
    {sims2::install::Display second;second.Draw({sims2::install::Screen::Missing,0,true,{}});}
    std::puts("PASS_INSTALLER_VULKAN: five screens presented; teardown and second device initialization complete; no game assets used");
    logx::Flush();return 0;
}catch(const std::exception& error){std::fprintf(stderr,"INSTALLER_VULKAN_FAIL: %s\n",error.what());return 1;}}
