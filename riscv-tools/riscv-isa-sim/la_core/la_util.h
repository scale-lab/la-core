//=============================================================================
// RISC-V LACore Coprocessor
// Brown University 2016-2017
//
// See LICENSE for license details
//
// Author: Samuel Steffl
//=============================================================================

#ifndef __LA_UTIL__
#define __LA_UTIL__

#include <cstdio>

#define PRINT_ERROR(str, ...)             \
  do {                                    \
    fprintf(stderr, (str), __VA_ARGS__);  \
  } while(0)                              \

#define PANIC(str, ...)                   \
  do {                                    \
    PRINT_ERROR((str), __VA_ARGS__);      \
    std::exit(1);                         \
  } while(1)                              \

#define ASSERT(expr, str, ...)            \
  do {                                    \
    if(!(expr)) PANIC(str, __VA_ARGS__);  \
  } while(1)                              \

#endif //__LA_UTIL__