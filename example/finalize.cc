#include <iostream>
#include <fstream>
#include <cstdlib>
#include <string>
#include <cinttypes>

#include "imaginary-wasm.hh"


namespace {
const int iterations = 100000;

int live_count = 0;

void finalize(intptr_t *i) {
  if (*i % (iterations / 10) == 0) {
    std::cout << "Finalizing #" << *i << "..." << std::endl;
  }
  --live_count;
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


void run_in_store(wasm::Store* store) {
  // Load binary.
  std::cout << "Loading binary..." << std::endl;
  auto file = load_file("finalize.wasm");

  // Compile.
  std::cout << "Compiling module..." << std::endl;
  auto module = wasm::Module::make(store, binary);
  if (auto error = module.error()) {
    std::cout << "> Error compiling module: " << error->what() << std::endl;
    exit(1);
  }

  // Instantiate.
  std::cout << "Instantiating modules..." << std::endl;
  for (int i = 0; i <= iterations; ++i) {
    if (i % (iterations / 10) == 0) std::cout << i << std::endl;
    auto instance = store.instantiate(module, wasm::Imports{});
    if (auto error = instance.error()) {
      std::cout << "> Error instantiating module " << i << ": " << error->what() << std::endl;
      exit(1);
    }
    // ?????
    instance->set_host_info(&i, &finalize);
    ++live_count;
  }

  // Shut down.
  std::cout << "Shutting down..." << std::endl;
}


void run() {
  // Initialize.
  std::cout << "Initializing..." << std::endl;
  auto engine = wasm::Engine::make();

  std::cout << "Live count " << live_count << std::endl;
  std::cout << "Creating store 1..." << std::endl;
  wasm::Store store1(engine);

  std::cout << "Running in store 1..." << std::endl;
  run_in_store(store1);
  std::cout << "Live count " << live_count << std::endl;

  {
    std::cout << "Creating store 2..." << std::endl;
    wasm::Store store2(engine);

    std::cout << "Running in store 2..." << std::endl;
    run_in_store(store2);
    std::cout << "Live count " << live_count << std::endl;

    std::cout << "Deleting store 2..." << std::endl;
    std::cout << "Live count " << live_count << std::endl;
  }

  std::cout << "Running in store 1..." << std::endl;
  run_in_store(store1);
  std::cout << "Live count " << live_count << std::endl;

  std::cout << "Deleting store 1..." << std::endl;
}


int main(int argc, const char* argv[]) {
  run();
  std::cout << "Live count " << live_count << std::endl;
  assert(live_count == 0);
  std::cout << "Done." << std::endl;
  return 0;
}

