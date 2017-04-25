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

#ifndef __SCHED_TASK_H__
#define __SCHED_TASK_H__

#include <stdint.h>

#include "sched/framework.h"
#include "sched/group.h"
#include "util/list.h"

//============================================================================
// Task Types
//============================================================================

//forward decls
typedef struct task_t;
typedef struct group_t;
typedef struct sched_t;
typedef struct cpu_t;

//task id
typedef uint64_t task_id_t;

/**
  * task_fn_t is the function a task executes. it can take 1 arg and
  * return nothing. task_args_t can be anything.
  *
  * NOTE: the task is responsible for free()ing the args if they need to be
  *       freed. the framework will not free them for you.
  */
typedef void * task_args_t;
typedef void (* task_fn_t)(task_args_t args);


/**
  * 'pseudo-methods' on the task
  */
typedef task_t * (* task_retire_func_t)(task_t *task);
typedef void (* task_destroy_t)(task_t *task);

//============================================================================
// Task
//============================================================================

typedef struct 
{
  task_id_t             task_id;   
  sched_t *             sched;
  group_t *             group;
  cpu_t *               cpu;
  task_fn_t             fn;
  task_args_t           args; 

  list_link_t           q_link; 

  task_retire_func_t    retire_func;
  task_destroy_t        destroy;
} 
task_t;

//============================================================================
// Task Factory
//============================================================================

/**
  * THE ONLY WAY YOU SHOULD CREATE TASKS!
  */
task_t * create_task(sched_t * sched, group_t *group, task_fn_t fn, 
                     task_args_t args, task_retire_func_t retire_func);

#endif // __SCHED_TASK_H__