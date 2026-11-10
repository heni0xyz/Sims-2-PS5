// HLSL to SPIR-V for the Vulkan backend. Two compilers, in this order:
//
// 1. DXC (libdxcompiler.so), loaded with dlopen so a machine without it still runs: the same
//    compiler family the Windows build's HLSL comes from, invoked through IDxcCompiler3 with
//    -spirv. CMake defines XBE_HAVE_DXC and adds /opt/dxc/include when DXC is installed.
// 2. glslang's HLSL front end (libglslang, linked): TShader with the HLSL input mode and the
//    HLSL functionality target, shifted so its bindings land where DXC's do.
//
// Both are asked for the one descriptor layout vulkan.cpp binds: cbuffers b0/b1 at bindings
// 0/1, textures t0..t3 at 2..5, samplers s0..s3 at 6..9, descriptor set 0. DXC gets it from
// -fvk-b/t/s-shift; glslang from setShiftBinding with HLSL IO mapping on.
#if !defined(_WIN32) || defined(XBE_VULKAN_HOST)

#include "gpu/spirv.h"
#include "gpu/cache_file.h"

#include <cstring>
#if defined(_WIN32)
#include <windows.h>
#include <process.h>
#else
#include <dlfcn.h>
#include <unistd.h>
#endif
#include <fstream>
#include <mutex>
#include <system_error>
#include <unordered_map>

#include "core/log.h"
#include "gpu/shaders.h"  // Hash

#if defined(XBE_HAVE_DXC) && __has_include(<dxc/dxcapi.h>)
#define XBE_SPIRV_DXC 1
#if !defined(_WIN32)
#include "dxc/WinAdapter.h"
#endif
#include "dxc/dxcapi.h"
#endif

#if __has_include(<glslang/Public/ShaderLang.h>)
#define XBE_SPIRV_GLSLANG 1
#ifndef ENABLE_HLSL
#define ENABLE_HLSL 1  // the distro libglslang is built with its HLSL front end; the headers gate its API on this
#endif
#include <glslang/Public/ResourceLimits.h>
#include <glslang/Public/ShaderLang.h>
#if __has_include(<glslang/SPIRV/GlslangToSpv.h>)
#include <glslang/SPIRV/GlslangToSpv.h>
#else
// glslang's source tree places SPIRV beside glslang; installed headers place
// it underneath. Cross builds can consume the pinned static build directly.
#include <SPIRV/GlslangToSpv.h>
#endif
#endif

namespace gpu {
namespace {

std::mutex g_lock;
std::unordered_map<uint64_t, std::vector<uint32_t>> g_cache;
std::filesystem::path g_dir;
std::string g_compiler = "none";

uint64_t CacheKey(const std::string& hlsl, const char* entry, bool fragment) {
    // Binding shifts were previously configured but never applied by mapIO.
    // Keep those structurally valid, layout-incompatible blobs out of this ABI.
    constexpr char compilerAbi[] = "hlsl-mapio-vulkan12-v3";
    uint64_t key = Hash(compilerAbi, sizeof(compilerAbi) - 1, fragment ? 0x7073ull : 0x7673ull);
    key = Hash(entry, std::strlen(entry), key);
    return Hash(hlsl.data(), hlsl.size(), key);
}

std::filesystem::path CacheFile(uint64_t key, bool fragment) {
    char name[48];
    std::snprintf(name, sizeof(name), "%016llx.%s.spv", (unsigned long long)key, fragment ? "ps" : "vs");
    return g_dir / name;
}

// D4: a cache blob is trusted only if it is a whole SPIR-V module, not merely a file
// whose first word is the magic (the magic is written first, so every truncated
// write used to pass). Checks: at least the 5-word header, a sane length, version
// 1.x, a non-zero sane id bound, reserved schema word 0, and an instruction walk
// that lands exactly on the end (a truncation behind the header ends mid-word-count).
bool ValidSpirv(const std::vector<uint32_t>& words) {
    constexpr size_t kMaxWords = 4u * 1024u * 1024u;  // 16 MiB: far above any shader this port generates
    if (words.size() < 5 || words.size() > kMaxWords) return false;
    if (words[0] != 0x07230203u) return false;  // magic
    uint32_t version = words[1];
    if (((version >> 16) & 0xffu) != 1u || ((version >> 8) & 0xffu) > 6u) return false;
    uint32_t bound = words[3];
    if (bound == 0 || bound > 0x400000u) return false;
    if (words[4] != 0) return false;  // schema is reserved and must be 0
    size_t at = 5;
    while (at < words.size()) {
        uint32_t count = words[at] >> 16;
        if (count == 0 || at + count > words.size()) return false;
        at += count;
    }
    return at == words.size();
}

bool ReadWords(const std::filesystem::path& path, std::vector<uint32_t>& words) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) return false;
    std::streamsize bytes = f.tellg();
    if (bytes < 20 || bytes % 4 || bytes > 16 * 1024 * 1024) return false;  // D4: header + sane length, before allocating
    words.resize(size_t(bytes) / 4);
    f.seekg(0);
    f.read(reinterpret_cast<char*>(words.data()), bytes);
    return f.good() && ValidSpirv(words);  // D4
}

