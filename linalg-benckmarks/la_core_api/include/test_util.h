//==========================================================================
// Copyright (c) 2017 Samuel Steffl
// All rights reserved.
// 
// Simple Lightweight Test Framework for the LACore Framework.
//==========================================================================

#ifndef __TEST_UTIL_H__
#define __TEST_UTIL_H__

#include <math.h>
#include <stdint.h>
#include <stdbool.h>

#include "util.h"
#include "la_core/la_core.h"

//==========================================================================
// Floating point Error Threshold
//==========================================================================

// what our error threshold is (arbitrary?)
#define VERIFY_EPSILON 1e-4

#define MARGIN_EXCEEDED(num1, num2) \
  (fabs(num1 - num2) > VERIFY_EPSILON * fmin(fabs(num1), fabs(num2)))


//==========================================================================
// DEFINING AND RUNNING TEST CASES AND TESTS
//==========================================================================

#define TEST_CASE(name)                                           \
static void test_ ## name () {                                    \
  bool     __success = true;                                      \
  uint32_t __tests = 0;                                           \
  uint32_t __failed = 0;                                          \
  uint32_t __alloced = 0;                                         \
  const char *__test_name = #name;                                \
  do                                                              \


#define END_TEST_CASE()                                           \
  while (0);                                                      \
  PRINT("%s %s, %d run, %d failed\n", __test_name,                \
    (__success ? "SUCCESS" : "FAIL"), __tests, __failed);         \
  ASSERT(__alloced == 0, "%s didn't clean up resources\n",        \
    __test_name);                                                 \
}                                                                 \

#define RUN_TEST(name)                                            \
  test_##name();

//==========================================================================
// RUNNING TESTS WITHIN A TEST CASE
//==========================================================================

#define EXPECT(expr, msg, ...)                                      \
  do {                                                              \
    ++__tests;                                                      \
    if(!(expr)) {                                                   \
      PRINT("  ");                                                  \
      PRINT(msg, ##__VA_ARGS__);                                    \
      __success = false;                                            \
      ++__failed;                                                   \
    }                                                               \
  } while(0)                                                        \


#define EXPECT_EQUAL(data1, data2)                                  \
  EXPECT(!MARGIN_EXCEEDED((data1), (data2)),                        \
    #data1 " is %f, but " #data2 " is %f\n",                        \
    (data1), (data2));                                              \


#define EXPECT_ARRAYS_EQUAL(data1, data2, count)                    \
  for(LaRegIdx i=0; i<(count); ++i){                                \
    EXPECT(!MARGIN_EXCEEDED((data1)[i], (data2)[i]),                \
      (#data1 "[%d] is %f, but " #data2 "[%d] is %f\n"),            \
      i, (data1)[i], i, (data2)[i]);                                \
  }                                                                 \


#define EXPECT_ARRAY_EQUALS_VAL(data, value, count)                 \
  for(LaRegIdx i=0; i<(count); ++i){                                \
    EXPECT(!MARGIN_EXCEEDED((data)[i], (value)),                    \
      (#data "[%d] is %f, but " #value " is %f\n"),                 \
      i, (data)[i], (value));                                       \
  }                                                                 \

#define EXPECT_ARRAY_STRIDE_EQUALS_VAL(data, value, count, stride)  \
  for(LaRegIdx i=0; i<(count); i+=(stride)){                        \
    EXPECT(!MARGIN_EXCEEDED((data)[i], (value)),                    \
      #data "[%d] is %f, but " #value " is %f\n",                   \
      (data)[i], i, (value));                                       \
  }                                                                 \

//==========================================================================
// DATA ALLOCATION/FREE UTILITIES
//==========================================================================

#define DP_ARRAY(name, n)                                         \
  ++__alloced;                                                    \
  double *name = (double *)malloc(n*sizeof(double))               \

#define SP_ARRAY(name, n)                                         \
  ++__alloced;                                                    \
  float *name = (float *)malloc(n*sizeof(float))                  \

#define FREE(name)                                                \
  --__alloced;                                                    \
  free(name);                                                     \

//==========================================================================
// DATA INITIALIZATION
//==========================================================================

#define DP_SCALAR_INIT(scalar, val)                               \
  (scalar) = (double)(val);                                       \

#define DP_ARRAY_INIT_VAL(array, val, count)                      \
  for(LaRegIdx i=0; i<(count); ++i){                              \
    (array)[i] = (double)(val);                                   \
  }                                                               \

#define DP_ARRAY_INIT_INCR(array, start_val, count)               \
  for(LaRegIdx i=0; i<(count); ++i){                              \
    (array)[i] = (double)(start_val) + i;                         \
  }                                                               \


#define SP_SCALAR_INIT(scalar, val)                               \
  (scalar) = (float)(val);                                        \

#define SP_ARRAY_INIT_VAL(array, val, count)                      \
  for(LaRegIdx i=0; i<(count); ++i){                              \
    (array)[i] = (float)(val);                                    \
  }                                                               \

#define SP_ARRAY_INIT_INCR(array, start_val, count)               \
  for(LaRegIdx i=0; i<(count); ++i){                              \
    (array)[i] = (float)(start_val) + i;                          \
  }                                                               \

//==========================================================================
// LAReg ITERATOR UTILITIES (each produces 56 unique iterations)
//==========================================================================

#define ITERATE_2_REGS(reg1, reg2)                                \
  for(LaRegIdx reg1=0; reg1<NumLAReg; ++reg1){                    \
    for(LaRegIdx reg2=0; reg2<NumLAReg; ++reg2){                  \
      if(reg1 != reg2) {                                          \
        do                                                        \


#define END_ITERATE_2_REGS()                                      \
        while (0);                                                \
      }                                                           \
    }                                                             \
  }                                                               \


#define ITERATE_3_REGS(reg1, reg2, reg3)                          \
  for(LaRegIdx reg1=0; reg1<NumLAReg; ++reg1){                    \
    for(LaRegIdx reg2=0; reg2<NumLAReg; ++reg2){                  \
      LaRegIdx reg3 = (reg2 + (NumLAReg/2)) % NumLAReg;           \
      if((reg1 != reg2) && (reg1 != reg3)) {                      \
        do                                                        \


#define END_ITERATE_3_REGS()                                      \
        while (0);                                                \
      }                                                           \
    }                                                             \
  }                                                               \

#define ITERATE_4_REGS(reg1, reg2, reg3, reg4)                    \
  for(LaRegIdx reg1=0; reg1<NumLAReg; ++reg1){                    \
    for(LaRegIdx reg2=0; reg2<NumLAReg; ++reg2){                  \
      LaRegIdx reg3 = (reg1 + (NumLAReg/2)) % NumLAReg;           \
      LaRegIdx reg4 = (reg2 + (NumLAReg/2)) % NumLAReg;           \
      if((reg1 != reg2) && (reg1 != reg4) &&                      \
          (reg3 != reg2) && (reg3 != reg4)) {                     \
        do                                                        \


#define END_ITERATE_4_REGS()                                      \
        while (0);                                                \
      }                                                           \
    }                                                             \
  }                                                               \


#endif // __TEST_UTIL_H__
