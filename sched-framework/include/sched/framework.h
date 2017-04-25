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

#ifndef __SCHED_FRAMEWORK_H__
#define __SCHED_FRAMEWORK_H__

#include <stdbool.h>
#include <stdint.h>

#include "sched/group.h"
#include "sched/task.h"
#include "synch/mutex.h"
#include "util/list.h"

//============================================================================
// Sched Framework CPU object
//============================================================================

typedef struct 
{
  uint8_t cpu_id;
  uint8_t threads;
  uint8_t threads_free;
  group_t * group;
  bool occupied;
  bool root_cpu;
} 
cpu_t;

//============================================================================
// Sched Framework Event Object
//============================================================================

typedef uint64_t event_t;

typedef struct 
{
  mutex_t lock;
  volatile bool fired;
  volatile uint64_t waiting;
  volatile uint64_t total;
}
event_data_t;

//============================================================================
// Sched Framework Errors
//============================================================================

enum {
  BAD_GROUP_OP,
  GROUP_BAD_DEP,
  GROUP_CREATE_FAILED,
  TASK_CREATE_FAILED,
} 
SCHED_ERROR_TYPE;

//============================================================================
// Sched Framework Member function types
//============================================================================

typedef task_t * (* task_get_next_task_t)(task_t *task);
typedef void (* task_destroy_t)(task_t *task);

typedef int (* sched_create_group_t)(sched_t *self, group_id_t group_id,
                                     group_id_t *dep_ids, uint64_t num_deps);

typedef int (* sched_open_group_t)(sched_t *self, group_id_t group_id);

typedef int (* sched_close_group_t)(sched_t *self, group_id_t group_id);

typedef int (* sched_create_task_t)(sched_t *self, group_id_t group_id, 
                                    task_fn_t fn, task_args_t args); 

//============================================================================
// Sched Framework Object
//============================================================================

typedef struct 
{
  event_data_t *          events;
  uint64_t                num_events;

  uint64_t                num_cpus;
  cpu_t *                 cpus;

  group_id_t              last_retired;

  mutex_t                 group_lock;

  list_t                  unopened_q;
  list_t                  dep_wait_q;
  list_t                  schedule_q;
  list_t                  retire_q;

  sched_create_group_t    create_group;
  sched_open_group_t      open_group;
  sched_close_group_t     close_group;
  sched_create_task_t     create_task;

  //sched_fire_event_t      fire_event;
  //sched_is_event_fired_t  is_event_fired;
  //sched_wait_event_t      wait_event;

  //sched_destroy_t         destroy;
} 
sched_t;

//============================================================================
// Sched Framework Initialization
//============================================================================

/**
  * creates the scheduling framework
  */
sched_t * create_sched(event_t num_events);

#endif //__SCHED_FRAMEWORK_H__
