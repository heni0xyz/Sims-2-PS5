#include "app/app.h"
#include "core/input.h"
#include "core/log.h"
#include "image_symbols.h"
#include "kernel/kernel.h"
#include "platform/title.h"
#include "services.h"
#include "video.h"
#include "platform.h"
#include "game/ui_aspect.h"
#include "game/disc_boot.h"
#include "game/install.h"
#include "installer.h"
#include <chrono>
#include <cstdio>
#include <exception>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <xmmintrin.h>

#ifndef SIMS2_BUILD_ID
#define SIMS2_BUILD_ID "unidentified"
#endif
int main() {
    platform_init("Sims2Independent");
    std::string run="manual-"+std::to_string(static_cast<unsigned long long>(now_seconds()*1000000));
    uint32_t seconds=0;
    // Optional one-shot bounded test, absent in normal packages. Strict token
    // parsing prevents a stale or malformed request from becoming an unbounded run.
    bool invalid_test=false;
    if (std::ifstream file("/app0/sims2-test-run.txt");file) {
        std::string key,value;
        while (file>>key) {
            if (!(file>>value)) {invalid_test=true;break;}
            if (key=="run" && value.size()<=96 && value.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_")==std::string::npos) run=value;
            else if (key=="seconds") {
                try {size_t used=0;const auto n=std::stoul(value,&used);if (used!=value.size()||!n||n>600) invalid_test=true;else seconds=static_cast<uint32_t>(n);}
                catch (...) {invalid_test=true;}
            } else invalid_test=true;
        }
        invalid_test |= !seconds;
        file.close();
        if (std::remove("/app0/sims2-test-run.txt")) invalid_test=true;
    }
    sims2::ps5::InitLogging({SIMS2_BUILD_ID,run.c_str(),true});
    std::set_terminate([] {title_platform::Fatal("Unhandled native exception or terminate request");});
    sims2::ps5::Log("BOOT","INFO","Starting independently generated Sims 2 runtime","BOOT");
    if (invalid_test) title_platform::Fatal("Invalid or unconsumed bounded test request");
    try {
        _mm_setcsr(0x1F80); // IEEE subnormals, masked exceptions, nearest rounding.
        if (!mem::Init()) throw std::runtime_error("Guest address space initialization failed");
        sims2::ps5::Log("MEM","INFO","4 GiB guest arena and physical aliases initialized","MEM");
        app::Get().width=1920;app::Get().height=1080;app::Get().fps_cap=60;app::Get().aspect_ratio="16:9";
        sims2::ui::enabled=true;
        app::Load();app::LogSystem();
#if defined(SIMS2_PERFORMANCE_LOG)
        LOGI("perf: build-config schema=1 deep=%u guest_samples=%u fps_cap=%u vsync=%u (diagnostic overhead; compare with normal profile before accepting performance)",
#if defined(XBE_DEEP_PROFILE) && XBE_DEEP_PROFILE
             1u,
#else
             0u,
#endif
#if defined(XBE_GUEST_PROFILE) && XBE_GUEST_PROFILE
             1u,
#else
             0u,
#endif
             app::Get().fps_cap,unsigned(app::Get().vsync));
#endif
        input::Init();
        app::GameFiles game;
        std::string data_error;
        if(!sims2::install::Ready(app::ExeDir(),game,data_error)){
            const auto mode=app::Output();
            if(!sims2::ps5::InstallGame(game,mode.width,mode.height,seconds))return 0;
        }
        if(!sims2::install::PrepareCache(app::ExeDir(),data_error))throw std::runtime_error(data_error);
        if (app::kGame.init) app::kGame.init();
        const app::Mode mode=app::Output();
        sims2::ps5::InitVideo(mode.width,mode.height);
        std::error_code ec;
        const std::filesystem::path hdd=app::ExeDir()/"hdd";
        std::filesystem::create_directories(hdd,ec);
        if (ec) throw std::runtime_error("Cannot create writable Xbox HDD mount: "+ec.message());
        if (!kernel::Boot(std::move(game.xbe),IMAGE_XBE,game.dir.string(),hdd.string()))
            throw std::runtime_error("Xbox runtime boot failed");
        const double start=now_seconds();
        for (;;) {
            sims2::ps5::PublishPads();
            if (seconds && now_seconds()-start>=seconds) {
                sims2::ps5::Log("TEST","INFO","Bounded test interval ended; gameplay success requires captured evidence");
                kernel::Shutdown(0);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(8));
        }
    } catch (const std::exception& error) {title_platform::Fatal(error.what());}
    catch (...) {title_platform::Fatal("Unhandled native exception");}
}
