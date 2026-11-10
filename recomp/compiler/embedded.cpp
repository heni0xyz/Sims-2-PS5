// Copyright (C) 2026 OpenAI Codex, for szampan
// SPDX-License-Identifier: GPL-3.0-only
#include "embedded.h"
#include "fatal.h"
#include "worker.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/TargetOptions.h"
#include "clang/Driver/Compilation.h"
#include "clang/Driver/Driver.h"
#include "clang/Driver/Job.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/CompilerInvocation.h"
#include "clang/Frontend/TextDiagnosticBuffer.h"
#include "clang/Frontend/TextDiagnosticPrinter.h"
#include "clang/FrontendTool/Utils.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/VirtualFileSystem.h"
#include <cstring>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#if defined(__PROSPERO__) && (!defined(__EXCEPTIONS) || !defined(__GXX_RTTI))
#error "The embedded PS5 compiler requires explicit -fexceptions -frtti"
#endif

using namespace sims2::compiler;
namespace {
std::once_flag targets_once;
} // namespace

int sims2_compiler_compile(const char *working_directory,
                           const char *const *arguments, std::size_t count,
                           Sims2CompilerLog callback, void *user) {
  if (!arguments || !count || !working_directory ||
      !llvm::sys::path::is_absolute(working_directory))
    return 2;
  std::lock_guard<std::mutex> lock(compiler_mutex);
  if (!compiler_usable)
    return 3;
  Sims2CompilerFatalState failure;
  try {
    Sims2CompilerFatalScope fatal_scope(failure);
    std::call_once(targets_once, [] {
      llvm::InitializeAllTargets();
      llvm::InitializeAllTargetMCs();
      llvm::InitializeAllAsmPrinters();
      llvm::InitializeAllAsmParsers();
    });
    LogStream output(callback, user);
    output << "Frontend: creating physical VFS (SDK getcwd/realpath required)\n";
    llvm::IntrusiveRefCntPtr<llvm::vfs::FileSystem> files(
        llvm::vfs::createPhysicalFileSystem().release());
    if (auto error = files->setCurrentWorkingDirectory(working_directory)) {
      output << "compiler working directory: " << error.message() << '\n';
      return 2;
    }
    output << "Frontend: working directory ready\n";
    auto ids = clang::DiagnosticIDs::create();
    clang::DiagnosticOptions options;
    auto *buffer = new clang::TextDiagnosticBuffer;
    clang::DiagnosticsEngine diagnostics(ids, options, buffer);
    auto invocation = std::make_shared<clang::CompilerInvocation>();
    bool valid = clang::CompilerInvocation::CreateFromArgs(
        *invocation, llvm::ArrayRef(arguments, count), diagnostics,
        "sims2-compiler");
    clang::CompilerInstance compiler(std::move(invocation));
    compiler.createVirtualFileSystem(std::move(files), buffer);
    compiler.createDiagnostics(
        new clang::TextDiagnosticPrinter(output, compiler.getDiagnosticOpts()));
    buffer->FlushDiagnostics(compiler.getDiagnostics());
    if (!valid)
      return 1;
    // DisableFree intentionally retains compiler memory in the standalone
    // driver; repeated installer jobs must release it. Refuse rather than leak
    // per unit.
    if (compiler.getFrontendOpts().DisableFree ||
        compiler.getFrontendOpts().ProgramAction != clang::frontend::EmitObj ||
        compiler.getTargetOpts().Triple != "x86_64-sie-ps5" ||
        !llvm::sys::path::is_absolute(compiler.getFrontendOpts().OutputFile)) {
      output
          << "Expected an object-only PS5 compilation with absolute output\n";
      return 2;
    }
    return clang::ExecuteCompilerInvocation(&compiler) ? 0 : 1;
  } catch (const Sims2CompilerFatal &) {
    return poison_compiler(failure.message, callback, user);
  } catch (const std::exception &error) {
    return poison_compiler(error.what(), callback, user);
  } catch (...) {
    return poison_compiler("Unknown compiler exception", callback, user);
  }
}

int sims2_compiler_compile_driver(const char *working_directory,
                                  const char *const *arguments,
                                  std::size_t count, Sims2CompilerLog callback,
                                  void *user) {
  if (!arguments || !count || !working_directory ||
      !llvm::sys::path::is_absolute(working_directory))
    return 2;
  std::vector<std::string> frontend;
  {
    std::lock_guard<std::mutex> lock(compiler_mutex);
    if (!compiler_usable)
      return 3;
    Sims2CompilerFatalState failure;
    try {
      Sims2CompilerFatalScope fatal_scope(failure);
      LogStream output(callback, user);
      output << "Driver: creating physical VFS (SDK getcwd/realpath required)\n";
      llvm::IntrusiveRefCntPtr<llvm::vfs::FileSystem> files(
          llvm::vfs::createPhysicalFileSystem().release());
      if (auto error = files->setCurrentWorkingDirectory(working_directory)) {
        output << "driver working directory: " << error.message() << '\n';
        return 2;
      }
      output << "Driver: working directory ready\n";
      clang::DiagnosticOptions options;
      clang::DiagnosticsEngine diagnostics(
          clang::DiagnosticIDs::create(), options,
          new clang::TextDiagnosticPrinter(output, options));
      clang::driver::Driver driver(arguments[0], "x86_64-sie-ps5", diagnostics,
                                   "", files);
      std::unique_ptr<clang::driver::Compilation> compilation(
          driver.BuildCompilation(llvm::ArrayRef(arguments, count)));
      if (!compilation || diagnostics.hasErrorOccurred())
        return 1;
      const auto &jobs = compilation->getJobs();
      if (jobs.size() != 1) {
        output << "Expected one in-process compiler job\n";
        return 2;
      }
      const auto &args = jobs.begin()->getArguments();
      if (args.empty() || llvm::StringRef(args.front()) != "-cc1") {
        output << "External compiler/assembler/linker execution refused\n";
        return 2;
      }
      for (const char *argument : llvm::ArrayRef(args).drop_front()) {
        // Clang's standalone driver adds this process-lifetime optimization
        // to ordinary jobs. The installer owns repeated in-process jobs and
        // must release their AST/backend allocations after each unit.
        if (llvm::StringRef(argument) == "-disable-free") {
          output << "In-process compiler: retaining per-unit memory cleanup\n";
          continue;
        }
        frontend.emplace_back(argument);
      }
    } catch (const Sims2CompilerFatal &) {
      return poison_compiler(failure.message, callback, user);
    } catch (const std::exception &error) {
      return poison_compiler(error.what(), callback, user);
    } catch (...) {
      return poison_compiler("Unknown driver exception", callback, user);
    }
  }
  std::vector<const char *> args;
  for (const auto &argument : frontend)
    args.push_back(argument.c_str());
  return sims2_compiler_compile(working_directory, args.data(), args.size(),
                                callback, user);
}