void WriteWords(const std::filesystem::path& path, const std::vector<uint32_t>& words) {
    WriteCacheFile(path,{reinterpret_cast<const uint8_t*>(words.data()),words.size()*4});
}

#ifdef XBE_SPIRV_DXC
using DxcCreateInstanceFn = HRESULT (*)(REFCLSID rclsid, REFIID riid, LPVOID* ppv);

// libdxcompiler's handle and entry point, taken once; null when the library is not there
DxcCreateInstanceFn DxcEntry() {
    static DxcCreateInstanceFn fn = [] {
#if defined(_WIN32)
        HMODULE lib = LoadLibraryW(L"dxcompiler.dll");
        return lib ? reinterpret_cast<DxcCreateInstanceFn>(GetProcAddress(lib, "DxcCreateInstance")) : nullptr;
#else
        void* lib = dlopen("libdxcompiler.so", RTLD_NOW | RTLD_LOCAL);
        if (!lib) lib = dlopen("/opt/dxc/lib/libdxcompiler.so", RTLD_NOW | RTLD_LOCAL);
        if (!lib) lib = dlopen("/usr/local/lib/libdxcompiler.so", RTLD_NOW | RTLD_LOCAL);
        return lib ? reinterpret_cast<DxcCreateInstanceFn>(dlsym(lib, "DxcCreateInstance")) : nullptr;
#endif
    }();
    return fn;
}

std::wstring Wide(const char* s) {
    std::wstring w;
    while (*s) w += wchar_t(uint8_t(*s++));
    return w;
}

bool CompileDxc(const std::string& hlsl, const char* entry, bool fragment, std::vector<uint32_t>& words,
                std::string* error) {
    DxcCreateInstanceFn create = DxcEntry();
    if (!create) return false;
    IDxcUtils* utils = nullptr;
    IDxcCompiler3* compiler = nullptr;
    if (FAILED(create(CLSID_DxcUtils, IID_PPV_ARGS(&utils))) ||
        FAILED(create(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler)))) {
        if (utils) utils->Release();
        if (compiler) compiler->Release();
        if (error) *error = "dxc: DxcCreateInstance failed";
        return false;
    }
    std::wstring wentry = Wide(entry);
    std::wstring wprofile = fragment ? L"ps_6_0" : L"vs_6_0";
    // the binding shifts of the header comment: b in place, t past the two cbuffers, s past them and the textures
    const wchar_t* args[] = {L"-E", wentry.c_str(), L"-T", wprofile.c_str(), L"-spirv",
                             L"-fspv-target-env=vulkan1.2", L"-HV", L"2018", L"-O1",
                             L"-fvk-b-shift", L"0", L"0", L"-fvk-t-shift", L"2", L"0",
                             L"-fvk-s-shift", L"6", L"0", L"-fvk-u-shift", L"10", L"0"};
    DxcBuffer source{hlsl.data(), hlsl.size(), DXC_CP_UTF8};
    IDxcResult* result = nullptr;
    HRESULT hr = compiler->Compile(&source, args, uint32_t(std::size(args)), nullptr, IID_PPV_ARGS(&result));
    bool ok = false;
    if (SUCCEEDED(hr) && result) {
        HRESULT status = E_FAIL;
        result->GetStatus(&status);
        IDxcBlobUtf8* messages = nullptr;
        result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&messages), nullptr);
        std::string text = messages && messages->GetStringLength() ? messages->GetStringPointer() : "";
        if (messages) messages->Release();
        if (SUCCEEDED(status)) {
            IDxcBlob* blob = nullptr;
            if (SUCCEEDED(result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&blob), nullptr)) && blob &&
                blob->GetBufferSize() % 4 == 0) {
                words.assign(static_cast<const uint32_t*>(blob->GetBufferPointer()),
                             static_cast<const uint32_t*>(blob->GetBufferPointer()) + blob->GetBufferSize() / 4);
                ok = ValidSpirv(words);  // D4: magic alone is not enough
            }
            if (blob) blob->Release();
        }
        if (!ok && error) *error = "dxc: " + text;
    } else if (error) {
        *error = "dxc: Compile call failed";
    }
    if (result) result->Release();
    compiler->Release();
    utils->Release();
    if (ok) g_compiler = "dxc";
    return ok;
}
#endif  // XBE_SPIRV_DXC

