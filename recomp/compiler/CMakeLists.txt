# SPDX-License-Identifier: GPL-3.0-only
cmake_minimum_required(VERSION 3.20)
project(sims2compiler CXX)
# Header-only cross checks see the FreeBSD declarations, not the title's heap.
# Limit this capability correction to Process.cpp, preserving all other cached
# LLVM objects and upstream's existing no-counter fallback.
if(CMAKE_CXX_COMPILER_TARGET STREQUAL "x86_64-sie-ps5")
  set_property(SOURCE "${LLVM_MAIN_SRC_DIR}/lib/Support/Process.cpp"
    TARGET_DIRECTORY LLVMSupport APPEND PROPERTY COMPILE_OPTIONS
    -include "${CMAKE_CURRENT_SOURCE_DIR}/ps5-support-config.h")
endif()
add_library(sims2-compiler-interface OBJECT embedded.cpp)
target_compile_features(sims2-compiler-interface PUBLIC cxx_std_17)
target_include_directories(sims2-compiler-interface PRIVATE
  "${LLVM_MAIN_INCLUDE_DIR}" "${LLVM_BINARY_DIR}/include"
  "${LLVM_MAIN_SRC_DIR}/../clang/include" "${LLVM_BINARY_DIR}/tools/clang/include"
  "${LLVM_MAIN_SRC_DIR}/../lld/include")
add_library(sims2-linker-interface OBJECT linker.cpp worker.cpp fatal.cpp)
target_compile_features(sims2-linker-interface PUBLIC cxx_std_17)
target_include_directories(sims2-linker-interface PRIVATE
  "${LLVM_MAIN_INCLUDE_DIR}" "${LLVM_BINARY_DIR}/include"
  "${LLVM_MAIN_SRC_DIR}/../lld/include")
add_dependencies(sims2-linker-interface intrinsics_gen target_parser_gen)
add_library(sims2-console-linker STATIC $<TARGET_OBJECTS:sims2-linker-interface>)
target_link_libraries(sims2-console-linker PRIVATE lldELF)
llvm_map_components_to_libnames(sims2CompilerBackend X86)
add_library(sims2-console-compiler STATIC $<TARGET_OBJECTS:sims2-compiler-interface>)
target_link_libraries(sims2-console-compiler PRIVATE clangFrontendTool clangDriver sims2-console-linker ${sims2CompilerBackend})
add_dependencies(sims2-compiler-interface clang-tablegen-targets intrinsics_gen target_parser_gen)
add_subdirectory(generator)
target_link_libraries(sims2-console-compiler PRIVATE sims2-console-generator)
