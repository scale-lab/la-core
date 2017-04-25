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

#include "sched/group.h"

#include <stdint.h>
#include <stdlib.h>

#include "sched/task.h"
#include "util/list.h"
#include "util/util.h"

//============================================================================
// Statics
//============================================================================

void destroy_impl(group_t *self) 
{
  //DEBUG(DBG_GROUP, "group G[%d].C[%d] destroyed\n", 
  //  self->group_id, self->get_cpu_id(self));

  ASSERT(list_empty(&(self->task_q)), "non-empty task_q on destroy");
  ASSERT(!list_link_in_list(&(self->q_link)), "group in list on destroy");
  free(self);
}

//============================================================================
// Group Factory
//============================================================================

group_t * create_group(group_id_t group_id, group_id_t * dep_ids, 
                       uint64_t num_deps)
{
  group_t * self = (group_t *)malloc(sizeof(group_t));
  ASSERT(self != NULL, "malloc");

  self->group_id        = group_id;
  self->current_task_id = 0;
  self->state           = GROUP_INIT;

  self->num_deps        = num_deps;
  self->dep_ids         = dep_ids;

  list_init(&(self->task_q));
  list_link_init(&(self->q_link));

  self->destroy         = &destroy_impl;

  //DEBUG(DBG_GROUP, "group G[%d].C[%d] created\n", 
  //  self->group_id, self->get_cpu_id(group));

  return self;
}