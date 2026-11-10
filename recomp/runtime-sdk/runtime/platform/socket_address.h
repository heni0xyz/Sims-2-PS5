#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>

// Xbox uses Winsock's little-endian 16-bit family. BSD puts an 8-bit
// length before its 8-bit family; copying the structures is not ABI-safe.
namespace xnet::address {
inline constexpr std::size_t kGuestIPv4Size=16;
template<class Native> void SetFamily(Native& value,uint16_t family) {
    if constexpr (sizeof(value.sin_family)==1)
        value.sin_family=static_cast<decltype(value.sin_family)>(family>255?255:family);
    else value.sin_family=static_cast<decltype(value.sin_family)>(family);
    if constexpr (requires {value.sin_len;}) value.sin_len=sizeof(Native);
}
template<class Native> Native Decode(const void* guest) {
    static_assert(sizeof(Native)==kGuestIPv4Size && sizeof(Native{}.sin_zero)==8);
    const auto* bytes=static_cast<const uint8_t*>(guest);
    Native result{};
    SetFamily(result,uint16_t(bytes[0])|(uint16_t(bytes[1])<<8));
    std::memcpy(&result.sin_port,bytes+2,2);
    std::memcpy(&result.sin_addr.s_addr,bytes+4,4);
    std::memcpy(result.sin_zero,bytes+8,8);
    return result;
}
template<class Native> void Encode(void* guest,const Native& value) {
    static_assert(sizeof(Native)==kGuestIPv4Size && sizeof(Native{}.sin_zero)==8);
    auto* bytes=static_cast<uint8_t*>(guest);
    const uint16_t family=static_cast<uint16_t>(value.sin_family);
    bytes[0]=uint8_t(family);bytes[1]=uint8_t(family>>8);
    std::memcpy(bytes+2,&value.sin_port,2);
    std::memcpy(bytes+4,&value.sin_addr.s_addr,4);
    std::memcpy(bytes+8,value.sin_zero,8);
}
}
