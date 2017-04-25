//==========================================================================
// Copyright (c) 2017 Samuel Steffl
// All rights reserved.
// 
// RISC-V LACore implementation of BLAS-3 DTRSM function
//
// 4 variations are provided for the purpose of design space exploration. 2 use 
// the scratchpad and 2 dont. 2 use a panelling approach while 2 use a 
// striping approach
//
// C := alpha*op(A)*op(B) + beta*C
// assuming the transpositions were done beforehand
// C = col-major, A = row-major, B=col-major
//
//==========================================================================

#include "gemm/dgemm_la_core.h"

#include <assert.h>
#include <stdbool.h>

#include "la_core/la_core.h"
#include "la_core/la_set_scalar.h"
#include "la_core/la_set_vec.h"
#include "la_core/la_copy.h"
#include "la_core/la_data_op.h"
#include "matrix_types.h"
#include "util.h"

extern bool     DEBUG;
extern uint64_t SCRATCH_SIZE;
extern bool     USE_SCRATCH;
extern bool     USE_PANEL;

//==========================================================================

static void dgemm_la_core_scratch_panel(double *C, double *A, double *B, 
  double alpha, double beta,
  IDX N, IDX M, IDX P, IDX Noffset, IDX Ncount, IDX Moffset, IDX Mcount)
{
  IDX istart = Noffset;
  IDX iend   = Noffset + Ncount;
  IDX jstart = Moffset;
  IDX jend   = Moffset + Mcount;

  //for a 2^16 scratchpad, K=504, so pretty decent
  const IDX SIZE_DP = sizeof(double);
  const IDX I_BLOCK = 16;
  const IDX J_BLOCK = I_BLOCK;
  const IDX K_BLOCK = SCRATCH_SIZE/(2*I_BLOCK*SIZE_DP) - I_BLOCK;

  assert(K_BLOCK != 0);

  const LaAddr CIN_SCH_ADDR     = 0;
  const LaAddr AIN_BIN_SCH_ADDR = CIN_SCH_ADDR + I_BLOCK*J_BLOCK*SIZE_DP;
  const LaAddr AIN_SCH_ADDR     = AIN_BIN_SCH_ADDR + I_BLOCK*J_BLOCK*SIZE_DP;
  const LaAddr BIN_SCH_ADDR     = AIN_SCH_ADDR + K_BLOCK*I_BLOCK*SIZE_DP;

  //what registers to use for constants (0.0, 1.0, alpha, beta)
  const LaRegIdx Zero_REG             = 0;
  const LaRegIdx One_REG              = 1;
  const LaRegIdx Alpha_REG            = 1;
  const LaRegIdx Beta_div_Alpha_REG   = 1;

  //registers for input and vector configs
  const LaRegIdx Cin_sch_REG          = 2;
  const LaRegIdx Cin_mem_REG          = 3;
  const LaRegIdx Bin_sch_REG          = 4;
  const LaRegIdx Bin_mem_REG          = 5;
  const LaRegIdx Ain_sch_REG          = 6;
  const LaRegIdx Ain_mem_REG          = 4;
  const LaRegIdx Ain_Bin_sch_REG      = 7;


  la_set_scalar_dp_reg(Zero_REG, 0.0);
  for (IDX j = jstart; j<jend; j+=J_BLOCK) {
    IDX jcount = MIN(J_BLOCK, (jend-j));

    for (IDX i = istart; i<iend; i+=I_BLOCK) {
      IDX icount = MIN(I_BLOCK, (iend-i));
      IDX ijcount = icount*jcount;
      LaAddr Cin = (LaAddr) &(IDXC(C, i, j, N, M));

      //load the C-elements into scratchpad
      la_set_vec_dp_mem(Cin_mem_REG, Cin, 1, icount, (N-icount));
      la_set_vec_adr_dp_sch(Cin_sch_REG, CIN_SCH_ADDR);
      la_set_scalar_dp_reg(Beta_div_Alpha_REG, beta/alpha);
      la_AaddBmulC(Cin_sch_REG, 
                   Cin_mem_REG, Zero_REG, Beta_div_Alpha_REG, ijcount);

      for(IDX k=0; k<P; k += K_BLOCK){
        IDX kcount = MIN(K_BLOCK, (P-k));
        IDX ikcount = icount*kcount;
        IDX jkcount = jcount*kcount;
        LaAddr Ain = (LaAddr) &(IDXR(A, i, k, N, P));
        LaAddr Bin = (LaAddr) &(IDXC(B, k, j, P, M));

        //load A and B panels into scratchpad
        la_set_vec_dp_mem(Bin_mem_REG, Bin, 1, kcount, (P-kcount));
        la_set_vec_adr_dp_sch(Bin_sch_REG, BIN_SCH_ADDR);
        la_copy(Bin_sch_REG, Bin_mem_REG, jkcount);

        la_set_vec_dp_mem(Ain_mem_REG, Ain, 1, kcount, (P-kcount));
        la_set_vec_adr_dp_sch(Ain_sch_REG, AIN_SCH_ADDR);
        la_copy(Ain_sch_REG, Ain_mem_REG, ikcount);

        //work on all the A and B elements in the scratchpad
        LaAddr Boff = 0;
        LaAddr Toff = 0;
        for(IDX jj=0; jj<jcount; ++jj){
          la_set_vec_dp_sch(Bin_sch_REG, BIN_SCH_ADDR+Boff, 1, kcount, -kcount);
          la_set_vec_dp_sch(Ain_sch_REG, AIN_SCH_ADDR, 1, kcount, 0);
          la_set_vec_dp_sch(Ain_Bin_sch_REG, 
                            AIN_BIN_SCH_ADDR+Toff, 1, icount, 0);
          la_AaddBmulC_sum_multi(Ain_Bin_sch_REG,
                                 Ain_sch_REG, Zero_REG, Bin_sch_REG, ikcount);
          Boff += kcount*SIZE_DP;
          Toff += icount*SIZE_DP;
        }

        //update the C-elements
        la_set_vec_dp_sch(Ain_Bin_sch_REG, AIN_BIN_SCH_ADDR, 1, icount, 0);
        if(k+kcount == P){
          //finally, write back C-elements to main memory
          la_set_scalar_dp_reg(Alpha_REG, alpha);
          la_AaddBmulC(Cin_mem_REG, 
                       Cin_sch_REG, Ain_Bin_sch_REG, Alpha_REG, ijcount);
        } else {
          la_set_scalar_dp_reg(One_REG, 1.0);
          la_AaddBmulC(Cin_sch_REG,
                       Cin_sch_REG, Ain_Bin_sch_REG, One_REG, ijcount);
        }
      }
    }
  }
}

