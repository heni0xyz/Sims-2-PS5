// Copyright (C) 2026 OpenAI Codex, for szampan
// SPDX-License-Identifier: GPL-3.0-only
#include "third_party/bc7enc.h"
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <thread>
#include <vector>
int main(int argc,char **argv) {
  try {
    if(argc!=5)throw std::runtime_error("Usage: encode_bc7 INPUT.rgba WIDTH HEIGHT OUTPUT.dds");
    const int w=std::stoi(argv[2]),h=std::stoi(argv[3]);
    if(w<4 || h<4 || w>8192 || h>8192 || w%4 || h%4)throw std::runtime_error("Invalid BC7 dimensions");
    std::ifstream file(argv[1],std::ios::binary);std::vector<uint8_t> rgba((std::istreambuf_iterator<char>(file)),{});
    if(rgba.size()!=size_t(w)*h*4)throw std::runtime_error("Wrong input length");
    std::vector<uint8_t> out(148+size_t(w)*h,0);
    auto put=[&](size_t off,uint32_t value){for(int b=0;b<4;++b)out[off+b]=uint8_t(value>>(8*b));};
    std::memcpy(out.data(),"DDS ",4);put(4,124);put(8,0xA1007);put(12,h);put(16,w);
    put(20,w*h);put(24,1);put(28,1);put(76,32);put(80,4);std::memcpy(out.data()+84,"DX10",4);
    put(108,0x1000);put(128,98);put(132,3);put(140,1);put(144,1);
    bc7enc_compress_block_init();
    auto encode=[&](int parity){
      bc7enc_compress_block_params params;bc7enc_compress_block_params_init(&params);
      params.m_uber_level=1;params.m_max_partitions=64;
      uint8_t block[64];
      for(int y=parity;y<h/4;y+=2)for(int x=0;x<w/4;++x){
        for(int row=0;row<4;++row)std::memcpy(block+row*16,rgba.data()+(size_t(y*4+row)*w+x*4)*4,16);
        bc7enc_compress_block(out.data()+148+(size_t(y)*(w/4)+x)*16,block,&params);
      }
    };
    std::thread a(encode,0);encode(1);a.join();
    std::ofstream target(argv[4],std::ios::binary);target.write((char*)out.data(),out.size());
    if(!target)throw std::runtime_error("Cannot write BC7 output");return 0;
  }catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
}
