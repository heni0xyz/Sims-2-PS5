// Copyright (C) 2026 OpenAI Codex, for szampan
// SPDX-License-Identifier: GPL-3.0-only
#include "generator.h"
#include <filesystem>
#include <mutex>
#include <vector>
#ifdef __PROSPERO__
#include "ps5_filesystem.h"
#endif

// The SDK tool's main is renamed at compile time. Its PS5 file operations use
// the SDK adapter; parser/analysis/code-generation algorithms are unchanged.
int sims2_xbe_recomp_main(int argc, char **argv);
namespace { std::mutex generator_mutex; }

int sims2_generate_source(const char *config, const char *verified_xbe,
                          const char *new_output, const char *xdk,
                          std::string &error) {
  std::lock_guard<std::mutex> lock(generator_mutex);
  error.clear();
  try {
    for (const char *path : {config, verified_xbe, new_output, xdk}) {
      if (!path || !std::filesystem::path(path).is_absolute()) {
        error = "Generator requires absolute paths";
        return 2;
      }
    }
    std::error_code ec;
#ifdef __PROSPERO__
    bool exists = ps5_recomp_fs::exists(new_output, ec);
#else
    bool exists = std::filesystem::exists(new_output, ec);
#endif
    if (ec) {
      error = "Cannot inspect generation output: " + ec.message();
      return 2;
    }
    if (exists) {
      error = "Generation output already exists";
      return 2;
    }
    std::vector<std::string> values = {
        "sims2-xbe-recomp", config, verified_xbe, new_output, "--xdk", xdk};
    std::vector<char *> arguments;
    for (auto &value : values) arguments.push_back(value.data());
    arguments.push_back(nullptr);
    int result = sims2_xbe_recomp_main(static_cast<int>(values.size()), arguments.data());
    if (result) error = "XboxXBERecomp generation failed; see analysis diagnostics";
    return result;
  } catch (const std::exception &exception) {
    error = exception.what();
    return 1;
  } catch (...) {
    error = "Unknown generation failure";
    return 1;
  }
}
