#include <iostream>
#include <fstream>
#include <cstdlib>
#include <string>
#include <cinttypes>

#include "wasm.hh"

namespace {
auto get_export_global(wasm::ownvec<wasm::Extern>& exports, size_t i) -> wasm::Global* {
  if (exports.size() <= i || !exports[i]->global()) {
    std::cout << "> Error accessing global export " << i << "!" << std::endl;
    exit(1);
  }
  return exports[i]->global();
}

auto get_export_func(const wasm::ownvec<wasm::Extern>& exports, size_t i) -> const wasm::Func* {
  if (exports.size() <= i || !exports[i]->func()) {
    std::cout << "> Error accessing function export " << i << "!" << std::endl;
    exit(1);
  }
  return exports[i]->func();
}

template<class T, class U>
void check(T actual, U expected) {
  if (actual != expected) {
    std::cout << "> Error reading value, expected " << expected << ", got " << actual << std::endl;
    exit(1);
  }
}

std::vector<char> load_file(const std::string &filename) {
  std::ifstream file(filename, std::ios::binary | std::ios::ate);
  auto file_size = file.tellg();
  std::vector<char> buffer(file_size);
  file.seekg(0);
  if (file.fail() || !file.read(buffer.data(), file_size)) {
    std::cout << "> Error loading module!" << std::endl;
    exit(1);
  }
  return buffer;
}

void run() {
  // Initialize.
  std::cout << "Initializing..." << std::endl;
  wasm::Store store;

  // Load binary.
  std::cout << "Loading binary..." << std::endl;
  auto file = load_file("global.wasm");

  // Create external globals.
  std::cout << "Creating globals..." << std::endl;
  auto var_f32_import = store.make_global(3.0f);
  auto var_i64_import = store.make_global(uint64_t(4));

  // Instantiate.
  std::cout << "Instantiating module..." << std::endl;
  wasm::Imports imports{
    {
      1.0f,
      2.0,
      var_f32_import,
      var_i64_import,
    },
  };
  auto instance = store.instantiate(file, imports);
  if (instance.error()) {
    std::cout << "> Error instantiating module: " << error->what() << std::endl;
    exit(1);
  }

  // Extract export.
  std::cout << "Extracting exports..." << std::endl;
  auto exports = instance->exports();
  size_t i = 0;
  auto const_f32_export = instance->exported_global(i++);
  auto const_i64_export = instance->exported_global(i++);
  auto var_f32_export = instance->exported_global(i++);
  auto var_i64_export = instance->exported_global(i++);
  i = 0;
  auto get_const_f32_import = instance->exported_func(i++);
  auto get_const_i64_import = instance->exported_func(i++);
  auto get_var_f32_import = instance->exported_func(i++);
  auto get_var_i64_import = instance->exported_func(i++);
  auto get_const_f32_export = instance->exported_func(i++);
  auto get_const_i64_export = instance->exported_func(i++);
  auto get_var_f32_export = instance->exported_func(i++);
  auto get_var_i64_export = instance->exported_func(i++);
  auto set_var_f32_import = instance->exported_func(i++);
  auto set_var_i64_import = instance->exported_func(i++);
  auto set_var_f32_export = instance->exported_func(i++);
  auto set_var_i64_export = instance->exported_func(i++);

  // Try cloning.
  // TODO: what is this really doing? new global? new handle to same global?
  // copy of data presently held by the global?
  auto var_f32_import_copy = var_f32_import;
  assert(var_f32_import_copy == var_f32_import);

  // Interact.
  std::cout << "Accessing globals..." << std::endl;

  // Check initial values.
  check(const_f32_import.f32(), 1);
  check(const_i64_import.i64(), 2);
  check(var_f32_import.f32(), 3);
  check(var_i64_import.i64(), 4);
  check(const_f32_export.f32(), 5);
  check(const_i64_export.i64(), 6);
  check(var_f32_export.f32(), 7);
  check(var_i64_export.i64(), 8);

  check(get_const_f32_import(), 1);
  check(get_const_i64_import(), 2);
  check(get_var_f32_import(), 3);
  check(get_var_i64_import(), 4);
  check(get_const_f32_export(), 5);
  check(get_const_i64_export(), 6);
  check(get_var_f32_export(), 7);
  check(get_var_i64_export(), 8);

  // Modify variables through API and check again.
  var_f32_import = 33.0f;
  var_i64_import = int64_t(34);
  var_f32_export = 37.0f;
  var_i64_export = int64_t(38);

  check(var_f32_import.f32(), 33);
  check(var_i64_import.i64(), 34);
  check(var_f32_export.f32(), 37);
  check(var_i64_export.i64(), 38);

  check(get_var_f32_import(), 33);
  check(get_var_i64_import(), 34);
  check(get_var_f32_export(), 37);
  check(get_var_i64_export(), 38);

  // Modify variables through calls and check again.
  set_var_f32_import(73);
  set_var_i64_import(74);
  set_var_f32_export(77);
  set_var_i64_export(78);

  check(var_f32_import.f32(), 73);
  check(var_i64_import.i64(), 74);
  check(var_f32_export.f32(), 77);
  check(var_i64_export.i64(), 78);

  check(get_var_f32_import(), 73);
  check(get_var_i64_import(), 74);
  check(get_var_f32_export(), 77);
  check(get_var_i64_export(), 78);

  // Shut down.
  std::cout << "Shutting down..." << std::endl;
}
}

int main(int argc, const char* argv[]) {
  run();
  std::cout << "Done." << std::endl;
  return 0;
}