//==========================================================================

static void dgemm_la_core_scratch_stripe(double *C, double *A, double *B, 
  double alpha, double beta,
  IDX N, IDX M, IDX P, IDX Noffset, IDX Ncount, IDX Moffset, IDX Mcount)
{
  IDX istart = Noffset;
  IDX iend   = Noffset + Ncount;
  IDX jstart = Moffset;
  IDX jend   = Moffset + Mcount;

  //for a 2^16 scratchpad, K=2730
  const IDX SIZE_DP = sizeof(double);
  const IDX I_BLOCK = SCRATCH_SIZE/(SIZE_DP*3);
  const IDX K_BLOCK = SCRATCH_SIZE/(SIZE_DP*3);

  const LaAddr CIN_SCH_ADDR     = 0;
  const LaAddr AIN_BIN_SCH_ADDR = CIN_SCH_ADDR + I_BLOCK*SIZE_DP;
  const LaAddr BIN_SCH_ADDR     = AIN_BIN_SCH_ADDR + K_BLOCK*SIZE_DP;

  //what registers to use for constants (0.0, 1.0, alpha, beta)
  const LaRegIdx Zero_REG             = 0;
  const LaRegIdx One_REG              = 1;
  const LaRegIdx Alpha_REG            = 1;
  const LaRegIdx Beta_div_Alpha_REG   = 1;

  //registers for input and vector configs
  const LaRegIdx Cin_sch_REG          = 2;
  const LaRegIdx Cin_mem_REG          = 3;
  const LaRegIdx Bin_sch_REG          = 4;
  const LaRegIdx Bin_mem_REG          = 5;
  const LaRegIdx Ain_mem_REG          = 6;
  const LaRegIdx Ain_Bin_sch_REG      = 7;


  la_set_scalar_dp_reg(Zero_REG, 0.0);
  for (IDX j = jstart; j<jend; ++j) {
    for (IDX i = istart; i<iend; i+=I_BLOCK) {
      IDX icount = MIN(I_BLOCK, (iend-i));
      LaAddr Cin = (LaAddr) &(IDXC(C, i, j, N, M));

      //load the C-elements into scratchpad
      la_set_vec_adr_dp_mem(Cin_mem_REG, Cin);
      la_set_vec_adr_dp_sch(Cin_sch_REG, CIN_SCH_ADDR);
      la_set_scalar_dp_reg(Beta_div_Alpha_REG, beta/alpha);
      la_AaddBmulC(Cin_sch_REG, 
                   Cin_mem_REG, Zero_REG, Beta_div_Alpha_REG, icount);

      for(IDX k=0; k<P; k += K_BLOCK){
        IDX kcount = MIN(K_BLOCK, (P-k));
        IDX ikcount = icount*kcount;
        LaAddr Ain = (LaAddr) &(IDXR(A, i, k, N, P));
        LaAddr Bin = (LaAddr) &(IDXC(B, k, j, P, M));

        //load B panel into scratchpad
        la_set_vec_adr_dp_mem(Bin_mem_REG, Bin);
        la_set_vec_adr_dp_sch(Bin_sch_REG, BIN_SCH_ADDR);
        la_copy(Bin_sch_REG, Bin_mem_REG, kcount);

        //work on all the A and B elements to make tmp C stride
        la_set_vec_dp_sch(Bin_sch_REG, BIN_SCH_ADDR, 1, kcount, -kcount);
        la_set_vec_dp_mem(Ain_mem_REG, Ain, 1, kcount, (P-kcount));
        la_set_vec_adrcnt_dp_sch(Ain_Bin_sch_REG, AIN_BIN_SCH_ADDR, icount);
        la_AaddBmulC_sum_multi(Ain_Bin_sch_REG,
                               Ain_mem_REG, Zero_REG, Bin_sch_REG, ikcount);

        //update the C-elements
        if(k+kcount == P){
          //finally, write back C-elements to main memory
          la_set_scalar_dp_reg(Alpha_REG, alpha);
          la_AaddBmulC(Cin_mem_REG, 
                       Cin_sch_REG, Ain_Bin_sch_REG, Alpha_REG, icount);
        } else {
          la_set_scalar_dp_reg(One_REG, 1.0);
          la_AaddBmulC(Cin_sch_REG,
                       Cin_sch_REG, Ain_Bin_sch_REG, One_REG, icount);
        }
      }
    }
  }
}

