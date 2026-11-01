#include "contracts.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

namespace sims2::ps5 {
namespace {
int16_t Stick(float value) {
    if (!std::isfinite(value)) return 0;
    value = std::clamp(value, -1.f, 1.f);
    return static_cast<int16_t>(std::lround(value * (value < 0 ? 32768.f : 32767.f)));
}
uint8_t Trigger(float value) {
    return std::isfinite(value) ? static_cast<uint8_t>(std::lround(std::clamp(value, 0.f, 1.f)*255.f)) : 0;
}
}

input::Pad MapPad(const PadSample& s) {
    input::Pad p{};
    if (!s.connected || (s.held & 0x80000000u)) return p;
    p.connected = true;
    // Xbox digital layout: up/down/left/right/start/back/L3/R3.
    constexpr uint32_t buttons[] = {0x10, 0x40, 0x80, 0x20, 0x8, 0x100000, 0x2, 0x4};
    for (int i = 0; i < 8; i++) if (s.held & buttons[i]) p.buttons |= uint16_t(1u<<i);
    // Face layout and shoulders preserve Xbox positional controls.
    constexpr uint32_t analog[] = {0x4000, 0x2000, 0x8000, 0x1000, 0x800, 0x400};
    for (int i = 0; i < 6; i++) p.analog[i] = s.held & analog[i] ? 255 : 0;
    p.analog[6] = Trigger(s.l2); p.analog[7] = Trigger(s.r2);
    p.lx = Stick(s.lx); p.ly = Stick(-s.ly); p.rx = Stick(s.rx); p.ry = Stick(-s.ry);
    return p;
}

bool TranslatePath(const std::string& guest, const std::string& assets,
                   const std::string& saves, std::string& result) {
    result.clear();
    if (guest.find('\0') != std::string::npos || guest.size() < 2 || guest[1] != ':') return false;
    char drive = guest[0];
    if (drive >= 'a' && drive <= 'z') drive -= 'a'-'A';
    if (drive != 'D' && drive != 'T' && drive != 'U') return false;
    const std::string& mount = drive == 'D' ? assets : saves;
    if (mount.empty()) return false;
    std::vector<std::string> parts;
    if (drive != 'D') parts.emplace_back(1, drive);
    const size_t floor = parts.size();
    std::string part;
    auto consume = [&]() {
        if (part == "..") {
            if (parts.size() == floor) return false;
            parts.pop_back();
        } else if (!part.empty() && part != ".") {
            if (part.find(':') != std::string::npos) return false;
            parts.push_back(part);
        }
        part.clear(); return true;
    };
    for (size_t i = 2; i < guest.size(); i++) {
        if (guest[i] == '/' || guest[i] == '\\') { if (!consume()) return false; }
        else part += guest[i];
    }
    if (!consume()) return false;
    result = mount;
    while (result.size() > 1 && result.back() == '/') result.pop_back();
    for (const auto& item : parts) result += '/' + item;
    return true;
}

bool ToGuest(uintptr_t base, uintptr_t native, uint32_t& guest) {
    if (native < base || uint64_t(native-base) >= (1ull<<32)) return false;
    guest = static_cast<uint32_t>(native-base); return true;
}
bool ToNative(uintptr_t base, uint32_t guest, uint64_t bytes, uintptr_t& native) {
    constexpr uint64_t size = 1ull<<32;
    if (bytes > size-guest || base > std::numeric_limits<uintptr_t>::max()-guest) return false;
    const uintptr_t address = base+guest;
    if (bytes && bytes-1 > std::numeric_limits<uintptr_t>::max()-address) return false;
    native = address; return true;
}
}
