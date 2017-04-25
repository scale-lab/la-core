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
  * RISC-V lightweight, spinlock-based condition variable implementation.
  * 
  * Motivation is the same as in the spinlock.h file. Additionally, gem5 does
  * not support pthreads, so we need a way to include synchronize without
  * using the pthread library (or m5threads, by Daniel Sanchez)
  *
  * For reference, see the following:
  * refspecs.linuxfoundation.org/LSB_1.3.0/gLSB/gLSB/libpthread-ddefs.html
  * 
  * All functions require a pointers to a condition variables (and mutexes)
  *
  * KNOWN RACE CONDITION: if the threads are clearing out after a broadcast
  * and another thread calls cond_wait() in the middle of the clearing out,
  * it will just pass on through. 
  * NOTE: this bug was in the gem5 m5threads implementation too.
  */

#ifndef __SYNCH_COND_H__
#define __SYNCH_COND_H__

#include "synch/mutex.h"
#include "synch/spinlock.h"


typedef struct{
  volatile unsigned int status;
  volatile unsigned int waiting;
  spinlock_t sl; 
} cond_t;


#define cond_init (cond)        \
  do {                          \
    cond->status = 0;           \
    cond->waiting = 0;          \
    cond->sl.lock = 0;          \
  } while ( 0 )

/**
  * 1) lock mtx
  * 2) call cond_wait
  * 3) another thread calls cond_broadcast on the cond (as it holds mtx)
  * 4) you return from this function
  */
#define cond_broadcast (cond)   \
  do {                          \
    cond->status = 1;
  } while ( 0 )

/**
  * 1) lock mtx
  * 2) call cond_wait
  * 3) another thread calls cond_broadcast on the cond (as it holds mtx)
  * 4) you return from this function
  */
#define cond_wait (cond, mtx)   \
  do {                          \
    cond->waiting++;            \
    mutex_unlock(mtx);          \
                                \
    while (1) {                 \
      if (cond->status == 1) {  \
         break;                 \
      }                         \
    }                           \
                                \
    spin_lock(&cond->sl);       \
    cond->waiting--;            \
    if (cond->waiting == 0) {   \
      cond->status = 0;         \
    }                           \
    spin_unlock(&cond->sl);     \
                                \
    mutex_lock(mtx);            \
  } while ( 0 )


#endif // __SYNCH_COND_H__