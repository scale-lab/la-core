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

#include "sched/framework.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "sched/group.h"
#include "sched/task.h"
#include "sched/syscall.h"
#include "synch/mutex.h"
#include "util/list.h"
#include "util/util.h"

static sched_t *singleton = NULL;

//============================================================================
// INTERNAL HARDWARE THREAD TASK SWAPPING API
//============================================================================

//
//priority queue, so walk from front to end. smallest group_ids in front
//
static void insert_into_retire_q(sched_t *self, group_t * retired_group)
{
  group_t *group;
  list_iterate_begin(&(self->retire_q), group, group_t, q_link)
    ASSERT(group->group_id != retired_group->group_id,
           "groups have same id\n");
    if (group->group_id > retired_group->group_id) {
      list_insert_before(&(retired_group->q_link), &(group->q_link));
      return;
    }
  list_iterate_end();
  list_insert_tail(&(self->retire_q), &(retired_group->q_link));
}

static void retire_group(sched_t *self, group_t *group)
{
  insert_into_retire_q(self, group);
  group_t *group;
  list_iterate_begin(&(self->retire_q), group, group_t, q_link)
    if(group->group_id == (self->last_retired + 1)){
      ++self->last_retired;
      list_remove(&(group->q_link));
      group->destroy(group);
    }
    else {
      return;
    }
  list_iterate_end();
}

static group_t * find_next_group_for_cpu(sched_t *self)
{
  group_t *group;
  while(!list_empty(&(self->schedule_q))){
    group = list_head(&(self->schedule_q), group_t, q_link);
    list_remove_head(&(self->schedule_q));
    if(list_empty(&(group->task_q)) && group->state == GROUP_CLOSED){
      retire_group(self, group);
    } 
    else {
      return group;
    }
  }
  return NULL;
}


//task_retire_func pseudocode:
//   LOCK MUTEX
//   destroy task, set cpu->task to NULL
//   if group has more tasks
//     pop task off queue, put on cpu->task, and return it
//   else if group closed and all tasks finished
//     retire_group and cpu->occupied = false
//     if(schedule_q not empty)
//       assign next group to cpu, cpu->occuped = true
//       if(group has tasks)
//         pop task off queue put on cpu and return it
//       else UNLOCK MUTEX/return NULL
//     else UNLOCK MUTEX/return NULL
//   else UNLOCK MUTEX/return NULL
static task_t * task_retire_func(task_t *task)
{
  sched_t *self = task->sched;
  group_t *group = task->group;
  cpu_t *cpu = task->cpu;

  //DEBUG(DBG_TASK, "task G[%d].T[%d].C[%d] finding next task\n", 
  //  self->group->group_id, self->task_id, self->group->get_cpu_id(group));

  mutex_lock(&(self->group_lock));
  ASSERT((++cpu->threads_free) <= cpu->threads, "invalid cpu threads free\n");
  ASSERT(cpu->occupied, "cpu flag incorrectly says not occupied\n");
  task->destroy(task);

  if(!list_empty(&(group->task_q))){
    task = list_head(&(group->task_q), task_t, q_link);
    list_remove_head(&(group->task_q));
    task->cpu = cpu;
    --cpu->threads_free;
    mutex_unlock(&(self->group_lock));
    return task;
  }
  else if(group->state == GROUP_CLOSED && cpu->threads_free == cpu->threads){
    cpu->occupied = false;
    cpu->group = NULL;
    retire_group(self, group);

    if((group = find_next_group_for_cpu(self)) != NULL){
      cpu->occupied = true;
      cpu->group = group;
      task = list_head(&(group->task_q), task_t, q_link);
      list_remove_head(&(group->task_q));
      task->cpu = cpu;
      --cpu->threads_free;
      mutex_unlock(&(self->group_lock));
      return task;
    }
  }
  mutex_unlock(&(self->group_lock));
  return NULL;
}




//============================================================================
// UTILITY FUNCTIONS USED BY MAIN GROUP CREATE/OPEN/CLOSE/CREATE_TASK
//
// All utility functions assume that any necessary mutexes are locked for the
// duration of the call. They do NOT perform any synchronization.
//============================================================================

static bool is_group_in_unopened_q(sched_t *self, group_id_t group_id)
{
  group_t *group;
  list_iterate_begin(&(self->unopened_q), group, group_t, q_link)
    if(group->group_id == group_id){
      return true;
    }
  list_iterate_end();
  return false;
}

