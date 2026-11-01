// Asset-free regression for the actual diagnostic texture reader/cache.
// OpenAI Codex for szampan. SDK implementation remains credited to TGP482.
#include <cstdio>
#include <cstdlib>
#include "../runtime-sdk/harness/reference_renderer.cpp"

namespace kernel { uint8_t* g_base = nullptr; }
// Synthetic local arena only; the title uses the real runtime memory manager.
namespace mem { uint32_t VirtualFromPhysical(uint32_t address) { return address; } }

namespace {
unsigned checks = 0;
void Require(bool ok, const char* what) {
    ++checks;
    if (!ok) { std::fprintf(stderr, "FAIL: %s\n", what); std::exit(1); }
}
bool Colour(const reference::Vec& value, uint32_t argb) {
    const auto expected = reference::Unpack(argb);
    for (int i = 0; i < 4; ++i)
        if (std::fabs(value[i] - expected[i]) > 0.00001f) return false;
    return true;
}
}

int main() {
    std::vector<uint8_t> arena(65536);
    kernel::g_base = arena.data();
    constexpr uint32_t texture = 0x100, palette = 0x200, data = 0x400, colours = 0x1000;
    kernel::W32(texture, 0x40001);
    kernel::W32(texture + 4, data);
    kernel::W32(texture + 12, 0x01110B20); // swizzled 2x2 P8, one level
    kernel::W32(palette + 4, colours);
    const uint32_t expected[] = {0x00112233, 0x804080C0, 0xFF00FF00, 0xFFFF0000};
    for (unsigned i = 0; i < 4; ++i) {
        arena[data + i] = uint8_t(i);
        kernel::W32(colours + i * 4, expected[i]);
    }
    reference::BeginFrame();
    auto first = reference::FindImage(texture, palette);
    Require(first && first->w == 2 && first->h == 2, "P8 texture dimensions");
    for (unsigned i = 0; i < 4; ++i)
        Require(Colour(first->texels[0][i], expected[i]), "palette RGB and alpha lookup");
    Require(reference::FindImage(texture, palette) == first, "unchanged palette cache reuse");
    kernel::W32(colours + 4, 0xFFFF00FF);
    auto changed = reference::FindImage(texture, palette);
    Require(changed != first && Colour(changed->texels[0][1], 0xFFFF00FF), "same-frame palette update invalidation");
    kernel::W32(0x240, 0);
    kernel::W32(0x244, 0x2000);
    kernel::W32(0x2004, 0xFF123456);
    auto alternate = reference::FindImage(texture, 0x240);
    Require(Colour(alternate->texels[0][1], 0xFF123456), "same texture with another stage palette");
    Require(reference::FindImage(texture, palette) == changed, "original palette cache remains distinct");
    for (unsigned code = 0; code < 4; ++code) {
        kernel::W32(palette, code << 30);
        const unsigned count = 256u >> code;
        arena[data] = uint8_t(count - 1);
        kernel::W32(colours + (count - 1) * 4, 0xAA123456);
        reference::BeginFrame();
        Require(Colour(reference::FindImage(texture, palette)->texels[0][0], 0xAA123456), "palette size last valid entry");
        if (code) {
            arena[data] = uint8_t(count);
            reference::BeginFrame();
            Require(Colour(reference::FindImage(texture, palette)->texels[0][0], 0), "short palette unused entries are transparent");
        }
    }
    std::printf("PASS: %u reference palette checks (synthetic data, no GPU/game assets)\n", checks);
}
