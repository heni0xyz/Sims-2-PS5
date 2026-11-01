// Build glue by OpenAI Codex for szampan. Disc parsing uses the upstream MIT SDK.
#include "kernel/disc.h"
#include <cstdio>
#include <cstring>
#include <fstream>
int main(int argc,char** argv) {
    if(argc!=3) {std::fprintf(stderr,"usage: iso_xbe <owner.iso> <new-default.xbe>\n");return 2;}
    const std::filesystem::path output=argv[2];
    if(std::filesystem::exists(output)) {std::fprintf(stderr,"Output exists; refusing to overwrite\n");return 2;}
    kernel::Disc disc;std::string error;
    if(!disc.Open(argv[1],error)) {std::fprintf(stderr,"ISO: %s\n",error.c_str());return 1;}
    std::vector<uint8_t> bytes;
    if(!disc.Load("default.xbe",bytes) || bytes.size()<4 || std::memcmp(bytes.data(),"XBEH",4)) {
        std::fprintf(stderr,"Supported XBE was not found in this ISO\n");return 1;
    }
    std::ofstream stream(output,std::ios::binary);
    stream.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());stream.close();
    if(!stream) {std::fprintf(stderr,"XBE output write failed\n");return 1;}
    std::printf("XBE_EXTRACTED bytes=%zu\n",bytes.size());return 0;
}
