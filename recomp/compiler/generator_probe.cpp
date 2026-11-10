// Copyright (C) 2026 OpenAI Codex, for szampan
// SPDX-License-Identifier: GPL-3.0-only
#include "generator.h"
#include <cstdio>
int main(int argc, char **argv) {
  if (argc != 5) {
    std::fprintf(stderr, "usage: generator-probe absolute-config absolute-XBE new-absolute-output absolute-XDK\n");
    return 2;
  }
  std::string error;
  int result = sims2_generate_source(argv[1],argv[2],argv[3],argv[4],error);
  if (result) std::fprintf(stderr,"GENERATOR_FAIL %s\n",error.c_str());
  else std::fprintf(stderr,"GENERATOR_PASS\n");
  return result;
}
