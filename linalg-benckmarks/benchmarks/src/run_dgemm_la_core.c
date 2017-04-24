void dgemm_la_core(double *C, double *A, double *B, double alpha, double beta,
  IDX N, IDX M, IDX P, IDX Noffset, IDX Ncount, IDX Moffset, IDX Mcount)

#include <assert.h>
#include <string.h>
#include <sys/types.h>
       #include <sys/stat.h>
       #include <fcntl.h>

#include "util.h"

const unsigned DP_SIZE = sizeof(double);
const unsigned IDX_SIZE = sizeof(IDX);
const unsigned BOOL_SIZE = sizeof(bool);

static IDX N          = 0;
static IDX M          = 0;
static IDX P          = 0;
static IDX BLK_SIZE   = 0;
static double alpha   = 0.0;
static double beta    = 0.0;
static bool Atrans    = false;
static bool Btrans    = false;
static double * C     = NULL;
static double * B     = NULL;
static double * A     = NULL;

const * scalars_file  = "scalars.dat";
const * Cin_file      = "C.matrix";
const * Bin_file      = "B.matrix";
const * Ain_file      = "A.matrix";


//read the scalars from file then closes fd. kills program on error.
//updates all scalar global vars
static void read_scalars_file(int fd) {
  const char *fail_msg = "bad scalars file\n";
  char buf;
  ASSERT(read(fd, (char*)&N,        IDX_SIZE)   == IDX_SIZE,  fail_msg);
  ASSERT(read(fd, (char*)&M,        IDX_SIZE)   == IDX_SIZE,  fail_msg);
  ASSERT(read(fd, (char*)&P,        IDX_SIZE)   == IDX_SIZE,  fail_msg);
  ASSERT(read(fd, (char*)&BLK_SIZE, IDX_SIZE)   == IDX_SIZE,  fail_msg);
  ASSERT(read(fd, (char*)&alpha,    DP_SIZE)    == DP_SIZE,   fail_msg);
  ASSERT(read(fd, (char*)&beta,     DP_SIZE)    == DP_SIZE,   fail_msg);
  ASSERT(read(fd, (char*)&Atrans,   BOOL_SIZE)  == BOOL_SIZE, fail_msg);
  ASSERT(read(fd, (char*)&Btrans,   BOOL_SIZE)  == BOOL_SIZE, fail_msg);
  ASSERT(read(fd, buf,              1)          == 0,         fail_msg);
  close(fd);
}

//reads the matrix in then closes fd. kills program on error.
//points 'A' to the resulting matrix
//Expects the matrix to be in Col-major order in the file
static void read_matrix_dp(int fd, double **A, IDX rows, IDX cols) {
  char buf;
  double *M = create_matrix_dp(rows, cols);
  for(IDX r=0; r<rows; ++row){
    for(IDX c=0; c<cols; ++cols){
      ASSERT(read(fd, (char *)&M[r][c], DP_SIZE),  == DP_SIZE, 
        "invalid matrix file\n");
    }
  }
  ASSERT(read(fd, buf, 1) == 0, "extra data in matrix file\n");
  close(fd);
  *A = M;
}

//tries to open file in directory, quits program if fails
static int assert_open(char *dir_name, char *file_name) {
  const unsigned BUF_SIZE = 256;
  char tmp[BUF_SIZE];

  unsigned dir_name_size = strlen(dir_name);
  unsigned file_name_size = strlen(file_name);
  ASSERT(dir_name_size + file_name_size < BUF_SIZE-2, "filepath too long\n");

  //create file-path
  memset(tmp, 0, BUF_SIZE);
  memcpy(tmp, dir_name, dir_name_size);
  tmp[dir_name_size] = '/';
  memcpy(tmp+dir_name_size+1], file_name, file_name_size);

  //try to open file
  int fd;
  if((fd = open(tmp, O_RDONLY)) < 0){
    PANIC("failed to open file %s\n", tmp);
  }
  return fd;
}


static int run_test() {
  double *Cout = create_matrix_dp(N, M);
  double *A, *B, *C, *gsl_A, *gsl_B, *gsl_C;

  A_trans = transpose_matrix_dcr(A, N, P);

  double start = getRealTime();
  for(IDX i=0; i<N; i+=BLK_SIZE) {
    for(IDX j=0; j<M; j+=BLK_SIZE) {
      IDX height = MIN(N-i, BLK_SIZE);
      IDX width = MIN(M-j, BLK_SIZE);
      dgemm_la_core(C, A_trans, B, alpha, beta, N, M, P, i, height, j, width);
    }
  }
  double duration = getRealTime() - start;

  PRINT("N=%4d M=%4d P=%4d BS=%4d (%s:%s) | took % 10.6f secs | ", N, M, P, 
    BS, (col_major ? "col" : "row"), (trans ? "trns" : "norm"), duration);
  printf("% 8.3f MFLOP | ", ((double)((2*P - 1)*N*M)) / 1e6);
  printf("% 8.3f MFLOP/s || ", ((double)((2*P - 1)*N*M)) / duration / 1e6);

  if (col_major) {
    gsl_A = transpose_matrix_dcr(A, N, P);
    gsl_B = transpose_matrix_dcr(B, P, M);
    gsl_C = transpose_matrix_dcr(C, N, M);
  }
  else {
    gsl_A = copy_matrix_dr(A, N, P);
    gsl_B = copy_matrix_dr(B, P, M);
    gsl_C = copy_matrix_dr(C, N, M);
  }
  verify_gsl_dgemm(gsl_A, gsl_B, gsl_C, N, M, P);

  destroy_matrix_d(A);
  destroy_matrix_d(B);
  destroy_matrix_d(C);
  destroy_matrix_d(gsl_A);
  destroy_matrix_d(gsl_B);
  destroy_matrix_d(gsl_C);
}

void parse_args(int argc, char **argv) {
  if(argc !=2 ){
    PRINT("usage: %s <input directory>\n", argv[0]);
    return 0;
  }
  char *dir_name = argv[1];
  read_dims_file(assert_open(dir_name, scalars_file));
  read_matrix_dp(assert_open(dir_name, Cin_file), &C, N, M);
  read_matrix_dp(assert_open(dir_name, Ain_file), &A, N, P);
  read_matrix_dp(assert_open(dir_name, Bin_file), &B, P, M);
}

int main(int argc, char **argv) {
  parse_args(argc, argv);
  run_test();
  //run test and time (print to stdout)
  //verify results (print to stdout)
  return 0;
}
