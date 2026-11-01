// SPDX-License-Identifier: GPL-3.0-only
#include "game/install_filesystem.h"
#include <fstream>
#include <iostream>
int main(int argc,char** argv){
 namespace fs=std::filesystem;using sims2::install::files::NoFollow;
 if(argc!=2)return 2;fs::path root=argv[1];fs::create_directory(root);
 std::ofstream(root/"data")<<"fixture";fs::create_directory(root/"folder");
 fs::create_symlink(root/"data",root/"link");fs::create_symlink(root/"absent",root/"broken");
 if(!fs::is_regular_file(NoFollow(root/"data")) || !fs::is_directory(NoFollow(root/"folder")) || fs::exists(NoFollow(root/"absent")))return 3;
 for(auto name:{"link","broken"}){bool refused=false;try{NoFollow(root/name);}catch(const fs::filesystem_error&){refused=true;}if(!refused)return 4;}
 std::cout<<"PASS_POSIX_METADATA: real descriptor status; regular/directory/missing; live and dangling links refused\n";
}
