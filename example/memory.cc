#include <iostream>
#include <fstream>
#include <cstdlib>
#include <string>
#include <cinttypes>

#include "imaginary-wasm.hh"

namespace {
template<class T, class U>
void check(T actual, U expected) {
  if (actual != expected) {
    std::cout << "> Error on result, expected " << expected << ", got " << actual << std::endl;
    exit(1);
  }
}

template<class... Args>
void check_ok(const wasm::Func &func, Args... xs) {
  func(xs...);
  if (auto error = func.error()) {
    std::cout << "> Error on result, expected return" << std::endl;
    exit(1);
  }
}

template<class... Args>
void check_trap(const wasm::Func &func, Args... xs) {
  func(xs...);
  if (!func.error()) {
    std::cout << "> Error on result, expected trap" << std::endl;
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
  auto engine = wasm::Engine::make();
  auto store_ = wasm::Store::make(engine.get());
  auto store = store_.get();

  // Load binary.
  std::cout << "Loading binary..." << std::endl;
  auto file = load_file("memory.wasm");

  // Instantiate.
  std::cout << "Instantiating module..." << std::endl;
  auto instance = store.instantiate(file, wasm::Imports{});
  if (auto error = instance.error()) {
    std::cout << "> Error instantiating module: " << error->what() << std::endl;
    exit(1);
  }

  // Extract export.
  std::cout << "Extracting exports..." << std::endl;
  size_t i = 0;
  auto memory = instance->exported_memory(0);
  auto size_func = instance->exported_func<int32_t()>(i++);
  auto load_func = instance->exported_func<int32_t(int32_t)>(i++);
  auto store_func = instance->exported_func<int32_t(int32_t)>(i++);

  // Try cloning.
  // ???
  //assert(memory->copy()->same(memory));

  // Check initial memory.
  std::cout << "Checking memory..." << std::endl;
  check(memory->size(), wasm::Pages(2));
  check(memory->contents()->size(), 0x20000u);
  check(memory->contents()[0], 0);
  check(memory->contents()[0x1000], 1);
  check(memory->contents()[0x1003], 4);

  check(size_func(), 2);
  check(load_func(0), 0);
  check(load_func(0x1000), 1);
  check(load_func(0x1003), 4);
  check(load_func(0x1ffff), 0);
  check_trap(load_func, 0x20000);

  // Mutate memory.
  std::cout << "Mutating memory..." << std::endl;
  memory->contents()[0x1003] = 5;
  check_ok(store_func, 0x1002, 6);
  check_trap(store_func, 0x20000, 0);

  check(memory->contents()[0x1002], 6);
  check(memory->contents()[0x1003], 5);
  check(load_func(0x1002), 6);
  check(load_func(0x1003), 5);

  // Grow memory.
  std::cout << "Growing memory..." << std::endl;
  check(memory->grow(1), true);
  check(memory->size(), 3u);
  check(memory->contents()->size(), 0x30000u);

  check(load_func(0x20000), 0);
  check_ok(store_func, 0x20000, 0);
  check_trap(load_func, 0x30000);
  check_trap(store_func, 0x30000, 0);

  check(memory->grow(1), false);
  check(memory->grow(0), true);

  // Create stand-alone memory.
  // TODO(wasm+): Once Wasm allows multiple memories, turn this into import.
  std::cout << "Creating stand-alone memory..." << std::endl;
  auto memory2 = store.make_memory(wasm::MemoryType{5, 5});
  check(memory2->size(), 5u);
  check(memory2->grow(1), false);
  check(memory2->grow(0), true);

  // Shut down.
  std::cout << "Shutting down..." << std::endl;
}
}

int main(int argc, const char* argv[]) {
  run();
  std::cout << "Done." << std::endl;
  return 0;
}

