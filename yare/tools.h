#pragma once

#include <memory>

#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
  TypeName(const TypeName&);   \
  void operator=(const TypeName&); 

#define DECLARE_STD_PTR(TypeName) \
    typedef std::unique_ptr<TypeName> TypeName##Uptr; \
    typedef std::shared_ptr<TypeName> TypeName##Sptr;

/*template <typename T>
using Sptr = std::shared_ptr<T>;

template <typename T>
using Uptr = std::unique_ptr<T>;*/