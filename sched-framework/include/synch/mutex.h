/*
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
 * Authors: Samuel Steffl
 */

/**
  * RISC-V lightweight, spinlock-based mutex implementation.
  * 
  * Motivation is the same as in the spinlock.h file. Additionally, gem5 does
  * not support pthreads, so we need a way to include synchronize without
  * using the pthread library (or m5threads, by Daniel Sanchez)
  *
  * For reference, see the following:
  *   repo.gem5.org/m5threads/file/dcec9ee72f99/pthread_defs.h
  *   repo.gem5.org/m5threads/file/dcec9ee72f99/pthread.c
  *   refspecs.linuxfoundation.org/LSB_1.3.0/gLSB/gLSB/libpthread-ddefs.html
  *
  * All functions require a pointer to a mutex
  */

#ifndef __SYNCH_MUTEX_H__
#define __SYNCH_MUTEX_H__

#include "synch/spinlock.h"

typedef struct { 
  spinlock_t sl; 
} mutex_t;


#define mutex_init(mutex)         \
  do {                            \
    (mutex)->sl.lock = 0;         \
  } while ( 0 )


#define mutex_lock(mutex)         \
  do {                            \
    spin_lock(&((mutex)->sl));    \
  } while ( 0 )


#define mutex_unlock(mutex)       \
  do {                            \
    spin_unlock(&((mutex)->sl));  \
  } while ( 0 )

/**
  * returns 1 on locked, 0 on failed to lock
  */
#define mutex_trylock(mutex)      \
  spin_trylock(&((mutex)->sl));


#endif // __SYNCH_MUTEX_H__

