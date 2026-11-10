// Copyright (C) 2026 OpenAI Codex, for szampan
// SPDX-License-Identifier: GPL-3.0-only
#include "installer_music.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <memory>
#define MINIMP3_IMPLEMENTATION
#define MINIMP3_NO_STDIO
#include "third_party/minimp3_ex.h"

bool Sims2InstallerMusic::Load(const char *path) {
    pcm.clear();rate=channels=0;position=0;gain=Enabled()?.10:0;
    std::unique_ptr<FILE,decltype(&std::fclose)> file(std::fopen(path,"rb"),std::fclose);
    if(!file || std::fseek(file.get(),0,SEEK_END)!=0)return false;
    auto bytes=std::ftell(file.get());
    if(bytes<=0 || bytes>16*1024*1024 || std::fseek(file.get(),0,SEEK_SET)!=0)return false;
    std::vector<uint8_t> source(static_cast<size_t>(bytes));
    if(std::fread(source.data(),1,source.size(),file.get())!=source.size())return false;
    mp3dec_ex_t decoder{};
    if(mp3dec_ex_open_buf(&decoder,source.data(),source.size(),MP3D_SEEK_TO_SAMPLE))return false;
    // An eight-minute bound prevents malformed input from expanding without limit.
    bool valid=(decoder.info.channels==1 || decoder.info.channels==2) &&
               decoder.info.hz>=8000 && decoder.info.hz<=48000 && decoder.samples &&
               decoder.samples<=uint64_t(decoder.info.hz)*decoder.info.channels*480;
    if(valid) {
        channels=decoder.info.channels;rate=decoder.info.hz;
        pcm.resize(static_cast<size_t>(decoder.samples));
        auto got=mp3dec_ex_read(&decoder,pcm.data(),pcm.size());
        valid=got==pcm.size() && got%channels==0 && !decoder.last_error;
    }
    mp3dec_ex_close(&decoder);
    if(!valid){pcm.clear();rate=channels=0;}
    return valid;
}
void Sims2InstallerMusic::Fill(int16_t *output,int frames) {
    if(frames<=0)return;
    if(pcm.empty()){std::fill_n(output,size_t(frames)*2,0);return;}
    const size_t count=Frames();
    const double step=double(rate)/48000;
    const double target=Enabled()?.10:0;
    for(int frame=0;frame<frames;++frame) {
        size_t a=static_cast<size_t>(position),b=(a+1)%count;
        double fraction=position-a;
        // Quiet background level and 20ms end fades avoid a click at loop wrap.
        double edge=std::min({1.0,position/(rate*.020),(count-position)/(rate*.020)});
        // Ten milliseconds to the requested gain avoids a click when muting.
        gain+=std::clamp(target-gain,-.10/480,.10/480);
        for(int c=0;c<2;++c) {
            int channel=channels==1?0:c;
            double sample=pcm[a*channels+channel]*(1-fraction)+pcm[b*channels+channel]*fraction;
            output[frame*2+c]=int16_t(std::clamp(sample*gain*edge,-32768.0,32767.0));
        }
        position+=step;if(position>=count)position=std::fmod(position,double(count));
    }
}
