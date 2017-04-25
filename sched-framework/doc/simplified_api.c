
int main(int argc, void **argv){

  initialize();

  create_group(Group1, {}, 0);
  create_group(Group2, {}, 0);
  create_group(Group3, {Group1, Group2});

  open_group(Group2);
  create_task(Group2, taskFunc0, taskArgs0);

  open_group(Group1);
  create_task(Group1, taskFunc0, taskArgs0);
  create_task(Group1, taskFunc1, taskArgs1);
  close_group(Group1);

  create_task(Group2, taskFunc1, taskArgs1);
  close_group(Group2);
  close_group(Group3);

  waitForTasksToFinish();
  cleanup();

  return 0;
}



void taskFunc0(void *args){

  //synchronize with other tasks
  wait_for_event(DATA_IN_0_READY);

  //1 consumer of this event
  compute_with_data();
  fire_event(DATA_1_IN_READY, 1);

  //10 consumers of this event
  compute_more_with_data();
  fire_event(DATA_OUT_0_READY, 10);
}