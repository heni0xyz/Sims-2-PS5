// SPDX-License-Identifier: GPL-3.0-only
// OpenAI Codex for szampan. Template pad/display/exit contracts retained.
#include "installer.h"
#include "services.h"
#include "video.h"
#include "platform.h"
#include "game/install.h"
#include "game/install_screen.h"
#include "game/install_display.h"
#include "game/disc_boot.h"
#include <atomic>
#include <chrono>
#include <thread>
#include <algorithm>
#include <cctype>
namespace sims2::ps5 {
namespace {
struct Job {
    std::thread worker;
    std::atomic<bool> cancel{false},done{false};
    std::atomic<unsigned> percent{0};
    bool success=false;
    std::string error;
    install::Source source;
    ~Job(){cancel=true;if(worker.joinable())worker.join();}
};
}
bool InstallGame(app::GameFiles& game,uint32_t width,uint32_t height,uint32_t test_seconds){
    const auto root=app::ExeDir();
    struct AspectGuard {std::string previous=app::Get().aspect_ratio;~AspectGuard(){app::Get().aspect_ratio=previous;}} aspect;
    app::Get().aspect_ratio="16:9";
    InitVideo(width,height,true);
    install::Display display;
    install::Choice choice;install::View view;
    Job job;bool running=false,quitting=false;
    std::filesystem::path iso;std::string discovery;
    double rescan=0;const double start=now_seconds();
    std::string last_image;
    Log("INSTALL","INFO","No verified extracted data; waiting for player","INSTALL_WAIT");
    for(;;){
        pad controller{};pad_poll(&controller);
        if((controller.pressed&PAD_OPTIONS) || (test_seconds && now_seconds()-start>=test_seconds)){quitting=true;job.cancel=true;}
        if(running && job.done.load(std::memory_order_acquire)){
            job.worker.join();running=false;
            if(quitting)return false;
            if(job.success && install::Ready(root,game,job.error) && install::PrepareCache(root,job.error)){
                choice.Set(install::Screen::DeleteQuestion);view.error.clear();
                Log("INSTALL","INFO","Verified extracted data committed; awaiting source ISO choice","INSTALL_COMPLETE");
            }else{
                choice.Set(install::Screen::Error);view.error=job.error;
                Log("INSTALL","ERROR",job.error.c_str(),"INSTALL_FAILED");
            }
        }
        if(quitting && !running)return false;
        if(!running && (choice.screen==install::Screen::Missing || choice.screen==install::Screen::Error) && now_seconds()>=rescan){
            rescan=now_seconds()+1;
            view.can_extract=disc::RootImage(root,iso,discovery) && !iso.empty();
            std::error_code ec;
            bool assets_exist=false;
            for(const auto& entry:std::filesystem::directory_iterator(root,ec)){
                auto name=entry.path().filename().string();for(auto& c:name)c=char(std::tolower(static_cast<unsigned char>(c)));
                assets_exist|=name=="assets";
            }
            if(assets_exist){
                view.can_extract=false;discovery="Existing assets are incomplete. Move that folder aside before extracting.";
            }
            if(!discovery.empty()){choice.Set(install::Screen::Error);view.error=discovery;}
        }
        if(!quitting){
            const auto action=choice.Poll(controller.pressed&PAD_CROSS,controller.pressed&PAD_CIRCLE,
                                         controller.held&(PAD_CROSS|PAD_CIRCLE),view.can_extract);
            if(action==install::Action::Extract){
                if(!install::RememberSource(root,iso,job.source,view.error)){choice.Set(install::Screen::Error);Log("INSTALL","ERROR",view.error.c_str(),"INSTALL_SOURCE_FAILED");continue;}
                job.cancel=false;job.done=false;job.percent=0;job.error.clear();job.success=false;
                choice.Set(install::Screen::Copying);view.screen=choice.screen;view.percent=0;display.Draw(view);
                running=true;Log("INSTALL","INFO","Beginning checked ISO extraction","INSTALL_BEGIN");
                job.worker=std::thread([&]{
                    try{
                        unsigned reported=101;
                        job.success=install::FromIso(job.source.path,root,[&](auto progress){
                            const auto percent=progress.stage==install::Stage::Complete?100u:unsigned(std::min<uint64_t>(99,progress.done*100/progress.total));
                            job.percent.store(percent,std::memory_order_relaxed);
                            if(percent!=reported){reported=percent;const auto message="Extraction progress "+std::to_string(percent)+"%";Log("INSTALL","INFO",message.c_str());}
                            return !job.cancel.load(std::memory_order_relaxed);
                        },job.error);
                    }catch(const std::exception& exception){job.success=false;job.error=exception.what();}
                    job.done.store(true,std::memory_order_release);
                });
            }else if(action==install::Action::KeepIso){
                Log("INSTALL","INFO","Player kept source ISO; starting extracted game");return true;
            }else if(action==install::Action::DeleteIso){
                if(install::DeleteSource(job.source,view.error)){
                    Log("INSTALL","INFO","Player confirmed source ISO deletion; installed data retained");return true;
                }
                choice.Set(install::Screen::DeleteError);Log("INSTALL","WARN",view.error.c_str());
            }
        }
        view.screen=choice.screen;view.percent=job.percent.load(std::memory_order_relaxed);
        const auto identity=std::to_string(int(view.screen))+":"+std::to_string(view.percent)+":"+std::to_string(view.can_extract)+":"+view.error;
        if(identity!=last_image){display.Draw(view);last_image=identity;}
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
}
}
