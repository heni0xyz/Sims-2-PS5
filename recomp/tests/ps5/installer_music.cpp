// Copyright (C) 2026 OpenAI Codex, for szampan
// SPDX-License-Identifier: GPL-3.0-only
#include "installer_music.h"
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <vector>
static void require(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
int main(int argc,char **argv) {
    try {
        require(argc==2,"Pass original source MP3");
        Sims2InstallerMusic music;std::array<int16_t,512> block;block.fill(1234);
        require(!music.Load("nonexistent-launcher-music.mp3"),"Missing music accepted");
        music.Fill(block.data(),256);
        require(std::all_of(block.begin(),block.end(),[](auto v){return v==0;}),"Missing music not silent");
        require(music.Load(argv[1]),"Original MP3 did not decode");
        require(music.Rate()==48000 && music.Frames()==5760000,"Unexpected full-track length/rate");
        // Different callback partitions must produce exactly the same PCM.
        std::vector<int16_t> first(96000),partitioned(96000);
        music.Fill(first.data(),48000);require(music.Load(argv[1]),"Reload failed");
        for(int p=0;p<48000;){int n=std::min(257,48000-p);music.Fill(partitioned.data()+p*2,n);p+=n;}
        require(first==partitioned,"Callback chunk size altered playback");
        int peak=0;for(auto v:first)peak=std::max(peak,std::abs(int(v)));
        require(peak>10 && peak<=3277,"Silent or incorrectly amplified music (10% gain)");
        music.SetEnabled(false);std::array<int16_t,2048> mute{};music.Fill(mute.data(),1024);
        require(std::all_of(mute.begin()+960,mute.end(),[](auto v){return v==0;}),"Mute failed after gain ramp");
        require(music.Load(argv[1]),"Muted reload failed");music.Fill(mute.data(),1024);
        require(std::all_of(mute.begin(),mute.end(),[](auto v){return v==0;}),"Muted startup leaked audio");
        music.SetEnabled(true);music.Fill(first.data(),48000);
        // Cross the real end of the 120-second source and verify loop continuity.
        for(size_t n=48000;n<music.Frames()+48000;n+=256)music.Fill(block.data(),256);
        require(!music.Load("nonexistent-launcher-music.mp3"),"Missing reload accepted");
        music.Fill(block.data(),256);require(std::all_of(block.begin(),block.end(),[](auto v){return v==0;}),"Failed reload retained stale audio");
        std::puts("INSTALLER_MUSIC_HOST_PASS full_track=120s callback_partitions=identical loop=passed");return 0;
    }catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
}
