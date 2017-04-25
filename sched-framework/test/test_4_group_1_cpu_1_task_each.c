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
#include "synch/mutex.h"
#include <stdint.h>

#include "sched/group.h"

typedef struct {
  uint8_t col;
  uint8_t row;
  uint16_t rows;
  uint16_t rows;

  float * dou

} mult_args_t;

void reduce_fn(task_args_t args){
  uint64_t real = (uint64_t) args;

  uint64_t row = (real >> 32);
  uint64_t col = (real & 0xffffffff);
}


#define NUM_EVENTS 20

void main(int argc, void **argv)
{
  sched_t * sched = sched_init(NUM_EVENTS);

  test0(sched);
  test1(sched);
  test2(sched);
  test3(sched);
  test4(sched);
  test5(sched);
  test6(sched);
  test7(sched);

  sched_create_group(0);
  sched_create_group(1);
  sched_create_group(2);

  sched_set_dependency(0, 1);
  sched_set_dependency(0, 2);
  sched_set_dependency(1, 2);

  sched_open_group(0);
  sched_open_group(1);
  sched_open_group(10);
  sched_open_group(1000);

  mult_args_t *margs = (mult_args_t*)malloc(WORKERS*

  sched_create_task(0, &group0_func, 

  return 0;
}


tests to run 

// can create 1 group on 1 cpus, open it up, schedule 1 task then close/exit
  // test that task_clone working for all the HW threads

// can create 1 group on 1 cpus, open it up, schedule N tasks, close it, then 
// wait until all the tasks are finished to exit
  // test that task_clone and task_swap both working for all the HW threads

// can create 4 groups, on 1 cpus, and make sure the correct one is issued first
// regardless of when tasks are created
  // test that task_clone and task_swap both working for all the HW threads

// create 4 groups on 2 cpus, and make sure ordering is preserved

// create 4 groups on 2 cpus, and make sure that group not swapped until
// previous group closed

// events

// single group/ 2 tasks, one fire event, the other wait for event, then one
// the other waits and the other fires.

// 2 groups on diff cores, 1 fire, 100 receivers, then 100 fires, and 1 receiver
// before exit

// 3 groups, 1 fire, 2 other grps listen, but only 2 grps at a time. grp 2 
// finishes, then grp 3 runs, and grp 1 waits unti 2 and 3 both done

