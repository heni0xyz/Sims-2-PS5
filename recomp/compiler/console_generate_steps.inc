// Copyright (C) 2026 OpenAI Codex, for szampan
// SPDX-License-Identifier: GPL-3.0-only
// Inside Run. Inputs are the owner's verified XBE outside the title package,
// and public config/XDK descriptions. Generated content stays in private /data.
  bool verify_generated_files(const std::string &directory) {
    DIR *entries = ::opendir(directory.c_str());
    if (!entries) { log("ERROR", "generation_list", "errno=%d", errno); return false; }
    size_t count = 0;
    while (auto *item = ::readdir(entries))
      if (std::strcmp(item->d_name, ".") && std::strcmp(item->d_name, "..")) ++count;
    int closed = ::closedir(entries);
    const size_t expected = sizeof(sims2GeneratedFiles)/sizeof(sims2GeneratedFiles[0]);
    if (closed || count != expected) {
      log("ERROR", "generation_inventory", "files=%zu expected=%zu close=%d", count, expected, closed);
      return false;
    }
    for (const auto &item : sims2GeneratedFiles) {
      auto bytes = llvm::MemoryBuffer::getFile(directory + "/" + item.name);
      if (!bytes) {
        log("ERROR", "generation_file_open", "file=%s error=%s", item.name, bytes.getError().message().c_str());
        return false;
      }
      auto digest = hash((*bytes)->getBuffer());
      bool valid = digest == item.sha256;
      log(valid ? "INFO" : "ERROR", "generation_file_verified", "file=%s bytes=%zu sha256=%s valid=%d",
        item.name, (*bytes)->getBufferSize(), digest.c_str(), valid);
      if (!valid) return false;
    }
    return true;
  }
  bool generate_game_source() {
    state.phase.store(7);
    const char *xbe = "/data/sims2-compiler-inputs/default.xbe";
    const char *config = "/app0/compiler-resources/generator/config/game.toml";
    const char *xdk = "/app0/compiler-resources/generator/xdk";
    const std::string generated = output + "/generated";
    log("INFO", "generation_begin", "owner_input=%s private_output=%s", xbe, generated.c_str());
    auto bytes = llvm::MemoryBuffer::getFile(xbe);
    if (!bytes) { log("ERROR", "generation_xbe_open", "error=%s", bytes.getError().message().c_str()); return false; }
    auto digest = llvm::SHA1::hash(llvm::ArrayRef(
      reinterpret_cast<const uint8_t *>((*bytes)->getBufferStart()), (*bytes)->getBufferSize()));
    std::string value;
    const char hex[] = "0123456789abcdef";
    for (uint8_t byte : digest) { value += hex[byte >> 4]; value += hex[byte & 15]; }
    bool valid = value == "16f9e41ce26c315a758f25d43ba1fa9d3809d1ad";
    log(valid ? "INFO" : "ERROR", "generation_xbe_verified", "bytes=%zu sha1=%s valid=%d",
      (*bytes)->getBufferSize(), value.c_str(), valid);
    if (!valid) return false;
    bytes.get().reset();
    std::string error;
    memory("generation_begin");
    double began = now_seconds();
    int code = sims2_generate_source(config, xbe, generated.c_str(), xdk, error);
    log(code ? "ERROR" : "INFO", "generation_end", "result=%d elapsed=%.6f error=%s", code, now_seconds()-began, error.c_str());
    memory("generation_end");
    if (code || !verify_generated_files(generated)) return false;
    code = sims2_generate_source(config, xbe, generated.c_str(), xdk, error);
    bool refused = code == 2 && error == "Generation output already exists";
    log(refused ? "INFO" : "ERROR", "generation_overwrite_refused", "result=%d valid=%d error=%s", code, refused, error.c_str());
    if (!refused || !verify_generated_files(generated)) return false;
    generation_valid = true;
    log("INFO", "generation_verdict", "status=PASS files=%zu overwrite_refused=1 game_code_executed=0",
      sizeof(sims2GeneratedFiles)/sizeof(sims2GeneratedFiles[0]));
    return true;
  }
