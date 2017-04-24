//==========================================================================
// Copyright (c) 2017 Samuel Steffl
// All rights reserved.
// 
// Utility routine for the LACore framework. Mostly for debugging/assertions
//==========================================================================

#ifndef __UTIL_H__
#define __UTIL_H__

#include <stdio.h>
#include <stdlib.h>

#define MAX(a,b)                  \
  ({ __typeof__ (a) _a = (a);     \
      __typeof__ (b) _b = (b);    \
    _a > _b ? _a : _b; })
    
#define MIN(a,b)                  \
  ({ __typeof__ (a) _a = (a);     \
      __typeof__ (b) _b = (b);    \
    _a < _b ? _a : _b; })


#ifndef NO_DBG

//does NOT print newline, so user has to
#define PRINT(fmt, ...)                     \
  do {                                      \
    fprintf(stderr, fmt, ##__VA_ARGS__);    \
  } while(0)
  

#define PANIC(fmt, ...)                     \
  do {                                      \
    PRINT(fmt, ##__VA_ARGS__);              \
    exit(1);                                \
  } while(0)


#define ASSERT(predicate, fmt, ...)         \
  do {                                      \
    if (!(predicate)) {                     \
      PANIC(fmt, ##__VA_ARGS__);            \
    }                                       \
  } while(0)

#else 

#define PRINT(fmt, ...)
#define PANIC(fmt, ...)
#define ASSERT(predicate, fmt, ...) \
  if(predicate){}

#endif // NO_DBG


#endif // __UTIL_H__
