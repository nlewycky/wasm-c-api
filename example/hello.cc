#include <cstddef>
#include <fstream>
#include <functional>
#include <iostream>
#include <utility>
#include <vector>

#include "imaginary-wasm.hh"

namespace {
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
} // namespace

void run() {
  // Initialize.
  std::cout << "Initializing..." << std::endl;
  wasm::Store store;

  // Load binary.
  std::cout << "Loading binary..." << std::endl;
  auto file = load_file("hello.wasm");

  // Here instantiation is performed straight from .wasm bytes. You can compile
  // and instantiate in two steps with:
  //   wasm::Module module = store.compile(file);
  //   if (auto error = module.get_error()) { /* ... */ }
  //   wasm::Instance instance = module.instantiate(wasm::Imports {});

  // Instantiate.
  std::cout << "Instantiating module..." << std::endl;
  wasm::Imports imports{
    {
      "",
      {
        {"hello", [] { std::cout << "> Hello world!" << std::endl; }}
      },
    }
  };

  wasm::Instance instance = store.instantiate(file, imports);
  if (auto error = instance.get_error()) {
    std::cout << "> Error instantiating module! " << error.what() << std::endl;
    exit(1);
  }

  // Extract export.
  std::cout << "Extracting export..." << std::endl;
  auto run_func = instance.exported_func(0);
  if (auto error = run_func.error()) {
    std::cout << "> Error accessing export: " << error.what() << std::endl;
    exit(1);
  }

  // Call.
  std::cout << "Calling export..." << std::endl;
  run_func();
  if (auto error = run_func.get_error()) {
    std::cout << "> Error calling function: " << error.what() << std::endl;
    exit(1);
  }

  // Shut down.
  std::cout << "Shutting down..." << std::endl;
}

int main(int argc, const char *argv[]) {
  run();
  std::cout << "Done." << std::endl;
  return 0;
}
