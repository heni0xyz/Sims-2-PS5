// SPDX-License-Identifier: GPL-3.0-only
// Read-only preflight using the exact same disc bootstrap as the PS5 title.
#include "game/disc_boot.h"
#include "kernel/disc.h"
#include "core/log.h"
#include <cstdio>
#include <cstdarg>
namespace logx {void Write(Level,const char*,...) {}}
int main(int argc,char** argv){
    if(argc!=2){std::fputs("usage: iso_verify <owner.iso>\n",stderr);return 2;}
    app::GameFiles game;std::string error;
    if(!sims2::disc::Open(game,argv[1],error)){std::fprintf(stderr,"ISO_REFUSED: %s\n",error.c_str());return 1;}
    // Read real data from the image through the mounted-disc streaming path.
    const auto* disc=kernel::MountedDisc();
    const auto* entry=disc->Find("DATA\\textures.arc");uint8_t probe[32]{};
    if(!entry || disc->Read(*entry,0,probe,sizeof(probe))!=sizeof(probe) ||
        disc->Read(*entry,entry->size-1,probe,sizeof(probe))!=1 ||
        disc->Read(*entry,entry->size,probe,sizeof(probe))!=0){std::fputs("ISO_READ_FAILED\n",stderr);return 1;}
    // Failed input must not unmount the verified owner image.
    const auto* mounted=disc;
    app::GameFiles rejected;
    if(sims2::disc::Open(rejected,std::filesystem::path(argv[1])/"missing",error) || kernel::MountedDisc()!=mounted){
        std::fputs("ISO_FAILURE_CHANGED_MOUNT\n",stderr);return 1;
    }
    std::puts("{\"status\":\"PASS_ISO_PREFLIGHT\",\"xbe_sha1\":\"16f9e41ce26c315a758f25d43ba1fa9d3809d1ad\",\"stream_probe\":\"PASS\",\"failed_mount_preserved\":true,\"scope\":\"Host preflight, not gameplay or console proof\"}");
}
