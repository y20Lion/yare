#pragma once

#include <memory>

#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
  TypeName(const TypeName&) = delete;   \
  void operator=(const TypeName&) = delete; 

#define DEFAULT_COPY_AND_ASSIGN(TypeName) \
   TypeName(const TypeName& other) = default; \
   TypeName& operator=(const TypeName&) = default;

template <typename T>
using Sptr = std::shared_ptr<T>;

template <typename T>
using Uptr = std::unique_ptr<T>;

#define VK_CHECK(x) assert(x == VK_SUCCESS);