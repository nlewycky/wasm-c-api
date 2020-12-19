#pragma once

#include <functional>
#include <initializer_list>
#include <string>
#include <vector>

namespace wasm {
struct Frame {
  // TODO: what are the pointer invalidation rules here?
  const Instance *instance() const;

  size_t module_offset() const;
  uint32_t func_index() const;
  size_t func_offset() const;
};

struct Error : std::runtime_error {};

// An error that occurs with a wasm function in the call stack.
struct Trap : Error {
  typedef iterator Frame*;
  typedef const_iterator const Frame*;
  Frame origin();
  iterator begin();
  iterator end();
  const_iterator begin() const;
  const_iterator end() const;
};
struct EngineConfig {
  EngineConfig();
  // Implementation provides extra c'tors here.
};
struct Module;
struct Instance;
struct Imports;
struct Global;
struct Store {
  explicit Store();
  explicit Store(const EngineConfig &);

  Module compile(const std::vector<char> &);
  Instance instantiate(const std::vector<char> &);

  Trap trap(const std::string &);  // TODO: maybe return pointer?

  Memory make_memory(const MemoryType &);

  Global make_const(int32_t);
  Global make_const(int64_t);
  Global make_const(float);
  Global make_const(double);
  Global make_global(int32_t);
  Global make_global(int64_t);
  Global make_global(float);
  Global make_global(double);
};
struct Module {
  explicit Module(Store &, const std::vector<char> &);
  Error *error() const;
  Instance instantiate(const Imports &) const;
};
struct Imports {
  struct Namespace {
    struct Object {
      // c'tors for each type of import object
      Object(const std::string &, std::function<void()>);
      Object(uint32_t, std::function<void()>);
      Object(std::function<void()>);
    };
    Namespace(std::initializer_list<std::pair<const std::string &, Object>>);
    Namespace(std::initializer_list<std::pair<uint32_t, Object>>);
    Namespace(std::initializer_list<Object>);
  };
  Imports(std::initializer_list<std::pair<const std::string &, Namespace>>);
  Imports(std::initializer_list<std::pair<uint32_t, Namespace>>);
  Imports(std::initializer_list<Namespace>);
};
struct Export {
  bool is_func() const;
};
struct Func {
  std::function<Trap(void)> func() const;
  Error *error() const;
  void operator()();
};
struct Global {
  Error *error() const;
  bool is_mutable() const;

  // kind()
  // type()

  float f32() const;
  double f64() const;
  int32_t i32() const;
  int64_t i64() const;

  Global &operator=(float);
  Global &operator=(double);
  Global &operator=(int32_t);
  Global &operator=(int64_t);
};
struct Instance {
  Error *error() const;

  size_t exports_size() const;
  bool exports_empty() const;

  // const_iterator exported_func_begin() const;
  // const_iterator exported_func_end() const;
  // std::pair<const_iterator, const_iterator> exported_funcs();
  Func exported_func(int) const;
  Func exported_func(const std::string &) const;
  template<typename Fn>
  Func exported_func(int) const;
  template<typename Fn>
  Func exported_func(const std::string &) const;
  size_t exported_func_size() const;
  bool exported_func_empty() const;
};

struct Limits {
  Pages min;
  Pages max;
};

struct MemoryType {
  Limits limits;
};

struct Memory {
  Pages size();
  bool grow(Pages);
  std::span<char> contents();
};

struct RefVal {
  void *ptr = nullptr;
};
struct Val {
  std::variant<int32_t, int64_t, float, double, RefVal> value;
};

// TODO: should we have a way to manually marshall types?
// it seems like it would only be useful if it could handle multiple arguments
// at a time?
} // namespace wasm
