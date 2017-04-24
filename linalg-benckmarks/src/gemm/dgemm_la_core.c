//==========================================================================
// Copyright (c) 2017 Samuel Steffl
// All rights reserved.
// 
// LACore implementation of BLAS-3 DTRSM function
//==========================================================================

#include "gemm/dgemm_la_core.h"

#include "util/matrix_utilities.h"

#include "la_core/la_core.h"
#include "la_core/la_set_scalar.h"
#include "la_core/la_set_vec.h"
#include "la_core/la_copy.h"
#include "la_core/la_data_op.h"

//NOTE: right now assuming that a full row/column of either matrix can fit 
//      into the scratchpad! At a 64 KB scratchpad, this would limit us to 
//      around 4000x4000 element matrices. Clearly, we can modify the algorithm
//      to use chunks in the future
static void dgemm_kernel(double *C, double *A, double *B, 
  double alpha, double beta,
  IDX N, IDX M, IDX P, IDX Noffset, IDX Ncount, IDX Moffset, IDX Mcount)
{
  IDX i_count = Ncount;
  IDX j_start = Moffset;
  IDX j_end   = Moffset + Mcount;
  IDX j_count = Mcount;
  IDX k_count = P;

  //what registers to use for constants
  const LaRegIdx Zero_REG         = 0;
  const LaRegIdx One_REG          = 1;
  const LaRegIdx Alpha_REG        = 2;
  const LaRegIdx Beta_REG         = 3;

  //registers for input and vector configs
  const LaRegIdx Cin_mem_REG      = 4;
  const LaRegIdx Cin_sch_REG      = 5;
  const LaRegIdx Bin_mem_REG      = 6;
  const LaRegIdx Bin_sch_REG      = 7;

  //registers for intermediate vector configs
  const LaRegIdx Ain_mem_REG      = 4;
  const LaRegIdx Ain_Bin_sch_REG  = 6;

  //registers for output vector configs
  const LaRegIdx Cout_mem_REG     = 4;
  
  //memory start addresses in the scratchpad for intermediate results
  const LaAddr CIN_SCH_ADDR      = 0;
  const LaAddr BIN_SCH_ADDR      = CIN_SCH_ADDR + i_count*sizeof(double);
  const LaAddr AIN_BIN_SCH_ADDR  = BIN_SCH_ADDR;

  //CONFIG: load some constants into LAcore registers
  la_set_scalar_dp_reg(Zero_REG,  0.0);
  la_set_scalar_dp_reg(One_REG,   1.0);
  la_set_scalar_dp_reg(Alpha_REG, alpha);
  la_set_scalar_dp_reg(Beta_REG,  beta);

  for(IDX j=j_start; j<j_end; ++j){
    LaAddr Cin  = (LaAddr) &(IDXC(C, i_start, j, N, P));
    LaAddr Ain  = (LaAddr) &(IDXR(A, j,       0, N, P));
    LaAddr Bin  = (LaAddr) &(IDXC(B, 0,       j, N, P));
    LaAddr Cout = Cin;

    //CONFIG:
    la_set_vec_adrcnt_dp_mem(Cin_mem_REG, Cin,             i_count);
    la_set_vec_adrcnt_dp_sch(Cin_sch_REG, CIN_SCH_ADDR,    i_count);
    la_set_vec_adrcnt_dp_mem(Bin_mem_REG, Bin,             k_count);
    la_set_vec_dp_sch(       Bin_sch_REG, BIN_SCH_ADDR, 1, k_count, -k_count);

    //EXEC: Cin_sch = (Cin_mem + 0)*beta
    //EXEC: Bin_sch = (Bin_mem + 0)*alpha
    la_AaddBmulC(Cin_sch_REG, Cin_mem_REG, Zero_REG, Beta_REG,  i_count);
    la_AaddBmulC(Bin_sch_REG, Bin_mem_REG, Zero_REG, Alpha_REG, k_count);

    //CONFIG:
    la_set_vec_adrcnt_dp_mem(Ain_mem_REG,     Ain,              k_count);
    la_set_vec_adrcnt_dp_sch(Ain_Bin_sch_REG, AIN_BIN_SCH_ADDR, i_count);

    //EXEC: Ain_Bin_sch = (Bin_sch + 0)*Ain_mem (i_count dotprods of k_count)
    la_AaddBmulC_sum_multi(Ain_Bin_sch_REG, Bin_sch_REG, Zero_REG, Ain_mem_REG, 
                           (i_count*k_count));

    //CONFIG:
    la_set_vec_dp_mem(Cout_mem_REG, Cout, i_count);

    //EXEC: Cout_mem = (Cin_sch + Ain_Bin_sch)*1
    la_AaddBmulC(Cout_mem_REG, Cin_sch_REG, Ain_Bin_sch_REG, One_REG, i_count);
  }
}


// C := alpha*op(A)*op(B) + beta*C
// assuming the transpositions were done beforehand
// C = col-major, A = row-major, B=col-major
void dgemm_la_core(double *C, double *A, double *B, double alpha, double beta,
  IDX N, IDX M, IDX P, IDX Noffset, IDX Ncount, IDX Moffset, IDX Mcount)
{
  IDX Mend = (Moffset + Mcount);
  IDX Nend = (Noffset + Ncount);
  for (IDX j = Moffset; j<Mend; j += BLOCK_D) {
    for (IDX i = Noffset; i<Nend; i += BLOCK_D) {
      IDX icount = MIN(BLOCK_D, (Nend - i));
      IDX jcount = MIN(BLOCK_D, (Mend - j));
      dgemm_kernel(C, A, B, alpha, beta, N, M, P, i, icount, j, jcount);
  }
}