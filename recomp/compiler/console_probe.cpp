// Copyright (C) 2026 OpenAI Codex, for szampan
// SPDX-License-Identifier: GPL-3.0-only
#include "console_probe.h"
#include "embedded.h"
#include "fixture_manifest.h"
#include "platform.h"
#include "ps5platform/fp.h"
#include "ps5platform/memory.h"
#include "llvm/BinaryFormat/ELF.h"
#include "llvm/Config/llvm-config.h"
#include "llvm/Object/ELFObjectFile.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/SHA256.h"
#include <cerrno>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <exception>
#include <fcntl.h>
#include <mutex>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>
#if defined(SIMS2_PROBE_NATIVE_TITLE)
#include "native_packager.h"
#endif
#if defined(SIMS2_PROBE_EXECUTE)
#include "probe_core_loader.h"
#include "probe_oracle.h"
#include "ps5platform/exec.h"
#endif

#ifndef SIMS2_PROBE_BUILD_ID
#error "The console linker probe requires an explicit build identification"
#endif

#if defined(SIMS2_PROBE_COMPILER)
#define SIMS2_PROBE_TAG "COMPILERPROBE"
static constexpr const char *probe_kind = "console_compiler_synthetic_probe";
#else
#define SIMS2_PROBE_TAG "LINKPROBE"
static constexpr const char *probe_kind = "console_linker_synthetic_probe";
#endif
#if defined(SIMS2_PROBE_GENERATE)
#include "generator.h"
#include "llvm/Support/SHA1.h"
#include <dirent.h>
#endif