//==========================================================================

static const IDX BLOCK = 64;
static double tmp_buf[64*64];

//==========================================================================

static void dgemm_la_core_no_scratch_panel(double *C, double *A, double *B, 
  double alpha, double beta,
  IDX N, IDX M, IDX P, IDX Noffset, IDX Ncount, IDX Moffset, IDX Mcount)
{
  IDX istart = Noffset;
  IDX iend   = Noffset + Ncount;
  IDX jstart = Moffset;
  IDX jend   = Moffset + Mcount;

  //reasonable block size for not destroying the L1 cache
  const IDX SIZE_DP = sizeof(double);

  //what registers to use for constants (0.0, 1.0, alpha, beta)
  const LaRegIdx Zero_REG             = 0;
  const LaRegIdx One_REG              = 1;
  const LaRegIdx Alpha_REG            = 2;
  const LaRegIdx Beta_div_Alpha_REG   = 3;

  //registers for input and vector configs
  const LaRegIdx Cin_mem_REG          = 4;
  const LaRegIdx Bin_mem_REG          = 5;
  const LaRegIdx Ain_mem_REG          = 6;
  const LaRegIdx Ain_Bin_mem_REG      = 7;

  la_set_scalar_dp_reg(Zero_REG, 0.0);
  la_set_scalar_dp_reg(One_REG, 1.0);
  la_set_scalar_dp_reg(Alpha_REG, alpha);

  for (IDX j = jstart; j<jend; j+=BLOCK) {
    IDX jcount = MIN(BLOCK, (jend-j));

    for (IDX i = istart; i<iend; i+=BLOCK) {
      IDX icount = MIN(BLOCK, (iend-i));
      IDX ijcount = icount*jcount;
      LaAddr Cin = (LaAddr) &(IDXC(C, i, j, N, M));

      //Multiply C in place by beta/alpha
      la_set_vec_dp_mem(Cin_mem_REG, Cin, 1, icount, (N-icount));
      la_set_scalar_dp_reg(Beta_div_Alpha_REG, beta/alpha);
      la_AaddBmulC(Cin_mem_REG, 
                   Cin_mem_REG, Zero_REG, Beta_div_Alpha_REG, ijcount);

      for(IDX k=0; k<P; k += BLOCK){
        IDX kcount = MIN(BLOCK, (P-k));
        IDX ikcount = icount*kcount;
        LaAddr Ain = (LaAddr) &(IDXR(A, i, k, N, P));
        LaAddr Bin = (LaAddr) &(IDXC(B, k, j, P, M));
        LaAddr Ain_Bin = (LaAddr) tmp_buf;

        //work on all the A and B elements in the scratchpad
        LaAddr Boff = 0;
        LaAddr Toff = 0;
        for(IDX jj=0; jj<jcount; ++jj){
          la_set_vec_dp_mem(Bin_mem_REG, Bin+Boff, 1, kcount, -kcount);
          la_set_vec_dp_mem(Ain_mem_REG, Ain, 1, kcount, (P-kcount));
          la_set_vec_dp_mem(Ain_Bin_mem_REG, Ain_Bin+Toff, 1, icount, 0);
          la_AaddBmulC_sum_multi(Ain_Bin_mem_REG,
                                 Ain_mem_REG, Zero_REG, Bin_mem_REG, ikcount);
          Boff += P*SIZE_DP;
          Toff += icount*SIZE_DP;
        }

        //update the C-elements
        la_set_vec_dp_mem(Ain_Bin_mem_REG, Ain_Bin, 1, icount, 0);
        la_AaddBmulC(Cin_mem_REG, 
          Cin_mem_REG, Ain_Bin_mem_REG, (k+kcount==P ? Alpha_REG : One_REG),
          ijcount);
      }
    }
  }
}

