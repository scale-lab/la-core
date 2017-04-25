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
  * Synchnonization Primitives Test
  *
  * Target platform is the MinorCPU in gem5 modified with ThreadScheduling
  * 
  * This test will create 1 task group, and schedule 1 task to it. Then it 
  * will rigorously test the customized spinlock and mutex Thread Scheduling
  * RISC-V ISA primitives using the main thread and the task thread.
  */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "debug/dbg.h"
#include "sched/framework.h"
#include "synch/mutex.h"
#include "util/util.h"

//============================================================================
// Test Structs, Macros, Constants
//============================================================================

#define NUM_EVENTS 20

#define GROUP_1 1

typedef struct {
  mutex_t startup_lock;
  mutex_t testing_lock;
  volatile uint64_t counter;
  volatile bool ready;
}
targs_t;

//============================================================================
// Test Tasks
//============================================================================

void task_func(void *args){
  targs_t *targs = (targs_t *)args;

  DEBUG(DBG_TEST, "inside task_func/n");

  mutex_lock(&(targs->startup_lock));
  targs->ready = true;
  mutex_unlock(&(targs->startup_lock));

  for(int i=0; i<100; ++i){
    mutex_lock(&(targs->testing_lock));
    ASSERT(targs->counter == i+2, 
      "test failed: invalid counter. expected %d got %d\n",
      i, targs->counter);
    targs->counter -= 1;
    mutex_unlock(&(targs->testing_lock));

    //busy idle
    for(int j=0; j<20; ++j);
  }

  DEBUG(DBG_TEST, "task exiting successfully/n");
}

//============================================================================
// Test Main
//============================================================================

void main(int argc, void **argv)
{
  DEBUG(DBG_TEST, "main entered\n");

  sched_t * sched = create_sched(NUM_EVENTS);

  sched->create_group(sched, GROUP_1, NULL, 0);
  sched->open_group(sched, GROUP_1);

  targs_t *targs = (targs_t*)malloc(sizeof(targs_t));
  ASSERT(targs != NULL, "malloc failed\n");

  mutex_init(&(targs->startup_lock));
  mutex_init(&(targs->testing_lock));
  targs->counter = 0;
  targs->ready = false;

  //LOCK LOCK2!
  mutex_lock(&(targs->testing_lock));

  sched->create_task(sched, GROUP_1, &task_func, &targs);
  sched->close_group(sched, GROUP_1);

  DEBUG(DBG_TEST, "task scheduled, group closed/n");

  bool ready = false;
  while(!ready){
    mutex_lock(&(targs->startup_lock));
    if(targs->ready){
      ready = true;
    }
    mutex_unlock(&(targs->startup_lock));
  }

  DEBUG(DBG_TEST, "task is ready, test starting/n");

  for(int i=0; i<100; ++i){
    ASSERT(targs->counter == i, 
      "test failed: invalid counter. expected %d got %d\n",
      i, targs->counter);
    targs->counter += 2;
    mutex_unlock(&(targs->testing_lock));
    //busy idle
    for(int j=0; j<20; ++j);
    mutex_lock(&(targs->testing_lock));
  }

  //cleanup
  //sched->destroy(sched);

  DEBUG(DBG_TEST, "test passed\n");
  return 0;
}