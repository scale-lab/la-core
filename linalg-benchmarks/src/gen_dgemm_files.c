
#define NMP_MIN   16
#define NMP_MAX   4096
#define NMP_SCALE 2

#include <sys/stat.h>
#include <sys/types.h>
#include <stdarg.h>
#include <string.h>

//only do lower for now
void do_test(IDX M, IDX P, bool right, bool upper, bool unit, bool trans)
{
  double *A, *B, *B_orig, *gsl_A, *gsl_B, *gsl_B_solved, *gsl_B_orig;
  IDX B_rows = right ? P : M;
  IDX B_cols = right ? M : P;
  bool real_upper = trans ? !upper : upper;
  double alpha = get_scalar_double();

  //algorithm requirement #2 ('right' determines row order and indices)
  if (right) {
    A = create_matrix_trianglular_dc(M, M, upper, unit);
    B = create_matrix_dr(P, M, FILL);
    B_orig = copy_matrix_dr(B, P, M);
  } 
  else {
    A = create_matrix_trianglular_dr(M, M, upper, unit);
    B = create_matrix_dc(M, P, FILL);
    B_orig = copy_matrix_dc(B, M, P);
  }

  double start = getRealTime();
  double *A_actual = trans ? transpose_matrix_dcr(A, M, M) : A;
  do_trsm(A_actual, B, M, B_rows, B_cols, right, real_upper, unit, alpha);
  if (trans) {
    destroy_matrix_d(A_actual);
  }
  double duration = getRealTime() - start;

  double flop = unit ? TRSM_FLOP_UNIT(M, P) : TRSM_FLOP_DIAG(M, P);
  printf("M=%4u P=%4u %s%s%s%s | took % 10.6f secs | ", M, P, 
    (right ? "R" : "L"), (upper ? "U" : "L"), 
    (trans ? "T" : "N"), (unit ? "U" : "D"), duration);
  printf("% 10.3f MFLOP | ", flop/1e6);
  printf("% 10.3f MFLOP/s || ", flop/(duration*1e6));

  if (right) {
    gsl_A = transpose_matrix_dcr(A, M, M);
    gsl_B = copy_matrix_dr(B_orig, P, M);
    gsl_B_orig = copy_matrix_dr(B_orig, P, M);
    gsl_B_solved = copy_matrix_dr(B, P, M);
  }
  else {
    gsl_A = copy_matrix_dr(A, M, M);
    gsl_B = transpose_matrix_dcr(B_orig, M, P);
    gsl_B_orig = transpose_matrix_dcr(B_orig, M, P);
    gsl_B_solved = transpose_matrix_dcr(B, M, P);
  }
  verify_gsl_dtrsm(gsl_A, gsl_B, gsl_B_orig, gsl_B_solved,
    M, P, right, upper, unit, trans, alpha);

  destroy_matrix_d(A);
  destroy_matrix_d(B);
  destroy_matrix_d(B_orig);
  destroy_matrix_d(gsl_A);
  destroy_matrix_d(gsl_B);
  destroy_matrix_d(gsl_B_orig);
}

//http://stackoverflow.com/questions/2336242/
static void recursive_mkdir(const char *dir) {
  char tmp[256];
  char *p = NULL;
  size_t len;

  snprintf(tmp, sizeof(tmp),"%s",dir);
  len = strlen(tmp);
  if(tmp[len-1] == '/'){
    tmp[len -1] = 0;
  }
  for(p = tmp + 1; *p; p++){
    if(*p == '/') {
      *p = 0;
      mkdir(tmp, S_IRWXU);
      *p = '/';
    }
  }
  mkdir(tmp, S_IRWXU);
}

// C := alpha*op(A)*op(B) + beta*C

// sweep (a, b, c) sizes
// sweep op(A)
// sweep op(B)

int main(int argc, char **argv) {
  if(argc !=2 ){
    PRINT("usage: %s <output directory>\n", argv[0]);
    return 0;
  }

  char *dir_name = argv[1];
  recursive_mkdir(argv[1]);

  for (IDX NMP = NMP_MIN; NMP <= NMP_MAX; NMP *= NMP_SCALE) {
    generate(NMP, TRANS,     TRANS);
    generate(NMP, TRANS,     NOT_TRANS);
    generate(NMP, NOT_TRANS, TRANS);
    generate(NMP, NOT_TRANS, NOT_TRANS);
  }
  
  return 0;
}