//==========================================================================

static void dgemm_la_core_no_scratch_stripe(double *C, double *A, double *B, 
  double alpha, double beta,
  IDX N, IDX M, IDX P, IDX Noffset, IDX Ncount, IDX Moffset, IDX Mcount)
{
  IDX istart = Noffset;
  IDX iend   = Noffset + Ncount;
  IDX jstart = Moffset;
  IDX jend   = Moffset + Mcount;

  //what registers to use for constants (0.0, 1.0, alpha, beta)
  const LaRegIdx Zero_REG             = 0;
  const LaRegIdx One_REG              = 1;
  const LaRegIdx Alpha_REG            = 2;
  const LaRegIdx Beta_div_Alpha_REG   = 3;

  //registers for input and vector configs
  const LaRegIdx Cin_mem_REG          = 4;
  const LaRegIdx Bin_mem_REG          = 5;
  const LaRegIdx Ain_mem_REG          = 6;
  const LaRegIdx Ain_Bin_mem_REG      = 7;

  la_set_scalar_dp_reg(Zero_REG, 0.0);
  la_set_scalar_dp_reg(One_REG, 1.0);
  la_set_scalar_dp_reg(Alpha_REG, alpha);

  for (IDX j = jstart; j<jend; ++j) {
    for (IDX i = istart; i<iend; i+=BLOCK) {
      IDX icount = MIN(BLOCK, (iend-i));
      LaAddr Cin = (LaAddr) &(IDXC(C, i, j, N, M));

      //Multiply C in place by beta/alpha
      la_set_vec_adr_dp_mem(Cin_mem_REG, Cin);
      la_set_scalar_dp_reg(Beta_div_Alpha_REG, beta/alpha);
      la_AaddBmulC(Cin_mem_REG, 
                   Cin_mem_REG, Zero_REG, Beta_div_Alpha_REG, icount);

      for(IDX k=0; k<P; k += BLOCK){
        IDX kcount = MIN(BLOCK, (P-k));
        IDX ikcount = icount*kcount;
        LaAddr Ain = (LaAddr) &(IDXR(A, i, k, N, P));
        LaAddr Bin = (LaAddr) &(IDXC(B, k, j, P, M));
        LaAddr Ain_Bin = (LaAddr) tmp_buf;

        //work on all the A and B elements to make tmp C stride
        la_set_vec_dp_mem(Bin_mem_REG, Bin, 1, kcount, -kcount);
        la_set_vec_dp_mem(Ain_mem_REG, Ain, 1, kcount, (P-kcount));
        la_set_vec_dp_mem(Ain_Bin_mem_REG, Ain_Bin, 1, icount, 0);
        la_AaddBmulC_sum_multi(Ain_Bin_mem_REG,
                               Ain_mem_REG, Zero_REG, Bin_mem_REG, ikcount);

        //update the C-elements
        la_AaddBmulC(Cin_mem_REG, 
                     Cin_mem_REG, Ain_Bin_mem_REG,
                     (k+kcount==P ? Alpha_REG : One_REG), icount);
      }
    }
  }
}

//==========================================================================

void dgemm_la_core(double *C, double *A, double *B, 
  double alpha, double beta,
  IDX N, IDX M, IDX P, IDX Noffset, IDX Ncount, IDX Moffset, IDX Mcount)
{
  if (USE_SCRATCH && USE_PANEL) {
    dgemm_la_core_scratch_panel(C, A, B, alpha, beta, N, M, P, 
                                Noffset, Ncount, Moffset, Mcount);
  } else if (USE_SCRATCH && !USE_PANEL) {
    dgemm_la_core_scratch_stripe(C, A, B, alpha, beta, N, M, P, 
                                 Noffset, Ncount, Moffset, Mcount);
  } else if (!USE_SCRATCH && USE_PANEL) {
    dgemm_la_core_no_scratch_panel(C, A, B, alpha, beta, N, M, P, 
                                   Noffset, Ncount, Moffset, Mcount);
  } else {
    dgemm_la_core_no_scratch_stripe(C, A, B, alpha, beta, N, M, P, 
                                    Noffset, Ncount, Moffset, Mcount);
  }
}
