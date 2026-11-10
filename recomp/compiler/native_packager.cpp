// Copyright (C) 2026 OpenAI Codex, for szampan
// SPDX-License-Identifier: GPL-3.0-only
#include "native_packager.h"
#include "executable_file.h"
#include "third_party/ps5_native_tool/elf_object.hpp"
#include "third_party/ps5_native_tool/sce_module_writer.hpp"
#include "third_party/ps5_native_tool/self_container.hpp"
#include "third_party/ps5_native_tool/hash.hpp"
#include "llvm/ADT/SmallVector.h"
#include "llvm/BinaryFormat/ELF.h"
#include "llvm/Object/ArchiveWriter.h"
#include "llvm/Object/ELFObjectFile.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/SHA256.h"
#include <algorithm>
#include <cerrno>
#include <fcntl.h>
#include <set>
#include <stdexcept>
#include <unistd.h>

namespace sims2::compiler {
namespace {
std::string digest(llvm::StringRef bytes) {
  const auto value = llvm::SHA256::hash(llvm::ArrayRef(
      reinterpret_cast<const uint8_t *>(bytes.data()), bytes.size()));
  std::string result;
  for (uint8_t c : value) { result += "0123456789abcdef"[c>>4]; result += "0123456789abcdef"[c&15]; }
  return result;
}
llvm::StringRef view(const std::vector<uint8_t> &bytes) {
  return {reinterpret_cast<const char *>(bytes.data()), bytes.size()};
}
bool relative(const std::string &name) {
  if (name.empty() || name.front() == '/' || name.find('\\') != std::string::npos || name.find(':') != std::string::npos) return false;
  llvm::SmallVector<llvm::StringRef, 8> parts;
  llvm::StringRef(name).split(parts, '/');
  return std::none_of(parts.begin(), parts.end(), [](auto part) { return part.empty() || part == "." || part == ".."; });
}
std::unique_ptr<llvm::MemoryBuffer> read(const std::string &path) {
  auto bytes = llvm::MemoryBuffer::getFile(path);
  if (!bytes) throw std::runtime_error("Cannot read "+path+": "+bytes.getError().message());
  return std::move(*bytes);
}
void write_new(const std::string &path, llvm::StringRef bytes) {
  // Same readable output mode as Clang/LLD, so the configured diagnostic FTP
  // service can collect our artifacts. Existing files are still never replaced.
  int fd = ::open(path.c_str(), O_WRONLY|O_CREAT|O_EXCL, 0644);
  if (fd < 0) throw std::runtime_error("Cannot create new output "+path+": errno="+std::to_string(errno));
  size_t done = 0;
  while (done != bytes.size()) {
    auto wrote = ::write(fd, bytes.data()+done, bytes.size()-done);
    if (wrote < 0 && errno == EINTR) continue;
    if (wrote <= 0) { int error = errno; ::close(fd); throw std::runtime_error("Output write failed: "+path+" errno="+std::to_string(error)); }
    done += size_t(wrote);
  }
  if (::close(fd)) throw std::runtime_error("Output close failed: "+path);
  if (digest(read(path)->getBuffer()) != digest(bytes)) throw std::runtime_error("Output readback differs: "+path);
}
std::string expand(std::string value, const NativePackageRequest &request) {
  for (const auto &item : {std::pair<std::string, std::string>{"{KIT}", request.kit_directory}, {"{WORK}", request.output_directory}}) {
    size_t at = 0;
    while ((at = value.find(item.first, at)) != std::string::npos) {
      value.replace(at, item.first.size(), item.second); at += item.second.size();
    }
  }
  if (value.find('{') != std::string::npos) throw std::runtime_error("Unexpanded native link argument");
  return value;
}
}

int CreateNativeTitle(const NativePackageRequest &request, NativePackageResult &result,
                      Sims2CompilerLog callback, void *user) {
  result = {};
  auto log = [&](const std::string &message) { if (callback) callback(message.data(), message.size(), user); };
  if (!llvm::sys::path::is_absolute(request.kit_directory) || !llvm::sys::path::is_absolute(request.object_directory) ||
      !llvm::sys::path::is_absolute(request.output_directory) || request.objects.size() != 65 || request.stub_files.empty()) return 2;
  try {
    std::set<std::string> expected_objects{"x86_func_mapping.o"};
    for (unsigned i=0; i<64; ++i) expected_objects.insert("x86_recomp."+std::to_string(i)+".o");
    std::set<std::string> found_objects;
    std::vector<llvm::NewArchiveMember> members;
    for (const auto &input : request.public_inputs) {
      if (!relative(input.name) || digest(read(request.kit_directory+"/"+input.name)->getBuffer()) != input.sha256)
        throw std::runtime_error("Public native input differs: "+input.name);
      log("public_input_verified file="+input.name+" sha256="+input.sha256+"\n");
    }
    for (const auto &input : request.objects) {
      if (!expected_objects.count(input.name) || !found_objects.insert(input.name).second)
        throw std::runtime_error("Unexpected or duplicate private object: "+input.name);
      auto bytes = read(request.object_directory+"/"+input.name);
      if (digest(bytes->getBuffer()) != input.sha256) throw std::runtime_error("Private object differs: "+input.name);
      auto parsed = llvm::object::ObjectFile::createObjectFile(bytes->getMemBufferRef());
      if (!parsed) throw std::runtime_error(llvm::toString(parsed.takeError()));
      auto *elf = llvm::dyn_cast<llvm::object::ELF64LEObjectFile>(parsed->get());
      if (!elf || elf->getELFFile().getHeader().e_type != llvm::ELF::ET_REL ||
          elf->getELFFile().getHeader().e_machine != llvm::ELF::EM_X86_64) throw std::runtime_error("Invalid game object: "+input.name);
      llvm::NewArchiveMember member; member.Buf = std::move(bytes); member.MemberName = input.name;
      members.push_back(std::move(member));
      log("private_object_verified file="+input.name+" sha256="+input.sha256+"\n");
    }
    auto archive = llvm::writeArchiveToBuffer(members, llvm::SymtabWritingMode::NormalSymtab,
        llvm::object::Archive::K_GNU, true, false);
    if (!archive) throw std::runtime_error(llvm::toString(archive.takeError()));
    write_new(request.output_directory+"/libsims2-guest.a", (*archive)->getBuffer());
    archive->reset(); members.clear();
    const std::string linked_path = request.output_directory+"/llvm-pie.elf";
    if (::access(linked_path.c_str(), F_OK) == 0) throw std::runtime_error("Native link output already exists");
    std::vector<std::string> arguments;
    for (const auto &value : request.link_arguments) arguments.push_back(expand(value, request));
    std::vector<const char *> pointers;
    for (const auto &value : arguments) pointers.push_back(value.c_str());
    log("native_link_begin\n");
    int code = sims2_compiler_link(pointers.data(), pointers.size(), callback, user);
    log("native_link_end result="+std::to_string(code)+"\n");
    if (code) return code;
    auto raw = read(linked_path);
    auto parsed = llvm::object::ObjectFile::createObjectFile(raw->getMemBufferRef());
    if (!parsed) throw std::runtime_error(llvm::toString(parsed.takeError()));
    std::set<std::string> allowed(request.allowed_imports.begin(), request.allowed_imports.end());
    auto image = ps5::elf::read_image(std::span(reinterpret_cast<const uint8_t *>(raw->getBufferStart()), raw->getBufferSize()), linked_path);
    // The native writer imports .dynsym, not the static/debug symbol table.
    // The first console link left discarded RADV declarations in .symtab (for
    // example radv_EnumeratePhysicalDevices); they are absent from .dynsym.
    // Check exactly the loader-visible imports, keeping the same allowlist.
    for (const auto &symbol : image.dynamic_symbols)
      if (symbol.undefined() && !symbol.name.empty() && !allowed.count(symbol.name))
        throw std::runtime_error("Unverified native import: "+symbol.name);
    raw.reset(); parsed->reset();
    std::vector<ps5::elf::Stub> stubs;
    for (const auto &name : request.stub_files) {
      if (!relative(name)) throw std::runtime_error("Invalid stub path");
      auto data = read(request.kit_directory+"/"+name);
      stubs.push_back(ps5::elf::read_stub(std::span(reinterpret_cast<const uint8_t *>(data->getBufferStart()), data->getBufferSize()), name));
    }
    log("native_conversion_begin\n");
    auto elf = ps5::module::write_executable(image, stubs); // original recipe defaults, unchanged writer
    write_new(request.output_directory+"/eboot.elf", view(elf));
    result.elf_sha256 = digest(view(elf)); result.elf_bytes = elf.size();
    ps5::self::SignOptions options; options.magic = ps5::self::kAlternateMagic;
    auto self = ps5::self::sign(elf, options);
    auto container = ps5::self::parse(self);
    const auto restored = ps5::self::extract(self);
    if (ps5::crypto::sha256(restored) != container.digest) throw std::runtime_error("Native SELF integrity failed");
    write_new(request.output_directory+"/private-eboot.bin", view(self));
    const auto mode=files::EnsureExecutable(request.output_directory+"/private-eboot.bin");
    log("native_executable_mode "+mode.Describe()+"\n");
    result.self_sha256 = digest(view(self)); result.self_bytes = self.size();
    log("native_package_complete sha256="+result.self_sha256+"\n");
    return 0;
  } catch (const std::exception &error) {
    log("native_package_error message="+std::string(error.what())+"\n");
    return 1;
  }
}
}
