# SPDX-License-Identifier: GPL-3.0-only
cmake_minimum_required(VERSION 3.20)
project(sims2_compiler_fatal_probe CXX)
find_package(LLVM REQUIRED CONFIG)
add_executable(sims2-compiler-fatal-test ../fatal_probe.cpp ../fatal.cpp)
target_compile_features(sims2-compiler-fatal-test PRIVATE cxx_std_17)
target_include_directories(sims2-compiler-fatal-test PRIVATE ${LLVM_INCLUDE_DIRS})
target_link_libraries(sims2-compiler-fatal-test PRIVATE LLVMSupport)
