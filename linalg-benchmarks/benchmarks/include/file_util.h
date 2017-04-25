//==========================================================================
// Copyright (c) 2017 Samuel Steffl
// All rights reserved.
//==========================================================================

#include <stdio.h>

#include "lib/matrix_util.h"

//reads matrix M from file in whatever row/column order the file is in
int read_matrix_from_file_d(const char *filename, IDX h, IDX, w, double *M);

//writes matrix M to file in whatever row/column order M is in
int write_matrix_to_file_d(const char *filename, IDX h, IDX, w, double *M);