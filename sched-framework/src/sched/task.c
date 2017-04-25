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

#include "sched/task.h"

#include <stdlib.h>

#include "sched/framework.h"
#include "sched/group.h"
#include "util/list.h"
#include "util/util.h"

//============================================================================
// Static Implementations of the Task members
//============================================================================

static void destroy_impl(task_t *self)
{
  //DEBUG(DBG_TASK, "task G[%d].T[%d].C[%d] destroyed\n", 
  //  self->group->group_id, self->task_id, self->group->get_cpu_id(group));

  ASSERT(!list_link_in_list(&(self->q_link)), 
    "task destroyed while in queue");
  
  free(self);
}

//============================================================================
// Task Factory
//============================================================================

task_t * create_task(sched_t * sched, group_t *group, task_fn_t fn, 
                     task_args_t args, task_retire_func_t retire_func)
{
  task_t * self = (task_t *)malloc(sizeof(task_t));
  ASSERT(self != NULL, "malloc");

  self->task_id       = group->current_task_id++;
  self->sched         = sched;
  self->group         = group;
  self->fn            = fn;
  self->args          = args;

  list_link_init(&(self->q_link));

  self->retire_func   = retire_func;
  self->destroy       = &destroy_impl;

  //DEBUG(DBG_TASK, "task G[%d].T[%d].C[%d] created\n", 
  //  self->group->group_id, self->task_id, self->group->get_cpu_id(group));

  return self;
}