// Copyright (C) 2026 OpenAI Codex, for szampan
// SPDX-License-Identifier: GPL-3.0-only
// Host asset compiler. Original MP3 metadata remains in artwork/audio.
#include "third_party/psgfx_at9/at9.h"
#include <fstream>
#include <iterator>
#include <iostream>
#include <stdexcept>

static void write(const char *name,const std::vector<uint8_t>& data) {
    std::ofstream f(name,std::ios::binary);f.write((const char*)data.data(),data.size());
    if(!f)throw std::runtime_error("Cannot write audio asset");
}
int main(int argc,char **argv) {
    try {
        if(argc!=4)throw std::runtime_error("Usage: make_home_sound SOURCE.mp3 snd0.at9 preview.wav");
        std::ifstream f(argv[1],std::ios::binary);if(!f)throw std::runtime_error("Cannot read source MP3");
        std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(f)),{});
        at9::Audio audio;std::string error;
        if(!at9::decode_file(bytes,audio,error))throw std::runtime_error(error);
        at9::Options options;
        // Full 120-second song fits the 2 MiB Home limit at 120 kb/s (192 does not).
        options.duration_sec=double(audio.pcm.size())/audio.channels/audio.rate;
        options.bitrate_kbps=120;options.fade_in=.02;options.fade_out=.02;
        options.loop=true;
        auto result=at9::detail::make_snd0_ex(audio,options,nullptr,nullptr,2);
        if(result.seconds!=120)throw std::runtime_error("Unexpected original track duration");
        write(argv[2],result.at9);write(argv[3],at9::wav_bytes(result.processed));
        std::cout<<"{\"input_rate\":"<<audio.rate<<",\"input_channels\":"<<audio.channels
                 <<",\"input_seconds\":"<<double(audio.pcm.size())/audio.channels/audio.rate
                 <<",\"clip_seconds\":120,\"bitrate_kbps\":120,\"loop\":true,\"bytes\":"
                 <<result.at9.size()<<",\"loudness_lufs\":"<<result.loudness_param<<"}\n";
        return 0;
    }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