#ifdef XBE_SPIRV_GLSLANG
bool CompileGlslang(const std::string& hlsl, const char* entry, bool fragment, std::vector<uint32_t>& words,
                    std::string* error) {
    static std::once_flag once;
    std::call_once(once, [] { glslang::InitializeProcess(); });
    EShLanguage stage = fragment ? EShLangFragment : EShLangVertex;
    glslang::TShader shader(stage);
    const char* text = hlsl.c_str();
    shader.setStrings(&text, 1);
    shader.setEntryPoint(entry);
    shader.setSourceEntryPoint(entry);
    shader.setEnvInput(glslang::EShSourceHlsl, stage, glslang::EShClientVulkan, 120);
    shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_2);
    shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_5);
    // Keep the Vulkan 1.2 target portable: optional HLSL metadata extensions
    // require a device extension the renderer does not enable.
    shader.setHlslIoMapping(true);
    // HLSL register numbers become bindings, shifted into the layout of the header comment
    shader.setShiftBinding(glslang::EResUbo, 0);
    shader.setShiftBinding(glslang::EResTexture, 2);
    shader.setShiftBinding(glslang::EResSampler, 6);
    shader.setShiftBinding(glslang::EResImage, 10);
    shader.setAutoMapBindings(false);
    shader.setAutoMapLocations(true);
    if (!shader.parse(GetDefaultResources(), 120, false, EShMsgDefault)) {
        if (error) *error = std::string("glslang: ") + shader.getInfoLog();
        return false;
    }
    glslang::TProgram program;
    program.addShader(&shader);
    if (!program.link(EShMsgDefault)) {
        if (error) *error = std::string("glslang: ") + program.getInfoLog();
        return false;
    }
    // glslang's standalone frontend maps I/O after link as well: this applies
    // the b/t/s binding shifts and automatic locations before SPIR-V emission.
    if (!program.mapIO()) {
        if (error) *error = std::string("glslang I/O mapping: ") + program.getInfoLog();
        return false;
    }
    glslang::SpvOptions options;
    options.generateDebugInfo = false;
    options.optimizeSize = false;
    options.disassemble = false;
    options.validate = true;
    glslang::GlslangToSpv(*program.getIntermediate(stage), words, nullptr, &options);
    if (!ValidSpirv(words)) {  // D4
        if (error) *error = "glslang: no valid SPIR-V produced";
        return false;
    }
    g_compiler = "glslang";
    return true;
}
#endif  // XBE_SPIRV_GLSLANG

}  // namespace

void SetSpirvCacheDir(std::filesystem::path dir, std::string device_key) {
    std::lock_guard lock(g_lock);
    // a folder name, nothing else: keep the characters a path is happy with
    for (char& c : device_key)
        if (!(c >= 'a' && c <= 'z' || c >= 'A' && c <= 'Z' || c >= '0' && c <= '9' || c == '.' || c == '-' || c == '_'))
            c = '_';
    g_dir = dir / device_key;
}

void DiscardCachedSpirv(const std::string& hlsl, const char* entry, bool fragment) {
    // D4: drop both the in-memory words and the on-disk entry, so the next attempt
    // (and the next run) recompiles from HLSL instead of reusing a blob that
    // vkCreateShaderModule rejected.
    uint64_t key = CacheKey(hlsl, entry, fragment);
    std::filesystem::path file;
    {
        std::lock_guard lock(g_lock);
        g_cache.erase(key);
        if (!g_dir.empty()) file = CacheFile(key, fragment);
    }
    if (!file.empty()) {
        std::error_code ec;
        std::filesystem::remove(file, ec);
    }
}

bool CompileHlslToSpirv(const std::string& hlsl, const char* entry, bool fragment, std::vector<uint32_t>& words,
                        std::string* error) {
    uint64_t key = CacheKey(hlsl, entry, fragment);
    {
        std::lock_guard lock(g_lock);
        auto it = g_cache.find(key);
        if (it != g_cache.end()) {
            words = it->second;
            return !words.empty();
        }
    }
    std::filesystem::path file;
    {
        std::lock_guard lock(g_lock);
        if (!g_dir.empty()) file = CacheFile(key, fragment);
    }
    if (!file.empty() && ReadWords(file, words)) {
        std::lock_guard lock(g_lock);
        g_cache[key] = words;
        return true;
    }
    if (!file.empty()) {  // D4: an entry that failed validation is discarded, not left for the next run
        std::error_code ec;
        std::filesystem::remove(file, ec);
    }
    std::string dxc_error, glslang_error;
    bool ok = false;
#ifdef XBE_SPIRV_DXC
    ok = CompileDxc(hlsl, entry, fragment, words, &dxc_error);
#endif
#ifdef XBE_SPIRV_GLSLANG
    if (!ok) ok = CompileGlslang(hlsl, entry, fragment, words, &glslang_error);
#endif
    if (!ok) {
        if (error) *error = (dxc_error.empty() ? "" : dxc_error + "\n") + glslang_error;
        // cached as a failure, like d3d12.cpp's Compile: a bad shader is not recompiled every draw
        std::lock_guard lock(g_lock);
        g_cache[key] = {};
        return false;
    }
    {
        std::lock_guard lock(g_lock);
        g_cache[key] = words;
    }
    if (!file.empty()) WriteWords(file, words);
    return true;
}

const char* SpirvCompilerName() {
    std::lock_guard lock(g_lock);
    return g_compiler.c_str();
}

}  // namespace gpu

#endif  // !defined(_WIN32)