static bool is_group_in_dep_wait_q(sched_t *self, group_id_t group_id)
{
  group_t *group;
  list_iterate_begin(&(self->dep_wait_q), group, group_t, q_link)
    if(group->group_id == group_id){
      return true;
    }
  list_iterate_end();
  return false;
}

static bool is_group_in_schedule_q(sched_t *self, group_id_t group_id)
{
  group_t *group;
  list_iterate_begin(&(self->schedule_q), group, group_t, q_link) 
    if(group->group_id == group_id){
      return true;
    }
  list_iterate_end();
  return false;
}

static bool is_group_in_a_cpu(sched_t *self, group_id_t group_id)
{
  for(unsigned i=0; i<self->num_cpus; ++i){
    if(self->cpus[i].occupied && self->cpus[i].group->group_id == group_id){
      return true;
    }
  }
  return false;
}

static bool is_group_retired(sched_t *self, group_id_t group_id)
{
  if(group_id <= self->last_retired){
    return true;
  }
  group_t *group;
  list_iterate_begin(&(self->retire_q), group, group_t, q_link)
    if(group->group_id == group_id){
      return true;
    }
  list_iterate_end();
  return false;
}

static bool can_schedule_group(sched_t *self, group_t *group)
{
  for(unsigned i=0; i<group->num_deps; ++i){
    group_id_t dep_id = group->dep_ids[i];

    if(is_group_in_unopened_q(self, dep_id)){
      return false;
    }
    if(is_group_in_dep_wait_q(self, dep_id)){
      return false;
    } 
    else if(is_group_in_schedule_q(self, dep_id)){
      continue;
    }
    else if(is_group_in_a_cpu(self, dep_id)){
      continue;
    }
    else if(is_group_retired(self, dep_id)){
      continue;
    }
    else {
      //dependency has not even been scheduled yet
      return false;
    }
  }
  //all deps are either retired, scheduled or in a cpu, so we're good
  return true;
}

static cpu_t * find_unoccupied_cpu(sched_t *self)
{
  for(unsigned i=0; i<self->num_cpus; ++i){
    if(!self->cpus[i].occupied && self->cpus[i].threads_free > 0){
      return &(self->cpus[i]);
    }
  }
  return NULL;
}

static cpu_t * find_cpu_by_group_id(sched_t *self, group_id_t group_id)
{
  for(unsigned i=0; i<self->num_cpus; ++i){
    if(!self->cpus[i].occupied && self->cpus[i].group->group_id == group_id){
      return &(self->cpus[i]);
    }
  }
  return NULL;
}

static bool try_issue_task(cpu_t *cpu)
{
  ASSERT(cpu->occupied, "cpu is not occupied\n");
  if(cpu->threads_free == 0){
    return false;
  }
  task_t *task;
  if(!list_empty(&(cpu->group->task_q))){
    task = list_head(&(cpu->group->task_q), task_t, q_link);
    list_remove_head(&(cpu->group->task_q));
    --cpu->threads_free;
    task->cpu = cpu;
    ASSERT(sys_task_clone(task) == 0, "sys_task_clone failed\n");
    return true;
  }
  return false;
}


// Pseudocode:
//  if sched_q empty, 
//    if any cpu is unoccupied
//      assign group to cpu
//      for each open hardware thread
//        if group has task in front of q, schedule it now
//    else put at end of sched_q
//  else put at end of sched_q
static void schedule_group(sched_t *self, group_t *group)
{
  cpu_t * cpu;
  if(list_empty(&(self->schedule_q))){
    if((cpu = find_unoccupied_cpu(self)) != NULL){
      cpu->occupied = true;
      cpu->group = group;
      while(try_issue_task(cpu, group)) {
        //task issue successfull
      }
    } 
    else {
      list_insert_tail(&(self->schedule_q), &(group->q_link));
    }
  }
}

/**
  * Assume lock
  */
//for each group in dep_wait_q in increasing numeric order
//  if can_schedule()
//    remove from dep_queue
//    schedule_group()
static void check_dep_wait_q(sched_t *self)
{
  group_t *group;
  list_iterate_begin(&(self->dep_wait_q), group, group_t, q_link)
    if(can_schedule_group(self, group)){
      list_remove(&(group->q_link));
      schedule_group(self, group);
    }
  list_iterate_end();
}

