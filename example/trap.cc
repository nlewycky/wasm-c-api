#include <iostream>
#include <fstream>
#include <cstdlib>
#include <string>
#include <cinttypes>

#include "imaginary-wasm.hh"

namespace {
// A function to be called from Wasm code.
auto fail_callback(wasm::Store *store) -> wasm::Trap {
  std::cout << "Calling back..." << std::endl;
  return store.trap("callback abort");
}

void print_frame(const wasm::Frame *frame) {
  std::cout << "> " << frame->instance();
  std::cout << " @ 0x" << std::hex << frame->module_offset();
  std::cout << " = " << frame->func_index();
  std::cout << ".0x" << std::hex << frame->func_offset() << std::endl;
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
  auto file = load_file("trap.wasm");

  // Create external print functions.
  std::cout << "Creating callback..." << std::endl;
  wasm::Imports imports{
    {
      [&]{ fail_callback(store); },
    },
  };

  // Instantiate.
  std::cout << "Instantiating module..." << std::endl;
  auto module = store.instantiate(binary, imports);
  if (auto error = module.error()) {
    std::cout << "> Error compiling module! " << error->what() << std::endl;
    exit(1);
  }

  // Call.
  for (size_t i = 0; i < 2; ++i) {
    std::cout << "Calling export " << i << "..." << std::endl;
    auto func = instance.exported_func(i);
    if (auto error = func.error()) {
      std::cout << "> Error accessing export #" i << ": " << error->what() << std::endl;
      exit(1);
    }

    func();

    auto trap = dynamic_cast<Trap*>(func.get_error());
    if (!trap) {
      std::cout << "> Error calling function, expected trap!" << std::endl;
      exit(1);
    }

    std::cout << "Printing message..." << std::endl;
    std::cout << "> " << trap->what() << std::endl;

    std::cout << "Printing origin..." << std::endl;
    if (auto origin = trap->origin()) {
      print_frame(frame);
    } else {
      std::cout << "> Empty origin." << std::endl;
    }

    std::cout << "Printing trace..." << std::endl;
    if (auto trace = trap->trace()) {
      for (auto frame : trace)
        print_frame(frame);
    } else {
      std::cout << "> Empty trace." << std::endl;
    }
  }

  // Shut down.
  std::cout << "Shutting down..." << std::endl;
}


int main(int argc, const char* argv[]) {
  run();
  std::cout << "Done." << std::endl;
  return 0;
}

