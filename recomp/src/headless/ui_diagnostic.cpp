// PC-only calibration and one-shot capture; never changes controller input.
#if defined(SIMS2_HEADLESS_RUNTIME)
#include "game/ui_aspect.h"
#include "game/cas_layout.h"
#include "game/render_resolution.h"
#include "game/font_upscale.h"
#include "gpu/backend.h"
#include "core/input.h"
#include "core/log.h"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
void PollUiDiagnostic(int port,input::Pad&) {
    if(port || !std::getenv("SIMS2_UI_LAYOUT_TRACE"))return;
    using clock=std::chrono::steady_clock;
    static auto next=clock::now();const auto now=clock::now();
    if(now<next)return;next=now+std::chrono::milliseconds(500);
    if(FILE* f=std::fopen("ui-diagnostic.cmd","r")) {
        char action[48]{};int value=-1;
        const bool read=std::fscanf(f,"%47s %d",action,&value)==2;
        std::fclose(f);
        if(std::remove("ui-diagnostic.cmd"))return;
        if(read && !std::strcmp(action,"cas_edges") && value>=0 && value<=1) {
            sims2::ui::cas_edge_anchors.store(value!=0,std::memory_order_relaxed);LOGR("[SIMS2][UI][CALIBRATION] cas_edges=%d",value);
        } else if(read && !std::strcmp(action,"portrait_mode") && value>=0 && value<=1) {
            sims2::ui::native_portrait=value!=0;LOGR("[SIMS2][UI][CALIBRATION] native_portrait=%d",value);
        } else if(read && !std::strcmp(action,"native_resolution") && value>=0 && value<=1) {
            sims2::render::native_resolution=value!=0;
            LOGR("[SIMS2][RENDER][CALIBRATION] native_resolution=%d",value);
        } else if(read && !std::strcmp(action,"font_upscale") && value>=0 && value<=1) {
            sims2::fonts::TestEnabled(value!=0);LOGR("[SIMS2][FONT][CALIBRATION] font_upscale=%d",value);
        } else if(read && !std::strcmp(action,"capture_frame") && value==1) {
            gpu::g_capture=true;LOGR("[SIMS2][UI][CALIBRATION] one-shot GPU frame capture requested");
        } else LOGE("[SIMS2][UI][CALIBRATION] invalid PC diagnostic command");
    }
}
#endif
