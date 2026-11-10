// SPDX-License-Identifier: GPL-3.0-only
// Exercise exactly the console's install transaction on the host, privately.
#include "game/install.h"
#include "core/log.h"
#include <cstdio>
#include <fstream>
namespace logx {void Write(Level,const char*,...) {}}
int main(int argc,char** argv){
    if(argc!=3 && !(argc==4 && std::string(argv[3])=="--test-delete-fixture")){std::fputs("usage: iso_install <owner.iso> <new-root-directory> [--test-delete-fixture]\n",stderr);return 2;}
    try{
        auto root=std::filesystem::absolute(argv[2]);if(!std::filesystem::exists(root))std::filesystem::create_directories(root);
        if(argc==4){
            if(std::filesystem::exists(root/"hdd"))throw std::runtime_error("Use a fresh root for the synthetic cache/save fixture");
            std::filesystem::create_directories(root/"hdd/Z");std::filesystem::create_directories(root/"hdd/E");
            {std::ofstream save(root/"hdd/E/synthetic-save.txt");save<<"keep this fixture";}
            {std::ofstream cache(root/"hdd/Z/old-cache.txt");cache<<"old cache fixture";}
            if(std::filesystem::exists(root/"assets"))throw std::runtime_error("Use a fresh root for the public placeholder fixture");
            std::filesystem::create_directory(root/"assets");
            {std::ofstream marker(root/"assets/README.txt",std::ios::binary);marker<<"SIMS2-INSTALLER-PLACEHOLDER-1\r\n";}
        }
        std::string error;unsigned last=101;
        if(!sims2::install::FromIso(argv[1],root,[&](auto status){auto percent=unsigned(status.done*100/status.total);if(percent!=last){std::printf("INSTALL_PROGRESS %u%%\n",percent);std::fflush(stdout);last=percent;}return true;},error)){
            std::fprintf(stderr,"INSTALL_REFUSED: %s\n",error.c_str());return 1;
        }
        app::GameFiles game;if(!sims2::install::Ready(root,game,error)){std::fprintf(stderr,"INSTALLED_DATA_REFUSED: %s\n",error.c_str());return 1;}
        if(argc==4){
            if(std::filesystem::exists(root/"assets/README.txt"))throw std::runtime_error("Public placeholder was not consumed");
            std::puts("PASS_PUBLIC_PLACEHOLDER: legacy Windows CRLF accepted before actual ISO extraction");
            if(!sims2::install::PrepareCache(root,error) || !std::filesystem::is_empty(root/"hdd/Z") ||
               std::filesystem::file_size(root/"hdd/Z.before-notice-install/old-cache.txt")!=17 ||
               std::filesystem::file_size(root/"hdd/E/synthetic-save.txt")!=17)throw std::runtime_error("Cache archive/save preservation fixture failed: "+error);
            std::puts("PASS_CACHE_FIXTURE: old Z cache archived; synthetic E save untouched; repeat preparation idempotent");
            const auto fixture=root/"source-delete-fixture.iso";
            if(std::filesystem::exists(fixture))throw std::runtime_error("Refusing to overwrite deletion fixture");
            {std::ofstream file(fixture);file<<"synthetic file, not the owner ISO";}
            sims2::install::Source snapshot;
            if(!sims2::install::RememberSource(root,fixture,snapshot,error))throw std::runtime_error(error);
            const auto data=root/"assets/sims2.bin";std::ifstream original(data,std::ios::binary);std::vector<char> copy((std::istreambuf_iterator<char>(original)),{});original.close();
            {std::ofstream changed(data,std::ios::binary|std::ios::app);changed<<'x';}
            if(sims2::install::Ready(root,game,error) || sims2::install::DeleteSource(snapshot,error) || !std::filesystem::exists(fixture))throw std::runtime_error("Incomplete installed-data deletion guard failed");
            {std::ofstream restore(data,std::ios::binary|std::ios::trunc);restore.write(copy.data(),copy.size());}
            {std::ofstream file(fixture,std::ios::app);file<<" changed";}
            if(sims2::install::DeleteSource(snapshot,error) || !std::filesystem::exists(fixture))throw std::runtime_error("Changed-source deletion guard failed");
            if(!sims2::install::RememberSource(root,fixture,snapshot,error) || !sims2::install::DeleteSource(snapshot,error) || std::filesystem::exists(fixture))throw std::runtime_error("Confirmed fixture deletion failed: "+error);
            std::puts("PASS_SOURCE_DELETE_FIXTURE: changed source refused; explicit fixture deletion succeeds; owner ISO not touched");
        }
        std::puts("PASS_OWNER_INSTALL: copied, checksummed, patched and promoted; source ISO unchanged; PS5 NOT TESTED");return 0;
    }catch(const std::exception& error){std::fprintf(stderr,"INSTALL_FAILED: %s\n",error.what());return 1;}
}