static group_t * retrieve_group_from_q(sched_t *self, group_id_t group_id)
{
  group_t *group;
  list_iterate_begin(&(self->schedule_q), group, group_t, q_link)
    if(group->group_id == group_id){
      return group;
    }
  list_iterate_end();
  return NULL;
}

static group_t * remove_group_from_q(sched_t *self, group_id_t group_id)
{
  group_t *group = retrieve_group_from_q(self, group_id);
  if(group != NULL){
    list_remove(&(group->q_link));
  }
  return group;
}


static bool group_id_duplicate_check(sched_t * self, group_id_t group_id)
{
  if (is_group_in_unopened_q(self, group_id)) {
    return true;
  }
  if (is_group_in_dep_wait_q(self, group_id)) {
    return true;
  }
  else if (is_group_in_schedule_q(self, group_id)) {
    return true;
  }
  else if (is_group_in_a_cpu(self, group_id)) {
    return true;
  }
  else if (is_group_retired(self, group_id)) {
    return true;
  }
  return false;
}

static bool group_fails_dep_check(group_id_t group_id,
  group_id_t *dep_ids, uint64_t num_deps)
{
  for (unsigned i = 0; i<num_deps; ++i) {
    if (group_id <= dep_ids[i]) {
      return true;
    }
  }
  return false;
}

//============================================================================
// CREATE GROUP
//
// Pseudocode:
//   LOCK MUTEX
//   if group has a bad dependency (id < its id)
//     UNLOCK MUTEX/return error
//   else if group id is a duplicate
//     UNLOCK MUTEX/return error
//   else create group
//   UNLOCK MUTEX
//============================================================================


static int create_group_impl(sched_t *self, group_id_t group_id,
  group_id_t *dep_ids, uint64_t num_deps)
{
  mutex_lock(&(self->group_lock));
  if (group_fails_dep_check(group_id, dep_ids, num_deps)) {
    mutex_unlock(&(self->group_lock));
    return GROUP_BAD_DEP;
  }
  if (group_id_duplicate_check(self, group_id)) {
    mutex_unlock(&(self->group_lock));
    return GROUP_BAD_DEP;
  }
  group_t *group = create_group(group_id, dep_ids, num_deps);
  if (group == NULL) {
    mutex_unlock(&(self->group_lock));
    return GROUP_CREATE_FAILED;
  }
  list_insert_tail(&(self->unopened_q), &(group->q_link));
  mutex_unlock(&(self->group_lock));
  return 0;
}

//============================================================================
// OPEN GROUP
//
// group_id must be > 0 (due to last_retired initializing to 0)
//
// Need 1 mutex for the whole thing, very coarse grained but its fine for now
// Pseudocode:
//   LOCK MUTEX
//   if group not in unopened_q
//     UNLOCK MUTEX
//     return error
//   set state: init-->open
//   if can_schedule_group()
//     schedule_group()
//     check_dep_wait_q()
//   else put in wait_dep_q
//   UNLOCK MUTEX
//============================================================================

static int open_group_impl(sched_t *self, group_id_t group_id)
{
  mutex_lock(&(self->group_lock));
  if(!is_group_in_unopened_q(self, group_id)){
    mutex_unlock(&(self->group_lock));
    return BAD_GROUP_OP;
  }
  group_t * group = remove_group_from_q(&(self->unopened_q), group_id);
  ASSERT(group != NULL, "group was not in unopened queue\n");

  group->state = GROUP_OPEN;
  if(can_schedule_group(self, group)){
    schedule_group(self, group);
    check_dep_wait_q(self);
  }
  else {
    list_insert_tail(&(self->dep_wait_q), &(group->q_link));
  }
  mutex_unlock(&(self->group_lock));
}

//============================================================================
// CLOSE GROUP
//============================================================================

