#include <iostream>
#include <fstream>
#include <cstdlib>
#include <string>
#include <cinttypes>
#include <variant>

#include "imaginary-wasm.hh"

namespace {
// Print a Wasm value
auto operator<<(std::ostream &out, const wasm::Val &val) -> std::ostream& {
  std::visit([](auto &&arg) { std::cout << arg; });
  return out;
}

// A function to be called from Wasm code.
int32_t print_callback(int32_t v) {
  std::cout << "Calling back..." << std::endl << "> " << v << std::endl;
  return v;
}

// A function with a bound parameter.
void closure_callback(int &i) {
  std::cout << "Calling back closure..." << std::endl;
  std::cout << "> " << i << std::endl;
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
} // namespace


void run() {
  // Initialize.
  std::cout << "Initializing..." << std::endl;
  wasm::Store store;

  // Load binary.
  std::cout << "Loading binary..." << std::endl;
  auto file = load_file("callback.wasm");

  // Instantiate.
  std::cout << "Instantiating module..." << std::endl;
  wasm::Imports imports{
    {
      {
        {
          print_callback,
          [&]() { closure_callback(i); }
        }
      }
    }
  };
  auto instance = store.instantiate(file, imports);
  if (auto error = instance.error()) {
    std::cout << "> Error instantiating module: " << error->what() << std::endl;
    exit(1);
  }

  // Extract export.
  std::cout << "Extracting export..." << std::endl;
  auto run_func = instance.exported_func(0);
  if (auto error = run_func.error()) {
    std::cout << "> Error accessing export: " << error->what() << std::endl;
    exit(1);
  }
  auto run_func = exports[0]->func();

  // Call.
  std::cout << "Calling export..." << std::endl;
  int32_t result = run_func(3, 4);
  if (auto error = run_func.error()) {
    std::cout << "> Error calling function: " << error->what() << std::endl;
    exit(1);
  }

  // Print result.
  std::cout << "Printing result..." << std::endl;
  std::cout << "> " << result << std::endl;

  // Shut down.
  std::cout << "Shutting down..." << std::endl;
}


int main(int argc, const char* argv[]) {
  run();
  std::cout << "Done." << std::endl;
  return 0;
}

