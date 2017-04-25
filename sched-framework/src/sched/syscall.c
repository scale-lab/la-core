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
 * Authors: Samuel Steffl
 */

/**
  * Task Scheduling Syscall Implementation. Used by the Task Scheduling
  * Framework to issue tasks to specific cpu cores. Two new syscalls are 
  * provided which implement a 'clone()/fork()' behavior and a 'execve()' 
  * behaviour, except you have control over which cpu they execute on
  *
  * NOTE: the core you issue a task to MUST have an available Hardware Thread
  *       or else the syscall will just fail. 
  *
  * reference material:
  *   nullprogram.com/blog/2015/05/15/
  *   syscalls.kernelgrok.com
  */

#include "sched/syscall.h"

#include "sched/task.h"
#include "util/util.h"

//============================================================================
// RISC-V Syscall Execution
//============================================================================

/**
  * RISC-V only has four system call argument registers by convention, and 
  * in gem5. The syscall returns up to 2 values, stored in (a0, a1) registers.
  *
  * Registers For RISC-V:
  * ABI Register Names    = {"zero", "ra", "sp",  "gp",
  *                          "tp",   "t0", "t1",  "t2",
  *                          "s0",   "s1", "a0",  "a1",
  *                          "a2",   "a3", "a4",  "a5",
  *                          "a6",   "a7", "s2",  "s3",
  *                          "s4",   "s5", "s6",  "s7",
  *                          "s8",   "s9", "s10", "s11",
  *                          "t3",   "t4", "t5",  "t6"};
  * ArgumentRegs          = {10, 11, 12, 13, 14, 15, 16, 17};
  * SyscallNumReg         = ArgumentRegs[7];
  * SyscallArgumentRegs[] = {ArgumentRegs[0], ArgumentRegs[1],
  *                          ArgumentRegs[2], ArgumentRegs[3]};
  * ReturnValueRegs[]     = {10, 11};
  * ReturnValueReg        = ReturnValueRegs[0];
  *
  * NOTE: do not inline, it messes up the registers
  *
  * NOTE: don't use 'register' keyword. see reference:
  *       gcc.gnu.org/onlinedocs/gcc/Local-Register-Variables.html
  */
static __attribute__ ((noinline))
int riscv_syscall(int code, void *a0, void *a1, void *a3, void *a4) 
{
  int ret;

  //set the syscall code
  asm volatile ( "mv a7, %0\n\t" : : "r" (code) : "a7" );

  //set the other arguments
  asm volatile ( "mv a0, %0\n\t" : : "r" (a0) : "a0" );
  asm volatile ( "mv a1, %0\n\t" : : "r" (a1) : "a1" );
  asm volatile ( "mv a2, %0\n\t" : : "r" (a3) : "a2" );
  asm volatile ( "mv a3, %0\n\t" : : "r" (a4) : "a3" );

  //syscall and get return value
  asm volatile ( "ecall\n\t" : : : "a0");
  asm volatile ( "mv %0, a0\n\t" : "=r" (ret) : : );

  return ret;
} 
 
//============================================================================
// Task Wrapper
//============================================================================

//forward declaration
int sys_task_swap(task_t *state);

/**
  * The task_wrapper_fn is what initially executes in the new thread right
  * after the syscall returns up the call-stack on the new thread. This 
  * function is responsible for cleaning up the task and scheduling the 
  * next task on this thread if a new task is available.
  */
static void task_wrapper_fn(void *args){
  task_t *task = (task_t *)args;

  // callee is responsible for freeing args if it needs to 
  task->fn(task->args);

  // get the next task if there is one
  task_t *next_task = task->retire_func(task);

  /**
    * task_swap will NOT return, but instead return from the syscall to the
    * start of task_wrapper_fn, with the NEW arguments and stack setup for
    * the new task.
    *
    * If there next_task was NULL, then exit(0) should not return either
    */
  if(next_task != NULL){
    sys_task_swap(next_task);
  } else {
    exit(0);
  }
  PANIC("should never reach here");
}

//============================================================================
// Public Syscall Functions
//============================================================================


int sys_task_clone(task_t *task) 
{
  ASSERT(task != NULL, "invalid task");
  return riscv_syscall(SYS_task_clone, 
                       (void *)( CLONE_VM | CLONE_FS | CLONE_FILES | 
                                 CLONE_SIGHAND | CLONE_THREAD),
                       (void *)( task->cpu->cpu_id ),
                       (void *)( &task_wrapper_fn ),
                       (void *)( task ));
}


int sys_task_swap(task_t *task)
{
  ASSERT(task != NULL, "invalid task");
  return riscv_syscall(SYS_task_swap, 
                       (void *)( &task_wrapper_fn ), 
                       (void *)( task ),
                       NULL, 
                       NULL);
}

int sys_cpu_count(uint64_t *num_cpus, uint64_t *cur_cpu)
{
  ASSERT(num_cpus != NULL && cur_cpu != NULL, "invalid task");
  int res = riscv_syscall(SYS_task_cpu_count, NULL, NULL, NULL, NULL);
  *num_cpus = ((res >> 32) && 0xffffffff);
  *cur_cpu = (res && 0xffffffff);
  return 0;
}

int sys_cpu_info(uint64_t cpu_id, uint64_t *total, uint64_t *free)
{
  ASSERT(total != NULL && free != NULL, "invalid task");
  int res = riscv_syscall(SYS_task_cpu_info, (void *)cpu_id, NULL, NULL, NULL);
  *total = ((res >> 32) && 0xffffffff);
  *free = (res && 0xffffffff);
  return 0;
}