//Pseudocode:
//  LOCK MUTEX
//  if group in dep_wait_q, schedule_q or in a cpu
//    if group is open
//      close group
//      if not on cpu, and has no tasks, retire group
//    else UNLOCK MUTEX/return error
//  else UNLOCK MUTEX/return error
//  UNLOCK MUTEX
static int close_group_impl(sched_t *self, group_id_t group_id)
{
  group_t * group;
  cpu_t * cpu;
  bool is_in_cpu = false;

  mutex_lock(&(self->group_lock));
  if(is_group_in_unopened_q(self, group_id)){
    mutex_unlock(&(self->group_lock));
    return BAD_GROUP_OP;
  }
  if(is_group_in_dep_wait_q(self, group_id)){
    group = retrieve_group_from_q(&(self->dep_wait_q), group_id);
  } 
  else if(is_group_in_schedule_q(self, group_id)){
    group = retrieve_group_from_q(&(self->schedule_q), group_id);
  }
  else if(is_group_in_a_cpu(self, group_id)){
    ASSERT((cpu = find_cpu_by_group_id(self, group_id)) != NULL, 
           "group not in cpu\n");
    group = cpu->group;
    is_in_cpu = true;
  }
  else if(is_group_retired(self, group_id)){
    mutex_unlock(&(self->group_lock));
    return BAD_GROUP_OP;
  }
  else {
    //group doesn't even exist yet
    mutex_unlock(&(self->group_lock));
    return BAD_GROUP_OP;
  }
  ASSERT(group != NULL, "group not in queue or cpu\n");
  ASSERT(group->state != GROUP_INIT, "invalid group state\n");
  if(group->state == GROUP_CLOSED){
    mutex_unlock(&(self->group_lock));
    return BAD_GROUP_OP;
  }
  group->state = GROUP_CLOSED;
  if(list_empty(&(group->task_q))){
    list_remove(&(group->q_link));
    retire_group(self, group);
  }
  mutex_unlock(&(self->group_lock));
  return 0;
}

//============================================================================
// CREATE TASK
//============================================================================

// Pseudocode:
//   LOCK MUTEX
//   if group exists in any queue or cpu
//     if group is open
//       create task and push to back of queue
//       if group on cpu
//         try to schedule task
//     else UNLOCK MUTEX/return error
//   else UNLOCK MUTEX/return error
//   UNLOCK MUTEX
static int create_task_impl(sched_t *self, group_id_t group_id, 
                            task_fn_t fn, task_args_t args)
{
  cpu_t *cpu;
  group_t *group;
  task_t *task;
  bool is_in_cpu = false;

  mutex_lock(&(self->group_lock));
  if(is_group_in_unopened_q(self, group_id)){
    mutex_unlock(&(self->group_lock));
    return BAD_GROUP_OP;
  }
  if(is_group_in_dep_wait_q(self, group_id)){
    group = retrieve_group_from_q(&(self->dep_wait_q), group_id);
  } 
  else if(is_group_in_schedule_q(self, group_id)){
    group = retrieve_group_from_q(&(self->schedule_q), group_id);
  }
  else if(is_group_in_a_cpu(self, group_id)){
    ASSERT((cpu = find_cpu_by_group_id(self, group_id)) != NULL, 
           "group not in cpu\n");
    group = cpu->group;
    is_in_cpu = true;
  }
  else if(is_group_retired(self, group_id)){
    mutex_unlock(&(self->group_lock));
    return BAD_GROUP_OP;
  }
  else {
    //group doesn't even exist yet
    mutex_unlock(&(self->group_lock));
    return BAD_GROUP_OP;
  } 
  ASSERT(group != NULL, "group not in queue or cpu\n");
  ASSERT(group->state != GROUP_INIT, "invalid group state\n");
  if(group->state == GROUP_CLOSED){
    mutex_unlock(&(self->group_lock));
    return BAD_GROUP_OP;
  }
  if((task = create_task(self, group, fn, args, &task_retire_func)) == NULL){
    mutex_unlock(&(self->group_lock));
    return TASK_CREATE_FAILED;
  }
  list_insert_tail(&(group->task_q), &(task->q_link));
  if(is_in_cpu){
    try_issue_task(cpu);
  }
  mutex_unlock(&(self->group_lock));
}

