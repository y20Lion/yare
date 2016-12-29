#pragma once

#include <iterator>
#include "tools.h"

template<typename ... Args>
std::string string_format_str(const std::string& format, Args ... args)
{
   size_t size = snprintf(nullptr, 0, format.c_str(), args.c_str() ...) + 1; // Extra space for '\0'
   std::unique_ptr<char[]> buf(new char[size]);
   snprintf(buf.get(), size, format.c_str(), args.c_str() ...);
   auto a = std::string(buf.get());
   return a;
}

template<typename ... Args>
std::string string_format(const std::string& format, Args ... args)
{
   size_t size = snprintf(nullptr, 0, format.c_str(), args ...) + 1; // Extra space for '\0'
   std::string result(size, '\0');
   snprintf((char*)result.data(), size, format.c_str(), args ...);
   return result;
}

template <typename T>
struct reversion_wrapper { T& iterable; };

template <typename T>
auto begin(reversion_wrapper<T> w) { return std::rbegin(w.iterable); }

template <typename T>
auto end(reversion_wrapper<T> w) { return std::rend(w.iterable); }

template <typename T>
reversion_wrapper<T> make_reverse(T&& iterable) { return{ iterable }; }

#define RANGE(collection) collection.begin(), collection.end() 

template <typename T>
class Range
{
public:
   Range() {}
   template <typename Titerable>
   Range(Titerable&& iterable) : it_start(iterable.begin()), it_end(iterable.end()) {}
   Range(T it_start, T it_end) : it_start(it_start), it_end(it_end) {}
   DEFAULT_COPY_AND_ASSIGN(Range)

      T begin() const { return it_start; }
   T end() const { return it_end; }

   T it_start;
   T it_end;
};