namespace {
struct Run {
  Sims2LinkerProbe &state;
  std::mutex lock;
  FILE *file = nullptr;
  std::string output;
  std::string run_id;
  std::string pending;
  std::string input_root = "/app0/linker-fixtures";
  const char *diagnostic_tool = "lld";
  bool io_tested = false;
  bool io_valid = false;
  bool compiler_inputs_valid = false;
  bool execution_tested = false;
  bool execution_valid = false;
  bool generation_valid = false;
  bool game_unit_valid = false;
  bool native_title_valid = false;
  ~Run() { if (file) std::fclose(file); }
  void log(const char *severity, const char *event, const char *format, ...) {
    char detail[1800];
    va_list args;
    va_start(args, format);
    std::vsnprintf(detail, sizeof(detail), format, args);
    va_end(args);
    std::lock_guard<std::mutex> guard(lock);
    char record[2300];
    std::snprintf(record, sizeof(record),
      "[SIMS2][" SIMS2_PROBE_TAG "][%s] t=%.6f build=%s run=%s thread=%llx event=%s %s",
      severity, now_seconds(), SIMS2_PROBE_BUILD_ID, run_id.c_str(),
      (unsigned long long)(uintptr_t)pthread_self(), event, detail);
    say("%s", record);
    if (file) { std::fprintf(file, "%s\n", record); std::fflush(file); }
  }
  void memory(const char *stage) {
    ps5_memory_stats stats{};
    if (!ps5_memory_query(&stats)) {
      log("WARN", "memory", "stage=%s availability=unknown", stage);
      return;
    }
    log("INFO", "memory", "stage=%s total=%llu free=%llu largest=%llu flexible=%llu",
      stage, (unsigned long long)stats.total_bytes, (unsigned long long)stats.free_bytes,
      (unsigned long long)stats.largest_bytes, (unsigned long long)stats.flexible_bytes);
  }
  bool directory(const std::string &path) {
    if (::mkdir(path.c_str(), 0755) == 0) return true;
    // Do not use std::filesystem::status: some mounted launchers deny stat.
    if (errno == EEXIST) return true;
    log("WARN", "mkdir", "path=%s errno=%d message=%s", path.c_str(), errno, std::strerror(errno));
    return false;
  }
  bool open_output() {
#if defined(SIMS2_PROBE_COMPILER)
    const char *roots[] = {"/data/sims2-compiler-probe", "/app0/compiler-runs"};
#else
    const char *roots[] = {"/app0/linker-runs", "/data/sims2-linker-probe"};
#endif
    for (const char *root : roots) {
      log("INFO", "output_attempt", "root=%s", root);
      if (!directory(root)) continue;
      output = std::string(root) + "/" + run_id;
      // A run never reuses an old result or overwrites another title.
      if (::mkdir(output.c_str(), 0755) != 0) {
        log("WARN", "run_directory", "path=%s errno=%d", output.c_str(), errno);
        continue;
      }
      file = std::fopen((output + "/probe.log").c_str(), "wb");
      if (file) { log("INFO", "output_ready", "path=%s", output.c_str()); return true; }
      log("WARN", "output_open", "path=%s errno=%d", output.c_str(), errno);
    }
    log("ERROR", "output_unavailable", "No verified writable output; no link attempted");
    return false;
  }
  static std::string hash(llvm::StringRef bytes) {
    auto digest = llvm::SHA256::hash(llvm::ArrayRef(
      reinterpret_cast<const uint8_t *>(bytes.data()), bytes.size()));
    static const char hex[] = "0123456789abcdef";
    std::string result;
    for (uint8_t b : digest) { result += hex[b >> 4]; result += hex[b & 15]; }
    return result;
  }
  bool input(const char *name, const char *expected) {
    std::string path = std::string("/app0/linker-fixtures/") + name;
    auto bytes = llvm::MemoryBuffer::getFile(path);
    if (!bytes) {
      log("ERROR", "input_open", "path=%s error=%s", path.c_str(), bytes.getError().message().c_str());
      return false;
    }
    std::string digest = hash((*bytes)->getBuffer());
    auto object = llvm::object::ObjectFile::createObjectFile((*bytes)->getMemBufferRef());
    if (!object) {
      log("ERROR", "input_elf", "path=%s error=%s", path.c_str(), llvm::toString(object.takeError()).c_str());
      return false;
    }
    auto *elf = llvm::dyn_cast<llvm::object::ELF64LEObjectFile>(object->get());
    bool valid = elf && elf->getELFFile().getHeader().e_machine == llvm::ELF::EM_X86_64 &&
      elf->getELFFile().getHeader().e_type == llvm::ELF::ET_REL && digest == expected;
    log(valid ? "INFO" : "ERROR", "input_verified", "file=%s bytes=%zu sha256=%s valid=%d",
      name, (*bytes)->getBufferSize(), digest.c_str(), valid);
    return valid;
  }
  // A callback may split a diagnostic anywhere. Preserve all bytes in bounded
  // records, including long lines, with a continuation marker.
  const char *diagnostic_event() const {
    if (std::strcmp(diagnostic_tool, "clang") == 0) return "clang_diagnostic";
    if (std::strcmp(diagnostic_tool, "native") == 0) return "native_diagnostic";
    return "lld_diagnostic";
  }
  void flush_diagnostic() {
    if (!pending.empty()) {
      log("INFO", diagnostic_event(), "continued=0 text=%s", pending.c_str());
      pending.clear();
    }
  }
  static void diagnostic(const char *data, size_t count, void *user) {
    auto &run = *static_cast<Run *>(user);
    for (size_t i = 0; i < count; ++i) {
      char c = data[i];
      if (c == '\n' || run.pending.size() == 768) {
        run.log("INFO", run.diagnostic_event(), "continued=%d text=%s", c != '\n', run.pending.c_str());
        run.pending.clear();
      }
      if (c != '\n') run.pending += c;
    }
  }
  int link(const char *label, bool missing) {
    state.phase.store(2);
    std::string dest = output + "/" + label + ".elf";
    std::vector<std::string> args = {"ld.lld", "-shared", "--no-undefined",
      "--threads=1", "--build-id=sha1", "--hash-style=gnu", "-z", "max-page-size=16384",
      "-soname", "sims2-linker-fixture.elf", "--Map=" + output + "/" + label + ".map",
      "-o", dest, input_root + "/" + (missing ? "missing.o" : "entry.o")};
    if (!missing) args.emplace_back(input_root + "/provider.o");
#if defined(SIMS2_PROBE_EXECUTE)
    // Unchanged linker layout consumed by the unchanged bounded upstream
    // loader: page-aligned segments, FreeBSD OSABI and exported .dynsym.
    args.insert(args.end(), {"-m", "elf_x86_64_fbsd", "-T", "/app0/compiler-resources/ps5-core.ld"});
#endif
    std::vector<const char *> pointers;
    for (size_t i = 0; i < args.size(); ++i) {
      pointers.push_back(args[i].c_str());
      log("INFO", "link_arg", "test=%s index=%zu value=%s", label, i, args[i].c_str());
    }
    memory(label);
    double began = now_seconds();
    diagnostic_tool = "lld";
    log("INFO", "link_begin", "test=%s expected=%s", label, missing ? "unresolved-symbol" : "success");
    int result = sims2_compiler_link(pointers.data(), pointers.size(), diagnostic, this);
    flush_diagnostic();
    log(result == 3 ? "ERROR" : "INFO", "link_end", "test=%s result=%d restart_required=%d elapsed=%.6f",
      label, result, result == 3, now_seconds() - began);
    memory(label);
    return result;
  }
  std::string validate(const char *label) {
    auto bytes = llvm::MemoryBuffer::getFile(output + "/" + label + ".elf");
    if (!bytes) { log("ERROR", "output_read", "error=%s", bytes.getError().message().c_str()); return {}; }
    auto object = llvm::object::ObjectFile::createObjectFile((*bytes)->getMemBufferRef());
    if (!object) { log("ERROR", "output_elf", "error=%s", llvm::toString(object.takeError()).c_str()); return {}; }
    auto *elf = llvm::dyn_cast<llvm::object::ELF64LEObjectFile>(object->get());
    if (!elf || elf->getELFFile().getHeader().e_machine != llvm::ELF::EM_X86_64 ||
        elf->getELFFile().getHeader().e_type != llvm::ELF::ET_DYN) {
      log("ERROR", "output_type", "Expected ELF64LE x86-64 ET_DYN"); return {};
    }
    auto segments = elf->getELFFile().program_headers();
    if (!segments) {
      log("ERROR", "output_segments", "error=%s", llvm::toString(segments.takeError()).c_str());
      return {};
    }
    unsigned loads = 0;
    for (const auto &segment : *segments) {
      if (segment.p_type != llvm::ELF::PT_LOAD) continue;
      ++loads;
      uint64_t alignment = segment.p_align;
      unsigned permissions = segment.p_flags;
      if (alignment < 16384 || segment.p_vaddr % alignment != segment.p_offset % alignment ||
          segment.p_filesz > segment.p_memsz ||
          (permissions & (llvm::ELF::PF_W | llvm::ELF::PF_X)) == (llvm::ELF::PF_W | llvm::ELF::PF_X)) {
        log("ERROR", "output_segments", "load=%u invalid layout or writable executable segment", loads);
        return {};
      }
    }
    if (!loads) { log("ERROR", "output_segments", "No loadable segments"); return {}; }
    unsigned found = 0;
    for (auto symbol : (*object)->symbols()) {
      auto name = symbol.getName();
      auto flags = symbol.getFlags();
      if (!name || !flags) {
        if (!name) llvm::consumeError(name.takeError());
        if (!flags) llvm::consumeError(flags.takeError());
        log("ERROR", "output_symbols", "Invalid symbol table"); return {};
      }
      if (!name->empty() && (*flags & llvm::object::SymbolRef::SF_Undefined)) {
        log("ERROR", "output_import", "symbol=%s", name->str().c_str()); return {};
      }
      if (*name == "sims2_probe_entry") found |= 1;
      if (*name == "sims2_probe_mix") found |= 2;
    }
    if (found != 3) { log("ERROR", "output_symbols", "Required definitions absent mask=%u", found); return {}; }
    std::string digest = hash((*bytes)->getBuffer());
    log("INFO", "output_verified", "test=%s bytes=%zu sha256=%s symbols=2 loads=%u page_bytes=16384 kind=synthetic-shared-elf",
      label, (*bytes)->getBufferSize(), digest.c_str(), loads);
    return digest;
  }
#if defined(SIMS2_PROBE_COMPILER)
#include "console_compile_steps.inc"
#endif
#if defined(SIMS2_PROBE_EXECUTE)
#include "console_execute_steps.inc"
#endif
#if defined(SIMS2_PROBE_GENERATE)
#include "console_generate_steps.inc"
#endif
#if defined(SIMS2_PROBE_GAME_UNIT)
#include "console_game_unit_steps.inc"
#endif
#if defined(SIMS2_PROBE_NATIVE_TITLE)
#include "console_native_title_steps.inc"
#endif
  int execute() {
    ps5_fp_ieee();
    char id[80];
    std::snprintf(id, sizeof(id), "%lld-%llu", (long long)std::time(nullptr),
      (unsigned long long)(now_seconds() * 1000000));
    run_id = id;
    log("INFO", "worker_begin", "llvm=%s linker_address=%p stack_bytes=16777216 game_code_executed=0 executing_linked_code=%d generating_game_source=%d compiling_game_unit=%d game_unit_count=%u preparing_native_title=%d",
      LLVM_VERSION_STRING, reinterpret_cast<const void *>(&sims2_compiler_link),
#if defined(SIMS2_PROBE_EXECUTE)
      1,
#else
      0,
#endif
#if defined(SIMS2_PROBE_GENERATE)
      1,
#else
      0,
#endif
#if defined(SIMS2_PROBE_GAME_UNIT)
      1,
#else
      0,
#endif
      unsigned(SIMS2_PROBE_GAME_UNIT_COUNT),
#if defined(SIMS2_PROBE_NATIVE_TITLE)
      1
#else
      0
#endif
      );
    memory("worker_begin");
    if (!open_output()) return 1;
#if defined(SIMS2_PROBE_COMPILER)
    int compiled = compile_inputs();
    if (compiled) return compiled;
    input_root = output;
#else
    for (const auto &fixture : sims2LinkFixtures)
      if (!input(fixture.name, fixture.sha256)) return 1;
#endif
    int code = link("first", false);
    if (code) return code == 3 ? 3 : 1;
    auto first = validate("first");
    if (first.empty()) return 1;
    code = link("expected-unresolved", true);
    if (code != 1) {
      log("ERROR", "negative_test", "Expected normal unresolved-symbol failure, received=%d", code);
      return code == 3 ? 3 : 1;
    }
    code = link("repeat", false);
    if (code) return code == 3 ? 3 : 1;
    auto repeat = validate("repeat");
    if (repeat.empty() || first != repeat) { log("ERROR", "repeat_mismatch", "Output differs after failed link"); return 1; }
#if defined(SIMS2_PROBE_COMPILER)
    if (!io_valid) return 1;
#endif
#if defined(SIMS2_PROBE_EXECUTE)
    if (!execute_outputs()) return 1;
#endif
#if defined(SIMS2_PROBE_GENERATE)
    if (!generate_game_source()) return 1;
#endif
#if defined(SIMS2_PROBE_GAME_UNIT)
    code = compile_game_units();
    if (code) return code;
#endif
#if defined(SIMS2_PROBE_NATIVE_TITLE)
    code = prepare_native_title();
    if (code) return code;
#endif
    return 0;
  }
  void summary(int code) {
    if (!output.empty() && file) {
      FILE *json = std::fopen((output + "/run-summary.json").c_str(), "wb");
      if (!json) { log("ERROR", "summary_write", "errno=%d", errno); if (!code) code = 1; }
      else {
        int rc = std::fprintf(json,
          "{\"schema\":1,\"kind\":\"%s\",\"build_id\":\"%s\",\"run_id\":\"%s\",\"status\":\"%s\",\"code\":%d,\"compiler_pass\":%s,\"io_tested\":%s,\"io_pass\":%s,\"game_ready\":false,\"linked_code_executed\":%s,\"execution_pass\":%s,\"generation_pass\":%s,\"game_unit_pass\":%s,\"private_title_created\":%s,\"game_code_executed\":false}\n",
          probe_kind, SIMS2_PROBE_BUILD_ID, run_id.c_str(), code ? "FAIL" : "PASS", code,
          compiler_inputs_valid ? "true" : "false", io_tested ? "true" : "false", io_valid ? "true" : "false",
          execution_tested ? "true" : "false", execution_valid ? "true" : "false", generation_valid ? "true" : "false", game_unit_valid ? "true" : "false", native_title_valid ? "true" : "false");
        int closed = std::fclose(json);
        if (rc < 0 || closed) { log("ERROR", "summary_write", "write/close failed errno=%d", errno); code = 1; }
      }
    }
    log(code == 0 ? "INFO" : "ERROR", "verdict", "status=%s code=%d elapsed=%.6f game_ready=0",
      code == 0 ? "PASS" : "FAIL", code, now_seconds() - state.began.load());
    state.verdict.store(code);
    state.phase.store(3);
  }
};

void *worker(void *data) {
  auto &state = *static_cast<Sims2LinkerProbe *>(data);
  Run run{state};
  int code = 1;
  try { code = run.execute(); }
  catch (const std::exception &e) { run.log("ERROR", "worker_exception", "message=%s", e.what()); code = 3; }
  catch (...) { run.log("ERROR", "worker_exception", "message=unknown"); code = 3; }
  run.summary(code);
  return nullptr;
}
}

bool Sims2LinkerProbe::start() {
  if (started) return false;
  phase.store(1);
  began.store(now_seconds());
  pthread_attr_t attributes;
  int rc = pthread_attr_init(&attributes);
  if (!rc) {
    rc = pthread_attr_setstacksize(&attributes, 16 * 1024 * 1024);
    if (!rc) rc = pthread_create(&thread, &attributes, worker, this);
    pthread_attr_destroy(&attributes);
  }
  started = rc == 0;
  if (!started) {
    say("[SIMS2][" SIMS2_PROBE_TAG "][ERROR] event=worker_start result=%d", rc);
    verdict.store(1); phase.store(3);
  }
  return started;
}
void Sims2LinkerProbe::finish() {
  if (!started || phase.load() != 3) return;
  int rc = pthread_join(thread, nullptr);
  say("[SIMS2][" SIMS2_PROBE_TAG "][%s] event=worker_join result=%d", rc ? "ERROR" : "INFO", rc);
  if (rc) verdict.store(1);
  else started = false;
}