//============================================================================
// Task Scheduling event API 
//============================================================================
/*
static void fire_event(sched_t *self, event_t event, uint64_t total)
{
  ASSERT(event < self->num_events, "event greater than num_events\n");
  event_data_t *data = self->events + event;
  ASSERT(data->fired == 0, "event %d already fired\n", event);

  mutex_lock(&(event->lock));
  data->fired = true;
  data->total = total;
  mutex_unlock(&(event_lock));

  DEBUG(DBG_EVENT, "fired event %d with %d total\n", event, total);
}

static bool is_event_fired(sched_t *self, event_t event)
{
  ASSERT(event < self->num_events, "event greater than num_events\n");
  event_data_t *data = self->events + event;

  /**
    * DO NOT lock mutex here while checking. there is only 1 writer, and 
    * it has the lock. so just do passive, lockless reads
    * /
  return data->fired;
}

static void wait_event(sched_t *self, event_t event)
{
  ASSERT(event < self->num_events, "event greater than num_events\n");
  event_data_t *data = self->events + event;

  /**
    * DO NOT lock mutex here while spin waiting. there is only 1 writer, and 
    * it has the lock. so just do passive, lockless reads
    * /
  while(!data->fired){}

  /**
    * TODO: this is a serialization bottleneck, you should look into making
    * this scale better as the number of active waiters grows
    * /
  mutex_lock(&(data->lock));
  if(++data->waiting == total){
    data->fired = false;
    data->total = 0;
    data->waiting = 0;
  }
  mutex_unlock(&(data->lock));
}
*/
//============================================================================
// Task Scheduling cleanup API 
//============================================================================

/**
static void destroy_impl(sched_t *self)
{

}
*/

//============================================================================
// Task Factory
//============================================================================

static void init_event_fields(sched_t *self, uint64_t num_events)
{
  self->num_events = num_events;
  self->events = (event_data_t *)malloc(num_events*sizeof(event_data_t));
  ASSERT(self->events != NULL, "malloc events\n");

  for(uint64_t i=0; i<num_events; ++i){
    mutex_init(&(self->events[i].lock));
    self->events[i].fired = false;
    self->events[i].waiting = 0;
    self->events[i].total = 0;
  }
}


static void init_cpu_fields(sched_t *self)
{
  uint64_t num_cpus;
  uint64_t cur_cpu_id;
  ASSERT(sys_cpu_count(&num_cpus, &cur_cpu_id) == 0, "sys_cpu_count\n");
  ASSERT(num_cpus > 0, "num_cpus < 1\n");
  ASSERT(cur_cpu_id < num_cpus, "cur_cpu_id >= num_cpus\n");
  self->num_cpus = num_cpus;
  self->cpus = (cpu_t *)malloc(num_cpus*sizeof(cpu_t));
  ASSERT(self->cpus != NULL, "malloc cpu array\n");
 
  uint64_t threads;
  uint64_t threads_free;
  for(uint64_t i=0; i<num_cpus; ++i){
    ASSERT(sys_cpu_info(i, &threads, &threads_free) == 0, "sys_cpu_info\n");
    ASSERT(threads_free <= threads, "free > threads for cpu\n");
    self->cpus[i].cpu_id = i;
    self->cpus[i].threads = threads;
    self->cpus[i].threads_free = threads_free;
    self->cpus[i].group = NULL;
    self->cpus[i].occupied = false;
    self->cpus[i].root_cpu = (i == cur_cpu_id);
  }
}

//============================================================================
// Scheduling Framework Initialization
//============================================================================

sched_t * create_sched(event_t num_events)
{
  ASSERT(singleton == NULL, "only 1 sched_framework allowed\n");

  sched_t * self = (sched_t *)malloc(sizeof(sched_t));
  ASSERT(self != NULL, "malloc sched\n");

  init_event_fields(self, num_events);
  init_cpu_fields(self);

  self->last_retired = 0;

  /**
    * create queues for managing the different states and dependencies 
    * of groups. These queues are for GROUPS only.
    */
  mutex_init(&(self->group_lock));
  list_init(&(self->unopened_q));
  list_init(&(self->dep_wait_q));
  list_init(&(self->schedule_q));
  list_init(&(self->retire_q));

  self->create_group    = &create_group_impl;
  self->open_group      = &open_group_impl;
  self->close_group     = &close_group_impl;
  self->create_task     = &create_task_impl;
/*
  fire_event      = &fire_event_impl;
  is_event_fired  = &is_event_fired_impl;
  wait_event      = &wait_event_impl;

  destroy         = &destroy_impl;
*/
  //DEBUG(DBG_SCHED, "sched framework created: %d events and %d cpus\n", 
  //  self->num_events, self->num_cpus);

  return self;
}