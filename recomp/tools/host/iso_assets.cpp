// Owner-disc extraction glue by OpenAI Codex for szampan.
// Disc parser: MIT XboxXBERecomp, Copyright 2026 TGP482; sdk/LICENSE.
#include "kernel/disc.h"
#include <algorithm>
#include <cstdio>
#include <fstream>

static bool SafeName(const std::string& name) {
    return !name.empty() && name != "." && name != ".." &&
        name.find_first_of("/\\:") == std::string::npos;
}

static bool Extract(kernel::Disc& disc, const kernel::Disc::Entry& entry,
                    const std::filesystem::path& parent, bool root_only) {
    if (!SafeName(entry.name)) {
        std::fprintf(stderr, "Unsafe disc member name; extraction refused\n");
        return false;
    }
    const auto path = parent / entry.name;
    if (entry.dir) {
        if (root_only) return true;
        std::filesystem::create_directory(path);
        for (const auto& child : entry.children)
            if (!Extract(disc, child, path, false)) return false;
        return true;
    }
    std::ofstream stream(path, std::ios::binary);
    std::vector<char> buffer(1024 * 1024);
    uint64_t pos = 0;
    while (pos < entry.size) {
        const auto want = uint32_t(std::min<uint64_t>(buffer.size(), entry.size - pos));
        if (disc.Read(entry, pos, buffer.data(), want) != want) {
            std::fprintf(stderr, "Short ISO read: %s\n", entry.name.c_str());
            return false;
        }
        stream.write(buffer.data(), want);
        if (!stream) return false;
        pos += want;
    }
    stream.close();
    if (!stream) return false;
    std::printf("EXTRACTED bytes=%u %s\n", entry.size, path.string().c_str());
    return true;
}

int main(int argc, char** argv) {
    if (argc < 3 || argc > 4 || (argc == 4 && std::string(argv[3]) != "--root-only")) {
        std::fprintf(stderr, "usage: iso_assets <owner.iso> <new-directory> [--root-only]\n");
        return 2;
    }
    try {
        const std::filesystem::path output = argv[2];
        if (std::filesystem::exists(output)) {
            std::fprintf(stderr, "Output directory exists; refusing to overwrite\n");
            return 2;
        }
        kernel::Disc disc;
        std::string error;
        if (!disc.Open(argv[1], error)) {
            std::fprintf(stderr, "ISO: %s\n", error.c_str());
            return 1;
        }
        const auto* root = disc.Find(std::vector<std::string>{});
        if (!root) return 1;
        std::filesystem::create_directories(output);
        for (const auto& entry : root->children)
            if (!Extract(disc, entry, output, argc == 4)) return 1;
        return 0;
    } catch (const std::exception& e) {
        std::fprintf(stderr, "Extraction failed: %s\n", e.what());
        return 1;
    }
}
