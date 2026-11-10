// Copyright (C) 2026 OpenAI Codex, for szampan
// SPDX-License-Identifier: GPL-3.0-only
// Inside Run. Reuse the previously verified, privately compiled PS5 objects.
// Outputs stay in this fresh run directory; never replace the running title.
  int prepare_native_title() {
    state.phase.store(9);
    sims2::compiler::NativePackageRequest request;
    request.kit_directory = "/app0/native-title-kit";
    request.object_directory = sims2NativeObjectDirectory;
    request.output_directory = output;
    for (const auto &item : sims2NativePublicInputs) request.public_inputs.push_back({item.name, item.sha256});
    for (const auto &item : sims2NativeObjects) request.objects.push_back({item.name, item.sha256});
    for (auto item : sims2NativeLinkArguments) request.link_arguments.emplace_back(item);
    for (auto item : sims2NativeStubFiles) request.stub_files.emplace_back(item);
    for (auto item : sims2NativeAllowedImports) request.allowed_imports.emplace_back(item);
    sims2::compiler::NativePackageResult result;
    diagnostic_tool = "native";
    memory("native_title_begin");
    double began = now_seconds();
    log("INFO", "native_title_begin", "objects=65 private_source=%s game_code_executed=0", request.object_directory.c_str());
    int code = sims2::compiler::CreateNativeTitle(request, result, diagnostic, this);
    flush_diagnostic();
    log(code ? "ERROR" : "INFO", "native_title_end", "result=%d elapsed=%.6f", code, now_seconds()-began);
    memory("native_title_end");
    if (code) return code;
    log("INFO", "native_artifact", "test=elf bytes=%zu sha256=%s valid=1", result.elf_bytes, result.elf_sha256.c_str());
    log("INFO", "native_artifact", "test=self bytes=%zu sha256=%s valid=1", result.self_bytes, result.self_sha256.c_str());
    native_title_valid = true;
    log("INFO", "native_title_verdict", "status=PASS game_code_linked=1 game_code_executed=0");
    return 0;
  }
