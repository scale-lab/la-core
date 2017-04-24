//==========================================================================
// Copyright (c) 2017 Samuel Steffl
// All rights reserved.
// 
// Common file for verify functions
//==========================================================================

#ifndef __VERIFY_COMMON_H__
#define __VERIFY_COMMON_H__

#include <math.h>

//==========================================================================
// Check if floating point values differ by more than factor of epsilon
//==========================================================================

//TODO: is this too high?
#define VERIFY_EPSILON 1e-4

#define MARGIN_EXCEEDED(num1, num2) \
  (fabs(num1 - num2) > VERIFY_EPSILON * fmin(fabs(num1), fabs(num2)))


#endif //__VERIFY_COMMON_H__