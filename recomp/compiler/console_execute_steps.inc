// Copyright (C) 2026 OpenAI Codex, for szampan
// SPDX-License-Identifier: GPL-3.0-only
// Inside Run. Only the just-compiled, validated no-import authored fixture is
// handed to the unchanged pinned PS5_RetroArch loader, never arbitrary modules.
  bool execute_outputs() {
    state.phase.store(6);
    log("INFO", "execution_begin", "loader=PS5_RetroArch fixture_only=1 imports=0 game_code=0");
    uint64_t regions_before = 0, bytes_before = 0;
    ps5_exec_live(&regions_before, &bytes_before);
    // Reject an object file as a module, then prove a valid load still works.
    void *bad = ps5_core_dlopen((output + "/missing.o").c_str(), 0);
    if (bad) {
      ps5_core_dlclose(bad);
      log("ERROR", "execution_negative", "valid=0 reason=object_accepted_as_module");
      return false;
    }
    const char *error = ps5_core_dlerror();
    log("INFO", "execution_negative", "valid=1 error=%s", error ? error : "<none>");
    size_t calls = 0;
    for (const char *label : {"first", "repeat"}) {
      log("INFO", "module_load_begin", "test=%s", label);
      void *module = ps5_core_dlopen((output + "/" + label + ".elf").c_str(), 0);
      if (!module) {
        error = ps5_core_dlerror();
        log("ERROR", "module_load", "test=%s error=%s", label, error ? error : "<none>");
        return false;
      }
      struct CloseModule {
        void *value;
        ~CloseModule() { if (value) ps5_core_dlclose(value); }
      } guard{module};
      void *entry = ps5_core_dlsym(module, "sims2_probe_entry");
      if (!entry) {
        error = ps5_core_dlerror();
        log("ERROR", "module_symbol", "test=%s error=%s", label, error ? error : "<none>");
        return false;
      }
      auto function = reinterpret_cast<uint32_t (*)(uint32_t)>(entry);
      log("INFO", "module_entry_ready", "test=%s native=%p", label, entry);
      for (const auto &test : sims2ProbeCases) {
        log("INFO", "execution_call", "test=%s input=%08x native=%p", label, test.input, entry);
        const uint32_t answer = function(test.input);
        execution_tested = true;
        ++calls;
        bool valid = answer == test.expected;
        log(valid ? "INFO" : "ERROR", "execution_result", "test=%s input=%08x received=%08x expected=%08x valid=%d",
          label, test.input, answer, test.expected, valid);
        if (!valid) return false;
      }
      int closed = ps5_core_dlclose(module);
      guard.value = nullptr;
      log(closed ? "ERROR" : "INFO", "module_unload", "test=%s result=%d", label, closed);
      if (closed) return false;
      log("INFO", "execution_end", "test=%s valid=1 calls=%zu", label, sizeof(sims2ProbeCases)/sizeof(sims2ProbeCases[0]));
    }
    uint64_t regions_after = 0, bytes_after = 0;
    ps5_exec_live(&regions_after, &bytes_after);
    execution_valid = regions_before == regions_after && bytes_before == bytes_after;
    log(execution_valid ? "INFO" : "ERROR", "execution_verdict",
      "status=%s calls=%zu regions_before=%llu regions_after=%llu bytes_before=%llu bytes_after=%llu",
      execution_valid ? "PASS" : "FAIL", calls, (unsigned long long)regions_before,
      (unsigned long long)regions_after, (unsigned long long)bytes_before, (unsigned long long)bytes_after);
    return execution_valid;
  }
