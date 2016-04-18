#pragma once

#include <assert.h>
#include <iostream>
#include <string>

#define RUNTIME_ERROR(error) {std::cout << (error) << std::endl; assert(false);}
