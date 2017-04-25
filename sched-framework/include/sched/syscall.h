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

#ifndef __SCHED_SYSCALL_H__
#define __SCHED_SYSCALL_H__

#include <stdint.h>

#include "sched/task.h"

//============================================================================
// Syscall related constants
//============================================================================

/**
  * RISC-V syscalls using newlib standard library
  */
#define SYS_task_clone      4096
#define SYS_task_swap       4097 
#define SYS_task_cpu_count  4098
#define SYS_task_cpu_info   4099
  

/** 
  * some of the relevant clone() syscall flags for gem5
  * 
  * see here: 
  * lxr.free-electrons.com/source/include/uapi/linux/sched.h
  */
#define CLONE_VM              0x00000100
#define CLONE_FS              0x00000200
#define CLONE_FILES           0x00000400
#define CLONE_SIGHAND         0x00000800
#define CLONE_THREAD          0x00010000

//============================================================================
// Public Syscall Functions
//============================================================================

/**
  * creates the task in a new hardware thread on a specific CPU given by 
  * the task_t struct. Analagous to "fork" or "clone" system calls, except
  * the user has full control over the cpu the task is issued to
  *
  * The stack/thread state is managed by the framework, so the user doesn't 
  * need to worry about it.
  */
int sys_task_clone(task_t *task);


/**
  * swaps the task with the current task on the same hardware thread.
  * This is analagous to "execve" system call, except
  * the user has full control over the hardware thread the task is issued to
  *
  * The stack/thread state is managed by the framework, so the user doesn't 
  * need to worry about it. The stack is, in fact, recycled from the currently
  * running thread.
  */
int sys_task_swap(task_t *task);

/**
  * returns the number of cpus available to this process as well as the
  * cpu_id of the currently calling thread
  */
int sys_cpu_count(uint64_t *num_cpus, uint64_t *cur_cpu);

/**
  * you pass in the cpu_id, and the syscall returns the total number of
  * hardware threads on the core available to this process, 
  * and of those, the number of threads that are currently idle (or free)
  */
int sys_cpu_info(uint64_t cpu_id, uint64_t *total, uint64_t *free);


#endif // __SCHED_SYSCALL_H__
