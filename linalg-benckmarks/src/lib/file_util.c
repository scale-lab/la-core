//==========================================================================
// Copyright (c) 2017 Samuel Steffl
// All rights reserved.
//==========================================================================

#include "lib/file_util.h"

#include <stdio.h>

#include "lib/matrix_util.h"

int read_matrix_from_file_d(const char *filename, IDX h, IDX, w, double *M) 
{
  int fd;
  if((fd = open(filename, O_RDONLY)) < 0){
    return fd;
  }

  int bytes;
  int bytes_read = 0;
  int bytes_left = sizeof(double)*w*h;

  while(bytes_left){
    if((bytes = read(fd, ((char *)M) + bytes_read, bytes_left)) < 0){
      close(fd);
      return bytes;
    }
    bytes_left -= bytes;
    bytes_read += bytes;
  }
  close(fd);

  return 0;
}

int write_matrix_to_file_d(const char *filename, IDX h, IDX, w, double *M) 
{
  int fd;
  if((fd = open(filename, O_WRONLY)) < 0){
    return fd;
  }

  int bytes_written = 0;
  int bytes_left = sizeof(double)*w*h;

  while(bytes_left){
    int bytes = write(fd, ((char *)M) + bytes_written, bytes_left);
    if(bytes < 0){
      close(fd);
      return bytes;
    }
    bytes_left -= bytes;
    bytes_written += bytes;
  }
  close(fd);

  return 0;
}