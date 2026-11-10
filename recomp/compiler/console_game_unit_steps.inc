// Copyright (C) 2026 OpenAI Codex, for szampan
// SPDX-License-Identifier: GPL-3.0-only
// Inside Run. All source just passed generation parity. Compile sequentially
// with frontend cleanup between jobs. Never link or execute game code here.
  int compile_game_units() {
    state.phase.store(8);
    const std::string generated = output + "/generated";
    const size_t count = sizeof(sims2GameCompileSources)/sizeof(sims2GameCompileSources[0]);
    for (const char *name : sims2GameCompileSources) {
    const std::string source = generated + "/" + name;
    std::string object_name = name;
    object_name.replace(object_name.size()-4, 4, ".o");
    const std::string object_path = output + "/" + (count == 1 ? "real-game-unit.o" : object_name);
    std::vector<std::string> arguments;
    for (const char *planned : sims2GameCompileArguments) {
      std::string value = planned;
      for (const auto &replacement : std::vector<std::pair<std::string, std::string>>{
              {"{GENERATED}", generated}, {"{SOURCE}", source}, {"{OBJECT}", object_path}}) {
        size_t at = 0;
        while ((at = value.find(replacement.first, at)) != std::string::npos) {
          value.replace(at, replacement.first.size(), replacement.second);
          at += replacement.second.size();
        }
      }
      arguments.push_back(std::move(value));
    }
    std::vector<const char *> pointers;
    for (size_t i = 0; i < arguments.size(); ++i) {
      pointers.push_back(arguments[i].c_str());
      log("INFO", "game_unit_arg", "test=%s index=%zu value=%s", name, i, arguments[i].c_str());
    }
    memory("game_unit_begin");
    double began = now_seconds();
    state.operation_began.store(began);
    diagnostic_tool = "clang";
    log("INFO", "game_unit_begin", "test=%s source=%s optimization=O2 game_code_executed=0", name, source.c_str());
    int code = sims2_compiler_compile_driver(generated.c_str(), pointers.data(), pointers.size(), diagnostic, this);
    flush_diagnostic();
    log(code ? "ERROR" : "INFO", "game_unit_end", "test=%s result=%d elapsed=%.6f restart_required=%d", name, code, now_seconds()-began, code == 3);
    memory("game_unit_end");
    if (code) return code == 3 ? 3 : 1;
    std::string digest;
    if (!object(object_path, &digest)) return 1;
    log("INFO", "game_unit_object", "test=%s path=%s sha256=%s valid=1", name, object_path.c_str(), digest.c_str());
    state.units_done.fetch_add(1);
    log("INFO", "game_unit_progress", "completed=%u units=%zu", state.units_done.load(), count);
    }
    game_unit_valid = true;
    log("INFO", "game_unit_verdict", "status=PASS units=%zu game_code_linked=0 game_code_executed=0", count);
    return 0;
  }
