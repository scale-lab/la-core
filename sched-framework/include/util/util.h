/*
 * Copyright (c) 2017 Samuel Steffl
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Authors: Nick Schank
 *          Samuel Steffl
 */

#ifndef __UTIL_UTIL_H__
#define __UTIL_UTIL_H__

#include <stdio.h>
#include <stdlib.h>

#include "debug/dbg.h"

#define MAX(a,b)									\
  ({ __typeof__ (a) _a = (a);			\
      __typeof__ (b) _b = (b); 		\
    _a > _b ? _a : _b; })
    
#define MIN(a,b)									\
  ({ __typeof__ (a) _a = (a);			\
      __typeof__ (b) _b = (b); 		\
    _a < _b ? _a : _b; })


#ifndef NO_DBG

//does NOT print newline, so user has to
#define PRINT_ERROR(fmt, ...) 							\
  do {																		  \
    fprintf(stderr, fmt, ##__VA_ARGS__);		\
  } while(0)
  

#define PANIC(fmt, ...) 										\
  do {																			\
    PRINT_ERROR(fmt, ##__VA_ARGS__);				\
    dbg(DBG_ERROR, "%s (line %d): " fmt,		\
        __FILE__, __LINE__, ##__VA_ARGS__);	\
    exit(1);																\
  } while(0)


#define ASSERT(predicate, fmt, ...)					\
  do {																			\
    if (!(predicate)) {											\
      PANIC(fmt, ##__VA_ARGS__);						\
    }																				\
  } while(0)

#else 

#define PRINT_ERROR(fmt, ...)
#define PANIC(fmt, ...)
#define ASSERT(predicate, fmt, ...) \
  if(predicate){}

#endif /* NO_DBG */


#endif /* __UTIL_UTIL_H__ */
