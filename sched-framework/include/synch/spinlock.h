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
  * RISC-V ISA spinlock impolementation
  *
  * Motivation for design: regular blocking mutexes are too heavyweight for
  * the highly parallel HPC applications. Using spinlocks for threads decreases
  * synchronization time at the expense of no other threads using the hart.
  * This shouldn't be a problem in HPC applications with a medium amount of
  * medium-to-heavyweight threads. The LACore design, for example, benefits
  * from spinlocks over blocking locks
  *
  * For reference, see this github:
  * riscv/riscv-linux/blob/master/arch/riscv/include/asm/spinlock.h
  * 
  */

#ifndef __SYNCH_SPINLOCK_H__
#define __SYNCH_SPINLOCK_H__

typedef struct {
  volatile unsigned int lock;
} spinlock_t;

#define spin_is_locked(sl)  ((sl)->lock != 0)

static inline void spin_unlock(spinlock_t *sl)
{
  __asm__ __volatile__ (
    "amoswap.w.rl x0, x0, %0"
    : "=A" (sl->lock)
    :
    : "memory");
}

static inline int spin_trylock(spinlock_t *sl)
{
  int tmp = 1, busy;

  __asm__ __volatile__ (
    "amoswap.w.aq %0, %2, %1"
    : "=r" (busy), "+A" (sl->lock)
    : "r" (tmp)
    : "memory");

  return !busy;
}

static inline void spin_lock(spinlock_t *sl)
{
  while (1) {
    if (spin_is_locked(sl))
      continue;

    if (spin_trylock(sl))
      break;
  }
}


#endif /* __SYNCH_SPINLOCK_H__ */
