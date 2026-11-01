#include "services.h"
#include "contracts.h"
#include "audio/audio.h"
#include "core/log.h"
#include "app/app.h"
#include "platform.h" // the template's PS5 services, never SDL
#include <array>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <functional>
#include <mutex>
#include <pthread.h>
#include <stdexcept>
#include <utility>

namespace {
sims2::ps5::LogSession session{"unknown", "unknown", true};
std::mutex pad_lock;
std::array<input::Pad,4> pads{};
std::function<void(float*,uint32_t)> audio_render;
bool audio_started=false;
constexpr int kAudioFrames=256; // platform.h's documented callback grain
std::array<float,kAudioFrames*2> audio_scratch{};
void FillAudio(int16_t* output,int frames,void*) {
    // The template currently always asks for 256. Chunking also handles a
    // larger request without allocations on the device's real-time thread.
    for (int offset=0;offset<frames;) {
        int n=frames-offset<kAudioFrames?frames-offset:kAudioFrames;
        audio_render(audio_scratch.data(),static_cast<uint32_t>(n));
        for (int i=0;i<n*2;i++) output[offset*2+i]=sims2::ps5::AudioSample(audio_scratch[i]);
        offset+=n;
    }
}
}
namespace sims2::ps5 {
void InitLogging(LogSession value) { session=value; }
void Log(const char* subsystem,const char* severity,const char* message,const char* milestone,
         uint32_t guest,uintptr_t native) {
    if (!session.diagnostic && std::string(severity)=="DEBUG") return;
    say("[SIMS2][%s][%s] t=%.6f build=%s run=%s tid=%llu milestone=%s guest=%08X native=%llx %s",
        subsystem,severity,now_seconds(),session.build,session.run,
        static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(pthread_self())),milestone,
        guest,static_cast<unsigned long long>(native),message);
}
void PublishPads() {
    pad first{}; pad_poll(&first);
    std::array<input::Pad,4> next{};
    for (int i=0;i<4;i++) {
        pad state{};
        bool connected=pad_player(i,&state);
        next[i]=MapPad({connected,state.held,state.left_x,state.left_y,state.right_x,state.right_y,state.l2,state.r2});
    }
    std::lock_guard lock(pad_lock); pads=next;
}
int16_t AudioSample(float value) {
    if (!std::isfinite(value)) return 0;
    if (value<=-1) return -32768;
    if (value>=1) return 32767;
    return static_cast<int16_t>(std::lround(value*(value<0?32768.f:32767.f)));
}
void StopAudio() {
    if (audio_started) audio_stop();
    audio_started=false; audio_render={};
}
}
namespace input {
void Init() {
    if (!pad_open()) sims2::ps5::Log("INPUT","WARN","No connected controller");
}
Pad Poll(int port) {
    if (port<0||port>=4) return {};
    Pad result;
    { std::lock_guard lock(pad_lock); result=pads[port]; }
    if (app::kGame.pad) app::kGame.pad(port,result);
    return result;
}
void Rumble(int port,uint16_t left,uint16_t right) {
    if (app::kGame.rumble) app::kGame.rumble(port,left,right);
    if (port>=0&&port<4) pad_player_vibrate(port,left/65535.f,right/65535.f);
}
}
namespace audio {
void StartOutput(std::function<void(float*,uint32_t)> render) {
    // Start/Stop run on the owner thread with guest audio callbacks quiescent.
    sims2::ps5::StopAudio();
    if (!render) throw std::invalid_argument("PS5 audio requires a mixer callback");
    audio_render=std::move(render);
    if (!audio_start(FillAudio,nullptr)) {
        audio_render={};
        sims2::ps5::Log("AUDIO","ERROR","Template audio initialization failed");
        throw std::runtime_error("PS5 audio initialization failed");
    }
    audio_started=true;
    sims2::ps5::Log("AUDIO","INFO","48 kHz stereo output initialized");
}
}
