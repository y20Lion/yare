#pragma once

#include <iterator>

#define RANGE(collection) collection.begin(), collection.end() 

template <typename T>
struct reversion_wrapper { T& iterable; };

template <typename T>
auto begin(reversion_wrapper<T> w) { return std::rbegin(w.iterable); }

template <typename T>
auto end(reversion_wrapper<T> w) { return std::rend(w.iterable); }

template <typename T>
reversion_wrapper<T> make_reverse(T&& iterable) { return{ iterable }; }