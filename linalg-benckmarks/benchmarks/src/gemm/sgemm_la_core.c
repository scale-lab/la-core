//==========================================================================
// Copyright (c) 2017 Samuel Steffl
// All rights reserved.
// 
// RISC-V LACore implementation of BLAS-3 DTRSM function
//==========================================================================

#include "gemm/sgemm_la_core.h"

#include <stdbool.h>

#include "la_core/la_core.h"
#include "la_core/la_set_scalar.h"
#include "la_core/la_set_vec.h"
#include "la_core/la_copy.h"
#include "la_core/la_data_op.h"
#include "matrix_types.h"
#include "util.h"

extern bool DEBUG;

//==========================================================================
// C := alpha*op(A)*op(B) + beta*C
// assuming the transpositions were done beforehand
// C = col-major, A = row-major, B=col-major
//==========================================================================

#define SIZE_SP       (sizeof(float))
#define SCRATCH_SIZE  (1<<18)
#define BANKS         (32)
#define BANK_SIZE     (SCRATCH_SIZE/BANKS)
#define STRIDE_S      (BANK_SIZE/SIZE_SP)

#define BANK_STRIDE_REG(reg, start, count) \
    la_set_vec_sp_sch(reg, start, STRIDE_S, (count), 1-(STRIDE_S*(count)))

#define BANK_REPEAT_REG(reg, start, count) \
    la_set_vec_sp_sch(reg, start, STRIDE_S, (count), -(STRIDE_S*(count)))


void sgemm_la_core(float *C, float *A, float *B, float alpha, float beta,
  IDX N, IDX M, IDX P, IDX Noffset, IDX Ncount, IDX Moffset, IDX Mcount)
{
  IDX istart = Noffset;
  IDX iend   = Noffset + Ncount;
  IDX jstart = Moffset;
  IDX jend   = Moffset + Mcount;

  const IDX I_BLOCK = BANKS;
  const IDX J_BLOCK = BANKS*8;
  const IDX K_BLOCK = BANKS;

  const LaAddr CIN_SCH_ADDR     = 0*(BANK_SIZE/8); //256 of 1024 bank items
  const LaAddr AIN_SCH_ADDR     = 2*(BANK_SIZE/8); //32 of 1024 bank items
  const LaAddr BIN_SCH_ADDR     = 4*(BANK_SIZE/8); //256 of 1024 bank items
  const LaAddr AIN_BIN_SCH_ADDR = 6*(BANK_SIZE/8); //256 of 1024 bank items

  //what registers to use for constants (0.0, 1.0, alpha, beta)
  const LaRegIdx Zero_REG             = 0;
  const LaRegIdx One_REG              = 1;
  const LaRegIdx Beta_REG             = 2;
  const LaRegIdx Alpha_REG            = 2;

  //registers for input and vector configs
  const LaRegIdx Cin_sch_REG          = 3;
  const LaRegIdx Cin_mem_REG          = 4;
  const LaRegIdx Bin_sch_REG          = 5;
  const LaRegIdx Bin_mem_REG          = 6;
  const LaRegIdx Ain_sch_REG          = 6; //NOTE: a/b regs are reversed!
  const LaRegIdx Ain_mem_REG          = 5;
  const LaRegIdx Ain_Bin_sch_REG      = 7;


  la_set_scalar_sp_reg(Zero_REG, 0.0);
  la_set_scalar_sp_reg(One_REG, 1.0);

  for (IDX j = jstart; j<jend; j+=J_BLOCK) {
    IDX jcount = MIN(J_BLOCK, (jend-j));

    for (IDX i = istart; i<iend; i+=I_BLOCK) {
      IDX icount = MIN(I_BLOCK, (iend-i));
      IDX ijcount = icount*jcount;
      LaAddr Cin = (LaAddr) &(IDXC(C, i, j, N, M));

      //load the C-elements into scratchpad
      la_set_vec_sp_mem(Cin_mem_REG, Cin, 1, icount, (N-icount));
      BANK_STRIDE_REG(Cin_sch_REG, CIN_SCH_ADDR, icount);
      la_set_scalar_sp_reg(Beta_REG, beta);
      la_AaddBmulC(Cin_sch_REG, Cin_mem_REG, Zero_REG, Beta_REG, ijcount);

      la_set_scalar_sp_reg(Alpha_REG, alpha);
      for(IDX k=0; k<P; k += K_BLOCK){
        IDX kcount = MIN(K_BLOCK, (P-k));
        IDX ikcount = icount*kcount;
        IDX jkcount = jcount*kcount;
        LaAddr Ain = (LaAddr) &(IDXR(A, i, k, N, P));
        LaAddr Bin = (LaAddr) &(IDXC(B, k, j, P, M));

        la_set_vec_sp_mem(Bin_mem_REG, Bin, 1, kcount, (P-kcount));
        BANK_STRIDE_REG(Bin_sch_REG, BIN_SCH_ADDR, kcount);
        la_AaddBmulC(Bin_sch_REG, Bin_mem_REG, Zero_REG, Alpha_REG, jkcount);

        la_set_vec_sp_mem(Ain_mem_REG, Ain, 1, kcount, (P-kcount));
        BANK_STRIDE_REG(Ain_sch_REG, AIN_SCH_ADDR, kcount);
        la_copy(Ain_sch_REG, Ain_mem_REG, ikcount);

        /*
        IDX jj_offset = 0;
        for(IDX jj=0; jj<jcount; ++jj){
          BANK_REPEAT_REG(Bin_sch_REG, BIN_SCH_ADDR+jj_offset, kcount);
          BANK_STRIDE_REG(Ain_Bin_sch_REG, AIN_BIN_SCH_ADDR+jj_offset, icount);
          la_AaddBmulC_sum_multi(Ain_Bin_sch_REG, Ain_sch_REG, Zero_REG, Bin_sch_REG, ikcount);
          jj_offset += SIZE_DP;
        }
        */

        BANK_STRIDE_REG(Bin_sch_REG, BIN_SCH_ADDR, kcount);
        for(IDX ii=0; ii<icount; ++ii){
          BANK_REPEAT_REG(Ain_sch_REG, AIN_SCH_ADDR+(ii*SIZE_SP), kcount);
          la_set_vec_adrcnt_sp_sch(Ain_Bin_sch_REG, AIN_BIN_SCH_ADDR+(ii*BANK_SIZE), jcount);
          la_AaddBmulC_sum_multi(Ain_Bin_sch_REG, Ain_sch_REG, Zero_REG, Bin_sch_REG, jkcount);
        }

        BANK_STRIDE_REG(Ain_Bin_sch_REG, AIN_BIN_SCH_ADDR, icount);
        if(k+kcount == P){
          //finally, write back C-elements to main memory
          la_AaddBmulC(Cin_mem_REG, Cin_sch_REG, Ain_Bin_sch_REG, One_REG, ijcount);
        } else {
          la_AaddBmulC(Cin_sch_REG, Cin_sch_REG, Ain_Bin_sch_REG, One_REG, ijcount);
        }
      }
    }
  }
}
