// SPDX-License-Identifier: GPL-3.0-only
// Layout/rasterizer: OpenAI Codex for szampan. Font attribution below retained.
#include "game/install_screen.h"
#include "game/install_font.h"
#include <algorithm>
#include <cstdlib>
#include <string_view>
namespace {
struct Canvas {
    sims2::install::Icons icons;
    std::vector<uint32_t> pixels=std::vector<uint32_t>(3840*2160,0xff171b26);
    void NativePixel(int x,int y,uint32_t colour){if(x>=0 && y>=0 && x<3840 && y<2160)pixels[y*3840+x]=colour;}
    void Pixel(int x,int y,uint32_t colour){for(int row=int(y*4.5);row<int((y+1)*4.5);++row)for(int col=x*6;col<(x+1)*6;++col)NativePixel(col,row,colour);}
    void Text(std::string_view text,int y,unsigned scale=2,uint32_t colour=0xfff3f4f8){
        sims2::install::RasterText(pixels,text,int(y*4.5),scale,colour);
    }
    // Original elementary geometry, matching filled-white PS button styling.
    void Button(int x,int y,bool cross){
        const auto icon=cross?icons.cross:icons.circle;
        if(icon.size()==256*256*4){
            for(unsigned row=0;row<108;++row)for(unsigned col=0;col<108;++col){
                const auto at=((255-row*256/108)*256+col*256/108)*4;
                const auto alpha=icon[at+3];const uint32_t background=0xff171b26;
                uint32_t colour=0xff000000;
                for(unsigned channel=0;channel<3;++channel)colour|=uint32_t((icon[at+channel]*alpha+((background>>(8*channel))&255)*(255-alpha)+127)/255)<<(8*channel);
                NativePixel(x*6-54+col,int(y*4.5)-54+row,colour);
            }
            return;
        }
        for(int dy=-12;dy<=12;++dy)for(int dx=-12;dx<=12;++dx){
            if(dx*dx+dy*dy>144)continue;Pixel(x+dx,y+dy,0xfff3f4f8);
            if(cross ? (std::abs(dx-dy)<2 || std::abs(dx+dy)<2) && std::abs(dx)<7 && std::abs(dy)<7
                     : dx*dx+dy*dy>=25 && dx*dx+dy*dy<=49)Pixel(x+dx,y+dy,0xff171b26);
        }
    }
    void Prompt(std::string_view text,int y,bool cross){Text(text,y,2);const int left=(3840-sims2::install::TextWidth(text,2))/12;Button(left-24,y+15,cross);}
    void Error(const std::string& error){
        const auto text=error.substr(0,512);size_t at=0;int line=0;
        while(at<text.size() && line<5){
            size_t end=at;
            while(end<text.size() && sims2::install::TextWidth(std::string_view(text).substr(at,end-at+1),1)<3200)++end;
            if(end==at)++end;
            if(end<text.size()){auto space=text.rfind(' ',end);if(space!=text.npos && space>at)end=space;}
            Text(std::string_view(text).substr(at,end-at),250+line*18,1,0xffffa3a3);
            at=end;while(at<text.size() && text[at]==' ')++at;++line;
        }
    }
};
}
sims2::install::Action sims2::install::Choice::Poll(bool cross,bool circle,bool held,bool can_extract){
    if(!armed){if(!held)armed=true;return Action::None;}
    if(screen==Screen::DeleteQuestion){if(circle)return Action::KeepIso;if(cross)return Action::DeleteIso;}
    if(screen==Screen::DeleteError && circle)return Action::KeepIso;
    if((screen==Screen::Missing || screen==Screen::Error) && can_extract && cross)return Action::Extract;
    return Action::None;
}
std::vector<uint32_t> sims2::install::DrawInstallScreen(const View& view,Icons icons){
    Canvas canvas;canvas.icons=icons;canvas.Text("The Sims 2",115,3,0xffa4eac0);
    switch(view.screen){
    case Screen::Missing:
        canvas.Text("No game data detected;",188);
        canvas.Text("extract from .iso in PPSA99145",212);
        if(view.can_extract)canvas.Prompt("Extract",280,true);
        else canvas.Text("Add one USA Xbox .iso beside eboot.bin",282,1);
        canvas.Text("Your source ISO will stay unchanged during extraction.",348,1);break;
    case Screen::Copying:
        canvas.Text("Extracting... ("+std::to_string(std::min(view.percent,99u))+"%)",214);
        for(unsigned x=0;x<360;++x)for(unsigned y=0;y<6;++y)canvas.Pixel(140+x,265+y,x*100<360*std::min(view.percent,99u)?0xffa4eac0:0xff384052);
        canvas.Text("Keep the game open until installation finishes.",315,1);break;
    case Screen::Error:
        canvas.Text("Extracting error",208,2,0xffffa3a3);canvas.Error(view.error);
        if(view.can_extract)canvas.Prompt("Retry",365,true);break;
    case Screen::DeleteQuestion:
        canvas.Text("Installation complete.",188);canvas.Text("Delete source .iso?",214);
        canvas.Prompt("Delete ISO",274,true);canvas.Prompt("Keep ISO",314,false);
        canvas.Text("Only the source ISO is removed; installed data and saves stay.",368,1);break;
    case Screen::DeleteError:
        canvas.Text("Could not delete source ISO.",188);canvas.Text("Installed game data is ready.",212);
        canvas.Error(view.error);canvas.Prompt("Continue",328,false);break;
    }
    return std::move(canvas.pixels);
}
