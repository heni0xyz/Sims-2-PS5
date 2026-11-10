// Copyright (C) 2026 OpenAI Codex, for szampan
// SPDX-License-Identifier: GPL-3.0-only
// Included inside Run only in the compiler diagnostic. Reuses its logging,
// memory snapshots, worker, LLD entry and output validation.
  bool resource(const char *relative, const char *expected) {
    const std::string path = std::string("/app0/compiler-resources/") + relative;
    log("INFO", "resource_begin", "file=%s", relative);
    auto bytes = llvm::MemoryBuffer::getFile(path);
    if (!bytes) {
      log("ERROR", "resource_open", "file=%s error=%s", relative, bytes.getError().message().c_str());
      return false;
    }
    const auto digest = hash((*bytes)->getBuffer());
    const bool valid = digest == expected;
    log(valid ? "INFO" : "ERROR", "resource_verified", "file=%s bytes=%zu sha256=%s valid=%d",
      relative, (*bytes)->getBufferSize(), digest.c_str(), valid);
    return valid;
  }
  bool object(const std::string &path, std::string *digest = nullptr) {
    auto bytes = llvm::MemoryBuffer::getFile(path);
    if (!bytes) {
      log("ERROR", "compiled_object_open", "path=%s error=%s", path.c_str(), bytes.getError().message().c_str());
      return false;
    }
    auto parsed = llvm::object::ObjectFile::createObjectFile((*bytes)->getMemBufferRef());
    if (!parsed) {
      log("ERROR", "compiled_object_invalid", "error=%s", llvm::toString(parsed.takeError()).c_str());
      return false;
    }
    auto *elf = llvm::dyn_cast<llvm::object::ELF64LEObjectFile>(parsed->get());
    bool valid = elf && elf->getELFFile().getHeader().e_machine == llvm::ELF::EM_X86_64 &&
      elf->getELFFile().getHeader().e_type == llvm::ELF::ET_REL;
    auto value = hash((*bytes)->getBuffer());
    log(valid ? "INFO" : "ERROR", "compiled_object_verified", "path=%s bytes=%zu sha256=%s valid=%d",
      path.c_str(), (*bytes)->getBufferSize(), value.c_str(), valid);
    if (valid && digest) *digest = value;
    return valid;
  }
  bool io_probe() {
    state.phase.store(5);
    log("INFO", "io_begin", "write_read_rename_unlink_and_truncate=1");
    bool good = true;
    std::string original = output + "/io-original.bin";
    std::string renamed = output + "/io-renamed.bin";
    std::vector<char> payload(131089);
    for (size_t i = 0; i < payload.size(); ++i) payload[i] = char((i * 37 + 11) & 255);
    log("INFO", "io_operation_begin", "operation=fopen path=%s", original.c_str());
    FILE *out = std::fopen(original.c_str(), "wb");
    if (!out) { log("ERROR", "io_open", "errno=%d", errno); return false; }
    log("INFO", "io_operation_begin", "operation=fwrite bytes=%zu", payload.size());
    const size_t written = std::fwrite(payload.data(), 1, payload.size(), out);
    log("INFO", "io_operation_begin", "operation=fflush");
    int flushed = std::fflush(out);
    log("INFO", "io_operation_begin", "operation=fclose");
    int closed = std::fclose(out);
    if (written != payload.size() || flushed || closed) {
      log("ERROR", "io_write", "bytes=%zu expected=%zu flush=%d close=%d errno=%d",
        written, payload.size(), flushed, closed, errno);
      good = false;
    }
    log("INFO", "io_operation_begin", "operation=readback path=%s", original.c_str());
    auto bytes = llvm::MemoryBuffer::getFile(original);
    bool read_ok = bytes && (*bytes)->getBufferSize() == payload.size() &&
      std::memcmp((*bytes)->getBufferStart(), payload.data(), payload.size()) == 0;
    log(read_ok ? "INFO" : "ERROR", "io_readback", "bytes=%zu valid=%d", payload.size(), read_ok);
    if (!read_ok) good = false;
    // Release any mapping before rename/delete; do not confuse an open mapping
    // lifetime with the launcher's filesystem behavior.
    if (bytes) bytes.get().reset();
    log("INFO", "io_operation_begin", "operation=rename");
    int result = ::rename(original.c_str(), renamed.c_str());
    int operation_error = result ? errno : 0;
    log(result ? "ERROR" : "INFO", "io_rename", "result=%d errno=%d", result, operation_error);
    if (result) good = false;
    const std::string &present = result ? original : renamed;
    log("INFO", "io_operation_begin", "operation=unlink path=%s", present.c_str());
    result = ::unlink(present.c_str());
    operation_error = result ? errno : 0;
    log(result ? "ERROR" : "INFO", "io_unlink", "result=%d errno=%d", result, operation_error);
    if (result) good = false;
    const std::string truncated = output + "/io-truncate.bin";
    log("INFO", "io_operation_begin", "operation=open_truncate_fixture path=%s", truncated.c_str());
    int fd = ::open(truncated.c_str(), O_RDWR | O_CREAT | O_EXCL, 0600);
    if (fd < 0) { log("ERROR", "io_truncate_open", "errno=%d", errno); good = false; }
    else {
      log("INFO", "io_operation_begin", "operation=ftruncate bytes=65553");
      result = ::ftruncate(fd, 65553);
      operation_error = result ? errno : 0;
      log(result ? "ERROR" : "INFO", "io_ftruncate", "size=65553 result=%d errno=%d", result, operation_error);
      if (result) good = false;
      log("INFO", "io_operation_begin", "operation=close");
      if (::close(fd)) { log("ERROR", "io_close", "errno=%d", errno); good = false; }
      log("INFO", "io_operation_begin", "operation=read_truncate_fixture");
      auto resized = llvm::MemoryBuffer::getFile(truncated);
      bool size_ok = resized && (*resized)->getBufferSize() == 65553;
      log(size_ok ? "INFO" : "ERROR", "io_truncate_readback", "valid=%d", size_ok);
      if (!size_ok) good = false;
      if (resized) resized.get().reset();
      log("INFO", "io_operation_begin", "operation=unlink_truncate_fixture");
      result = ::unlink(truncated.c_str());
      log(result ? "ERROR" : "INFO", "io_truncate_unlink", "result=%d errno=%d", result, result ? errno : 0);
      if (result) good = false;
    }
    log(good ? "INFO" : "ERROR", "io_verdict", "status=%s", good ? "PASS" : "FAIL");
    return good;
  }
  int compile(const char *label, const char *define) {
    state.phase.store(4);
    std::vector<std::string> args = {"/app0/embedded-clang", "--target=x86_64-sie-ps5",
      "-isysroot", "/app0/compiler-resources/sdk",
      "-resource-dir", "/app0/compiler-resources/clang-resource",
      "-stdlib++-isystem", "/app0/compiler-resources/sdk/target/include/c++/v1",
      // The PS5 driver adds target/include after libc++ automatically. An
      // explicit user -isystem C directory moves it before libc++ wrappers,
      // breaking their include_next chain (cstddef/cstdint/cstdlib).
      "-std=c++20", "-ffreestanding", "-fPIC", "-fno-plt", "-fno-stack-protector",
      "-femulated-tls", "-fdenormal-fp-math=ieee", "-march=x86-64-v2",
      "-fno-exceptions", "-fno-rtti", "-O2", "-g0", "-c",
      "/app0/compiler-resources/compile_fixture.cpp", "-o", output + "/" + label + ".o"};
#if defined(SIMS2_PROBE_EXECUTE)
    // The loader resolves exported functions from .dynsym. PS5's default
    // hidden visibility is suitable for a title, not this fixture module.
    args.emplace_back("-fvisibility=default");
    args.emplace_back("-fvisibility-nodllstorageclass=default");
#endif
    if (define) args.emplace_back(std::string("-D") + define);
    std::vector<const char *> pointers;
    for (size_t i = 0; i < args.size(); ++i) {
      pointers.push_back(args[i].c_str());
      log("INFO", "compile_arg", "test=%s index=%zu value=%s", label, i, args[i].c_str());
    }
    memory(label);
    double began = now_seconds();
    diagnostic_tool = "clang";
    log("INFO", "compile_begin", "test=%s", label);
    int code = sims2_compiler_compile_driver(output.c_str(), pointers.data(), pointers.size(), diagnostic, this);
    flush_diagnostic();
    log(code == 3 ? "ERROR" : "INFO", "compile_end", "test=%s result=%d restart_required=%d elapsed=%.6f",
      label, code, code == 3, now_seconds() - began);
    memory(label);
    return code;
  }
  int compile_inputs() {
    for (const auto &item : sims2CompilerResources)
      if (!resource(item.name, item.sha256)) return 1;
    io_tested = true;
    io_valid = io_probe();
    // Retain compiler evidence even when the independent I/O checks fail;
    // the overall verdict still fails and does not hide those errors.
    int code = compile("provider", "SIMS2_FIXTURE_PROVIDER");
    if (code) return code == 3 ? 3 : 1;
    if (!object(output + "/provider.o")) return 1;
    code = compile("entry", nullptr);
    if (code) return code == 3 ? 3 : 1;
    std::string first;
    if (!object(output + "/entry.o", &first)) return 1;
    code = compile("expected-syntax-error", "SIMS2_FIXTURE_SYNTAX_ERROR");
    if (code != 1) { log("ERROR", "compile_negative", "expected=1 received=%d", code); return code == 3 ? 3 : 1; }
    code = compile("entry-repeat", nullptr);
    if (code) return code == 3 ? 3 : 1;
    std::string repeated;
    if (!object(output + "/entry-repeat.o", &repeated) || first != repeated) {
      log("ERROR", "compile_repeat_mismatch", "Repeated object differs"); return 1;
    }
    code = compile("missing", "SIMS2_FIXTURE_MISSING");
    if (code) return code == 3 ? 3 : 1;
    if (!object(output + "/missing.o")) return 1;
    compiler_inputs_valid = true;
    log("INFO", "compiler_verdict", "status=PASS repeated_object_identical=1");
    return 0;
  }
