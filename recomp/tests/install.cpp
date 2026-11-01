// SPDX-License-Identifier: GPL-3.0-only
#include "game/install.h"
#include "game/install_screen.h"
#include "game/install_font.h"
#include "core/log.h"
#include <cstdio>
#include <fstream>
#include <stdexcept>
namespace logx {void Write(Level,const char*,...) {}}
namespace fs=std::filesystem;
void Need(bool condition,const char* reason){if(!condition)throw std::runtime_error(reason);}
int main(int argc,char** argv){try{
    Need(argc==3,"temporary test root and public font required");const auto base=fs::absolute(argv[1]);
    std::ifstream font_file(argv[2],std::ios::binary);std::vector<uint8_t> font((std::istreambuf_iterator<char>(font_file)),{});
    sims2::install::LoadInstallFont(font);
    Need(sims2::install::TextWidth("iii",2)<sims2::install::TextWidth("WWW",2),"Open Sans proportional metrics");
    auto damaged_font=font;damaged_font.back()^=1;bool font_refused=false;
    try{sims2::install::LoadInstallFont(damaged_font);}catch(const std::runtime_error&){font_refused=true;}
    Need(font_refused,"corrupt font resource refused without replacing valid font");
    auto root=[&](const char* name){auto path=base/name;Need(fs::create_directory(path),"fresh fixture root");return path;};
    std::vector<sims2::install::File> files{{"DATA/test.dat",(1u<<20)+123},{"movies/test.dat",515}};
    auto read=[](size_t index,uint64_t at,std::span<uint8_t> out){for(size_t i=0;i<out.size();++i)out[i]=uint8_t((at+i)*31+index);return true;};
    auto check=[&](const fs::path& path){for(size_t i=0;i<files.size();++i){std::ifstream stream(path/"assets"/files[i].name,std::ios::binary);std::vector<uint8_t> actual((std::istreambuf_iterator<char>(stream)),{}),expected(files[i].bytes);read(i,0,expected);Need(actual==expected,"actual copied bytes");}};
    std::string error;uint64_t previous=0;unsigned complete=0;const auto valid=root("valid");
    Need(sims2::install::Extract(valid,files,read,{},[&](auto status){Need(status.done>=previous && status.done<=status.total,"monotonic bounded progress");previous=status.done;if(status.stage==sims2::install::Stage::Complete){Need(fs::exists(valid/"assets/DATA/test.dat"),"completion follows promotion");complete++;}return true;},error),error.c_str());
    Need(complete==1 && !fs::exists(valid/".sims2-installing"),"exactly one committed install");check(valid);
    Need(!sims2::install::Extract(valid,files,read,{},{},error),"existing assets refused");check(valid);
    const auto cancel=root("cancel");
    Need(!sims2::install::Extract(cancel,files,read,{},[](auto status){return status.done<(1u<<20);},error),"cancel fails closed");
    Need(!fs::exists(cancel/"assets"),"cancelled install never promoted");
    Need(sims2::install::Extract(cancel,files,read,{},{},error),"known incomplete staging can be retried");check(cancel);
    const auto unknown=root("unknown");
    Need(!sims2::install::Extract(unknown,files,[](size_t,uint64_t,std::span<uint8_t>){return false;},{},{},error),"source read failure propagated");
    {std::ofstream file(unknown/".sims2-installing/personal-note.txt");file<<"preserve";}
    Need(!sims2::install::Extract(unknown,files,read,{},{},error),"unknown staging content refuses cleanup");
    Need(fs::file_size(unknown/".sims2-installing/personal-note.txt")==8,"unknown file preserved");
    for(const auto& name:{"../escape","/absolute","DATA/../escape","DATA\\escape","DATA/CON","DATA/a.","DATA//a"}){
        const auto bad=root(("bad"+std::to_string(complete++)).c_str());
        Need(!sims2::install::Extract(bad,{{name,10}},read,{},{},error),"unsafe path refused");Need(!fs::exists(bad/"assets"),"unsafe install not published");
    }
    Need(!sims2::install::Extract(root("duplicate"),{{"a.dat",1},{"A.dat",1}},read,{},{},error),"case-folded duplicate refused");
    const auto patch=root("patch-fail");
    Need(!sims2::install::Extract(patch,files,read,[](const auto&,std::string& reason){reason="synthetic patch failure";return false;},{},error),"patch failure propagated");
    Need(!fs::exists(patch/"assets") && error=="synthetic patch failure","failed patch never publishes assets");
    std::vector<uint8_t> unsupported(32,42),original=unsupported;
    Need(!sims2::install::PatchNotices(unsupported,error) && unsupported==original,"unsupported notice data preserved");
    sims2::install::Choice choice;
    Need(choice.Poll(true,false,true,true)==sims2::install::Action::None,"held button cannot extract on entry");
    choice.Poll(false,false,false,true);
    Need(choice.Poll(true,false,true,true)==sims2::install::Action::Extract,"fresh extract press");
    choice.Set(sims2::install::Screen::DeleteQuestion);
    Need(choice.Poll(true,false,true,true)==sims2::install::Action::None,"extract press cannot carry into delete confirmation");
    choice.Poll(false,false,false,true);
    Need(choice.Poll(false,true,true,true)==sims2::install::Action::KeepIso,"explicit keep choice");
    choice.Set(sims2::install::Screen::DeleteQuestion);choice.Poll(false,false,false,true);
    Need(choice.Poll(true,false,true,true)==sims2::install::Action::DeleteIso,"new delete confirmation required");
    const auto deletion=root("deletion");const auto iso=deletion/"source.iso";{std::ofstream file(iso);file<<"fixture";}
    sims2::install::Source source;
    Need(sims2::install::RememberSource(deletion,iso,source,error),"remember literal source file");
    Need(!sims2::install::DeleteSource(source,error) && fs::exists(iso),"failed install can never delete source");
    Need(sims2::install::DrawInstallScreen({}).size()==3840*2160,"standalone screen needs no game pixels");
    std::puts("INSTALL_PASS: real files/readbacks, atomic promotion, bounded progress, cancel/retry, unknown preservation, unsafe paths, duplicates, patch failure");return 0;
}catch(const std::exception& exception){std::fprintf(stderr,"INSTALL_FAIL: %s\n",exception.what());return 1;}}
