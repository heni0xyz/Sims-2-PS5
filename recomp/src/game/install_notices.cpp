// SPDX-License-Identifier: GPL-3.0-only
// Equal-size visible English notice patch. Original ISO is never written.
#include "game/install.h"
#include "kernel/crypto.h"
#include <algorithm>
#include <cstring>
#include <string_view>
namespace {
std::string Hash(const std::vector<uint8_t>& bytes){
    alignas(8) uint8_t state[128]{},digest[20]{};crypto::ShaInit(state);crypto::ShaUpdate(state,bytes.data(),bytes.size());crypto::ShaFinal(state,digest);
    std::string result;for(auto value:digest){result+="0123456789abcdef"[value>>4];result+="0123456789abcdef"[value&15];}return result;
}
uint32_t U32(const std::vector<uint8_t>& bytes,size_t at){uint32_t value;std::memcpy(&value,bytes.data()+at,4);return value;}
std::vector<uint8_t> Utf16(std::string_view text,size_t width){
    std::vector<uint8_t> out(width*2);for(size_t i=0;i<width;++i)out[i*2]=i<text.size()?uint8_t(text[i]):' ';return out;
}
bool Replace(std::vector<uint8_t>& bytes,size_t from,size_t size,std::string_view before,std::string_view after,unsigned count){
    if(after.size()>before.size())return false;
    auto old=Utf16(before,before.size()),next=Utf16(after,before.size());std::vector<size_t> matches;
    for(auto at=bytes.begin()+from,end=at+size;at!=end;){
        auto found=std::search(at,end,old.begin(),old.end());if(found==end)break;
        matches.push_back(size_t(found-bytes.begin()));at=found+old.size();
    }
    if(matches.size()!=count)return false;
    for(auto at:matches)std::copy(next.begin(),next.end(),bytes.begin()+at);return true;
}
}
bool sims2::install::PatchNotices(std::vector<uint8_t>& bytes,std::string& error){
    error.clear();
    if(bytes.size()!=14497786 || Hash(bytes)!="a5dc3658dd9d51bf4dada7664c04b0c8e9d8dde4") {error="Unsupported original USA UI notice archive";return false;}
    const auto index=U32(bytes,0);
    if(U32(bytes,4)!=0x10000 || index<16 || index>bytes.size()-4){error="Invalid UI archive index";return false;}
    const auto count=U32(bytes,index);size_t cursor=index+4,start=0,size=0;
    if(!count || count>100000){error="Invalid UI archive count";return false;}
    for(unsigned i=0;i<count;++i){
        if(cursor+12>bytes.size()){error="Truncated UI archive index";return false;}
        const auto offset=U32(bytes,cursor+4),length=U32(bytes,cursor+8);cursor+=12;
        const auto end=std::find(bytes.begin()+cursor,bytes.end(),0);const auto name_end=size_t(end-bytes.begin());
        if(name_end-cursor>256 || name_end+9>bytes.size() || offset<16 || uint64_t(offset)+length>index){error="Invalid UI archive member";return false;}
        if(std::string_view(reinterpret_cast<const char*>(bytes.data()+cursor),name_end-cursor)=="urbzui"){
            if(start){error="Duplicate UI notice resource";return false;}start=offset;size=length;
        }
        cursor=name_end+9;
    }
    if(!start || cursor!=bytes.size()){error="Missing UI notice resource";return false;}
    // Work on a separate buffer: an unexpected phrase/count never partially
    // patches the caller's archive. The final fingerprint checks every byte.
    auto patched=bytes;
    if(!Replace(patched,start,size,"Xbox console.","PlayStation. ",22) ||
       !Replace(patched,start,size,"Checking hard disk.  Please don't turn off your PlayStation. ","Checking storage device. Don't turn off your PlayStation.",10) ||
       !Replace(patched,start,size,"No saved game found on hard disk.","No save found on storage device.",4) ||
       !Replace(patched,start,size,"Load saved game from the hard disk?","Load save from the storage device?",3) ||
       Hash(patched)!="35a76c78a1b20ccffa3c75431550ebd5e6a5dc67"){
        error="UI notice patch verification failed";return false;
    }
    bytes.swap(patched);return true;
}
