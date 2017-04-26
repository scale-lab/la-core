
//==========================================================================
// Copyright (c) 2017 Samuel Steffl
// All rights reserved.
// 
// RISC-V LACore implementation of Complex double-precision FFT of
// arbitrary-length vectors
//==========================================================================

#include "fft/dfft_la_core.h"

#include <math.h>
#include <stdbool.h>
#include <string.h>

#include "la_core/la_core.h"
#include "la_core/la_set_scalar.h"
#include "la_core/la_set_vec.h"
#include "la_core/la_copy.h"
#include "la_core/la_data_op.h"
#include "matrix_types.h"
#include "util.h"

extern bool     DEBUG;
extern uint64_t SCRATCH_SIZE;


//==========================================================================

//used as a lookup table for the 'omegas' or roots of unity
static IDX omegas_size;
static Complex*omegas;

static const LaRegIdx tmp1_REG         = 1;
static const LaRegIdx tmp2_REG         = 2;
static const LaRegIdx Omega_sch_REG    = 6;
static const LaRegIdx OmegaRev_sch_REG = 7;

static const IDX DPCOUNT = sizeof(double);

static LaAddr OMEGA_SCH_ADR = 0;

//==========================================================================

//strides and skips in term of Complex! not double
void butterfly_inner(LaAddr A_jk, LaAddr A_jk2, IDX A_stride, 
  LaAddr Omega, IDX Omega_stride, IDX size)
{
  IDX DPSIZE = sizeof(double);

  const LaRegIdx Zero_REG         = 0;
  const LaRegIdx One_REG          = 0;
  const LaRegIdx tmp1_sch_REG     = 1;
  const LaRegIdx tmp2_sch_REG     = 2;
  const LaRegIdx u_sch_REG        = 2;
  const LaRegIdx t_sch_REG        = 3;

  const LaRegIdx A_jk_mem_REG     = 4;
  const LaRegIdx A_jk2_mem_REG    = 5;

  const LaAddr SCRATCH_DIV   = SCRATCH_SIZE/64;
  const LaAddr U_SCH_ADR     = 12*SCRATCH_DIV;
  const LaAddr T_SCH_ADR     = 24*SCRATCH_DIV;
  const LaAddr TMP_SCH_ADR   = 36*SCRATCH_DIV;

  //set these once up front
  la_set_vec_dp_mem(A_jk_mem_REG,     A_jk,          1, 2, (A_stride-1)*2);
  la_set_vec_dp_mem(A_jk2_mem_REG,    A_jk2,         1, 2, (A_stride-1)*2);
  //la_set_vec_dp_mem(Omega_mem_REG,    Omega,        1,2,(Omega_stride-1)*2);
  //la_set_vec_dp_mem(OmegaRev_mem_REG, Omega+DPSIZE,-1,2,(Omega_stride+1)*2);

  //if(Omega_stride == 0){
  //  la_set_vec_adr_dp_sch(tmp1_sch_REG, OMEGA_SCH_ADR);
  //  la_copy(tmp1_sch_REG, Omega_mem_REG, 2);
  //  la_set_vec_dp_sch(Omega_mem_REG,    OMEGA_SCH_ADR,1,2,-2);
  //  la_set_vec_dp_sch(OmegaRev_mem_REG, OMEGA_SCH_ADR+DPSIZE,-1,2,2);
 // }

  //real (ar*br + ai*bi)
  la_set_scalar_dp_reg(Zero_REG, 0.0);
  la_set_vec_adr_dp_sch(tmp1_sch_REG, TMP_SCH_ADR);
  la_AaddBmulC(tmp1_sch_REG, Omega_sch_REG, Zero_REG, A_jk2_mem_REG, size*2);

  la_set_scalar_dp_reg(One_REG, 1.0);
  la_set_vec_dp_sch(tmp1_sch_REG, TMP_SCH_ADR,        1, 1, 1);
  la_set_vec_dp_sch(tmp2_sch_REG, TMP_SCH_ADR+DPSIZE, 1, 1, 1);
  la_set_vec_dp_sch(t_sch_REG,    T_SCH_ADR,          1, 1, 1);
  la_AaddBmulC(t_sch_REG, tmp1_sch_REG, tmp2_sch_REG, One_REG, size);

  //imaginary (ar*bi - ai*br)
  la_set_scalar_dp_reg(Zero_REG, 0.0);
  la_set_vec_adr_dp_sch(tmp1_sch_REG, TMP_SCH_ADR);
  la_AaddBmulC(tmp1_sch_REG, OmegaRev_sch_REG, Zero_REG, A_jk2_mem_REG, size*2);

  la_set_scalar_dp_reg(One_REG, 1.0);
  la_set_vec_dp_sch(tmp1_sch_REG, TMP_SCH_ADR,        1, 1, 1); //W_i*A_jk2_r
  la_set_vec_dp_sch(tmp2_sch_REG, TMP_SCH_ADR+DPSIZE, 1, 1, 1); //W_r*A_jk2_i
  la_set_vec_dp_sch(t_sch_REG,    T_SCH_ADR+DPSIZE,   1, 1, 1);
  la_AsubBmulC(t_sch_REG, tmp2_sch_REG, tmp1_sch_REG, One_REG, size);

  //A[k+j] = u + t and A[k+j+m/2] = u - t
  la_set_scalar_dp_reg(One_REG, 1.0);
  la_set_vec_adr_dp_sch(u_sch_REG, U_SCH_ADR);
  la_set_vec_adr_dp_sch(t_sch_REG, T_SCH_ADR);

  la_copy(u_sch_REG, A_jk_mem_REG, size*2);
  la_AsubBmulC(A_jk2_mem_REG, u_sch_REG, t_sch_REG, One_REG, size*2);
  la_AaddBmulC(A_jk_mem_REG,  u_sch_REG, t_sch_REG, One_REG, size*2);
}

void dfft_la_core_inner(Complex *A, IDX size, IDX m)
{
  IDX M_CUTOFF      = 32;
  IDX K_BLOCK_SIZE  = 64;
  IDX J_BLOCK_SIZE  = 128;

  IDX mhalf       = m/2;
  IDX jmax        = mhalf;

  if(m < M_CUTOFF) { //stride across groups
    IDX kskip = m*K_BLOCK_SIZE;
    IDX A_stride = m;
    IDX Omega_stride = 0;
    for (IDX j=0; j<jmax; ++j) {
      LaAddr Omega = (LaAddr)&omegas[(2*omegas_size*j)/m];
      la_set_vec_dp_mem(tmp1_REG, Omega, 1, 2, 0);
      la_set_vec_dp_sch(Omega_sch_REG, OMEGA_SCH_ADR, 1, 2, 0);
      la_set_vec_dp_sch(OmegaRev_sch_REG, OMEGA_SCH_ADR+DPCOUNT, -1, 2, 2);
      la_copy(Omega_sch_REG, tmp1_REG, 2);

      for(IDX k=0; k<size; k+=kskip) {
        IDX kcount   = MIN(K_BLOCK_SIZE, (size-k)/m);
        LaAddr A_jk  = (LaAddr)&A[j+k];
        LaAddr A_jk2 = (LaAddr)&A[j+k+mhalf];
        butterfly_inner(A_jk, A_jk2, A_stride, Omega, Omega_stride, kcount);
      }
    }
  } else { //do 1 group at a time
    IDX jskip = J_BLOCK_SIZE;
    IDX A_stride     = 1;
    IDX Omega_stride = 2*omegas_size/m;
    for (IDX j=0; j<jmax; j+=J_BLOCK_SIZE) {
      IDX jcount   = MIN(J_BLOCK_SIZE, jmax-j);
      LaAddr Omega = (LaAddr)&omegas[(2*omegas_size*j)/m];

      la_set_vec_dp_mem(tmp1_REG, Omega, 1, 2, (Omega_stride-1)*2);
      la_set_vec_dp_sch(Omega_sch_REG, OMEGA_SCH_ADR, 1, 2, 0);
      la_set_vec_dp_sch(OmegaRev_sch_REG, OMEGA_SCH_ADR+DPCOUNT, -1, 2, 4);
      la_copy(Omega_sch_REG, tmp1_REG, jcount*2);

      for(IDX k=0; k<size; k+=m) {
        LaAddr A_jk  = (LaAddr)&A[j+k];
        LaAddr A_jk2 = (LaAddr)&A[j+k+mhalf];
        butterfly_inner(A_jk, A_jk2, A_stride, Omega, Omega_stride, jcount);
      }
    }
  }
}

//STOP OPTIMIZING THIS: doesn't have an effect on total performance
static void compute_omegas(IDX size)
{
  IDX half_size = size/2;
  IDX fourth_size = size/4;

  omegas_size = half_size;
  omegas = (Complex*) malloc(half_size*sizeof(Complex));
  double scale = 2*M_PI/(double)size;

  //precompute all the omegas used (roots-of-unity)
  for(IDX i=0; i<half_size; ++i){
    omegas[i].re = cos(scale*i);
  }
  for(IDX i=0; i<=fourth_size; ++i){
    omegas[i].im = sin(scale*i);
  }

  LaAddr w_2nd  = (LaAddr)(omegas+1);
  LaAddr w_last = (LaAddr)(omegas+half_size-1);
  const LaRegIdx src_REG = 0;
  const LaRegIdx dst_REG = 1;
  la_set_vec_dp_mem(src_REG, (w_2nd+8),    2, fourth_size-1, 0);
  la_set_vec_dp_mem(dst_REG, (w_last+8),  -2, fourth_size-1, 0);
  la_copy(dst_REG, src_REG, fourth_size-1);

  if(DEBUG){
    printf("omegas=[");
    for(IDX i=0; i<omegas_size;++i){
      printf("{%6.3f,%6.3f},", omegas[i]);
    }
    printf("]\n");
  }
}


//returns depth
IDX bit_reverse(Complex *A, Complex *B, IDX size) 
{
  IDX size2 = size/2;

  const LaRegIdx A_REG = 0;
  const LaRegIdx B_REG = 1;

  if(size > 1) {
    //B[i] = A[i*2];
    la_set_vec_dp_mem(A_REG, (LaAddr)A, 1, 2,    2);
    la_set_vec_dp_mem(B_REG, (LaAddr)B, 1, size, 0);
    la_copy(B_REG, A_REG, size);

    //B[size2+i] = A[i*2+1];
    la_set_vec_dp_mem(A_REG, (LaAddr)(A+1),     1, 2,    2);
    la_set_vec_dp_mem(B_REG, (LaAddr)(B+size2), 1, size, 0);
    la_copy(B_REG, A_REG, size);

    //iterate
    bit_reverse(B, A, size2);
    return 1 + bit_reverse(B+size2, A+size2, size2);
  }
  return 0;
}

void dfft_la_core(Complex*A, IDX size)
{
  IDX A_size = size*sizeof(Complex);

  compute_omegas(size);

  if(DEBUG){
    printf("before bits reversed\n");
    printf("m=0,j=0,j=0,A=[");
    for(IDX i=0; i<size;++i){
      printf("{%6.3f,%6.3f},", A[i]);
    }
    printf("]\n");
  }
  Complex *A_tmp = (Complex *)malloc(A_size);
  if(bit_reverse(A, A_tmp, size) % 2 == 1) {
    memcpy((char *)A, (char *)A_tmp, A_size);
  }
  if(DEBUG){
    printf("bits reversed\n");
    printf("m=0,j=0,j=0,A=[");
    for(IDX i=0; i<size;++i){
      printf("{%6.3f,%6.3f},", A[i]);
    }
    printf("]\n");
  }
  free(A_tmp);

  //perform kernel
  for(IDX m=2; m<=size; m*=2) {
    dfft_la_core_inner(A, size, m);
  }
  free(omegas);
}







/*

//==========================================================================
// Copyright (c) 2017 Samuel Steffl
// All rights reserved.
// 
// RISC-V LACore implementation of Complex double-precision FFT of
// arbitrary-length vectors
//==========================================================================

#include "fft/dfft_la_core.h"

#include <math.h>
#include <stdbool.h>
#include <string.h>

#include "la_core/la_core.h"
#include "la_core/la_set_scalar.h"
#include "la_core/la_set_vec.h"
#include "la_core/la_copy.h"
#include "la_core/la_data_op.h"
#include "matrix_types.h"
#include "util.h"

extern bool     DEBUG;
extern uint64_t SCRATCH_SIZE;


//==========================================================================

//used as a lookup table for the 'omegas' or roots of unity
static IDX omegas_size;
static Complex*omegas;

static const LaRegIdx tmp1_REG         = 1;
static const LaRegIdx tmp2_REG         = 2;
static const LaRegIdx Omega_sch_REG    = 6;
static const LaRegIdx OmegaRev_sch_REG = 7;

#define DPSIZE sizeof(double)

static LaAddr OMEGA_SCH_ADR = 0;

//need to set these before kernel starts!
static LaAddr SCRATCH_DIV  = 0;
static LaAddr U_SCH_ADR    = 0;
static LaAddr T_SCH_ADR    = 0;
static LaAddr TMP_SCH_ADR  = 0;

//==========================================================================

//strides and skips in term of Complex! not double
void butterfly_inner(LaAddr A_jk, LaAddr A_jk2, IDX A_stride, 
  LaAddr Omega, IDX Omega_stride, IDX size)
{
  IDX A_skip = (A_stride-1)*2;
  IDX size_x2 = size*2;

#define Zero_REG      0
#define One_REG       0
#define tmp1_sch_REG  1
#define tmp2_sch_REG  2
#define u_sch_REG     2
#define t_sch_REG     3

#define A_jk_mem_REG  4
#define A_jk2_mem_REG 5

  //set these once up front
  la_set_vec_dp_mem(A_jk_mem_REG,     A_jk,          1, 2, A_skip);
  la_set_vec_dp_mem(A_jk2_mem_REG,    A_jk2,         1, 2, A_skip);

  //real (ar*br + ai*bi)
  la_set_scalar_dp_reg(Zero_REG, 0.0);
  la_set_vec_dp_sch(t_sch_REG, T_SCH_ADR, 1, 1, 1);
  la_AaddBmulC_sum_multi(t_sch_REG, Omega_sch_REG, 
                         Zero_REG, A_jk2_mem_REG, size_x2);

  //imaginary (ar*bi - ai*br)
  la_set_scalar_dp_reg(Zero_REG, 0.0);
  la_set_vec_dp_sch(t_sch_REG,    T_SCH_ADR+DPSIZE,   1, 1, 1);
  la_AaddBmulC_sum_multi(t_sch_REG, OmegaRev_sch_REG, 
                         Zero_REG, A_jk2_mem_REG, size_x2);

  //A[k+j] = u + t and A[k+j+m/2] = u - t
  la_set_scalar_dp_reg(One_REG, 1.0);
  //la_set_vec_adr_dp_sch(u_sch_REG, U_SCH_ADR);
  la_set_vec_adr_dp_sch(t_sch_REG, T_SCH_ADR);

  //la_copy(u_sch_REG, A_jk_mem_REG, size*2);
  la_AsubBmulC(A_jk2_mem_REG, A_jk_mem_REG, t_sch_REG, One_REG, size_x2);
  la_AaddBmulC(A_jk_mem_REG,  A_jk_mem_REG, t_sch_REG, One_REG, size_x2);

#undef Zero_REG     
#undef One_REG      
#undef tmp1_sch_REG 
#undef tmp2_sch_REG 
#undef u_sch_REG    
#undef t_sch_REG    

#undef A_jk_mem_REG 
#undef A_jk2_mem_REG
}

void dfft_la_core_inner(Complex *A, IDX size)
{
#define M_CUTOFF     32
#define K_BLOCK_SIZE 64
#define J_BLOCK_SIZE 128

  for(IDX m=2; m<=size; m*=2) {
    IDX Omega_stride = 2*omegas_size/m;
    IDX mhalf       = m/2;
    IDX jmax        = mhalf;

    if(m < M_CUTOFF) { //stride across groups
      IDX kskip = m*K_BLOCK_SIZE;
      IDX A_stride = m;
      IDX Omega_stride = 0;
      for (IDX j=0; j<jmax; ++j) {
        LaAddr Omega = (LaAddr)&omegas[Omega_stride*j];
        la_set_vec_dp_mem(tmp1_REG, Omega, 1, 2, 0);
        la_set_vec_dp_sch(Omega_sch_REG, OMEGA_SCH_ADR, 1, 2, 0);
        la_set_vec_dp_sch(OmegaRev_sch_REG, OMEGA_SCH_ADR+DPSIZE, -1, 2, 2);
        la_copy(Omega_sch_REG, tmp1_REG, 2);

        for(IDX k=0; k<size; k+=kskip) {
          IDX jk = j+k;
          IDX kcount   = MIN(K_BLOCK_SIZE, (size-k)/m);
          LaAddr A_jk  = (LaAddr)&A[jk];
          LaAddr A_jk2 = (LaAddr)&A[jk+mhalf];
          butterfly_inner(A_jk, A_jk2, A_stride, Omega, Omega_stride, kcount);
        }
      }
    } 
    else { //do 1 group at a time
      IDX jskip = J_BLOCK_SIZE;
      IDX A_stride = 1;
      for (IDX j=0; j<jmax; j+=J_BLOCK_SIZE) {
        IDX jcount   = MIN(J_BLOCK_SIZE, jmax-j);
        LaAddr Omega = (LaAddr)&omegas[Omega_stride*j];

        la_set_vec_dp_mem(tmp1_REG, Omega, 1, 2, (Omega_stride-1)*2);
        la_set_vec_dp_sch(Omega_sch_REG, OMEGA_SCH_ADR, 1, 2, 0);
        la_set_vec_dp_sch(OmegaRev_sch_REG, OMEGA_SCH_ADR+DPSIZE, -1, 2, 4);
        la_copy(Omega_sch_REG, tmp1_REG, jcount*2);

        for(IDX k=0; k<size; k+=m) {
          IDX jk = j+k;
          LaAddr A_jk  = (LaAddr)&A[jk];
          LaAddr A_jk2 = (LaAddr)&A[jk+mhalf];
          butterfly_inner(A_jk, A_jk2, A_stride, Omega, Omega_stride, jcount);
        }
      }
    }
  }
#undef J_BLOCK_SIZE
#undef K_BLOCK_SIZE
#undef M_CUTOFF
}

//Garrett, Charles K. "Fast Polynomial Approximations to Sine and Cosine." (2012).
static void cosine_stream(Complex *data, IDX count){

#define A (-9.77507131527006498114e-12)
#define B (2.06207503915813519567e-09)
#define C (-2.75369918573799545860e-07)
#define D (2.48006913718665260256e-05)
#define E (-1.38888674687691339750e-03)
#define F (4.16666641590361985136e-02)
#define G (-4.99999998886526927002e-01)
#define H (9.99999999919365479957e-01)

#define Const_REG 0
#define accum_REG 1
#define Zero_REG  2
#define One_REG   3
#define X_REG     4

  la_set_scalar_dp_reg(Zero_REG, 0.0);
  la_set_scalar_dp_reg(One_REG, 1.0);
  la_set_vec_dp_mem(accum_REG, (LaAddr)&(data->re), 1, 1, 1);
  la_set_vec_dp_mem(X_REG,     (LaAddr)&(data->im), 1, 1, 1);

  //square the x
  la_AaddBmulC(X_REG, X_REG, Zero_REG, X_REG, count);

  la_set_scalar_dp_reg(Const_REG, A);
  la_AaddBmulC(accum_REG, accum_REG, Const_REG, X_REG, count);

  la_set_scalar_dp_reg(Const_REG, B);
  la_AaddBmulC(accum_REG, accum_REG, Const_REG, X_REG, count);

  la_set_scalar_dp_reg(Const_REG, C);
  la_AaddBmulC(accum_REG, accum_REG, Const_REG, X_REG, count);

  la_set_scalar_dp_reg(Const_REG, D);
  la_AaddBmulC(accum_REG, accum_REG, Const_REG, X_REG, count);

  la_set_scalar_dp_reg(Const_REG, E);
  la_AaddBmulC(accum_REG, accum_REG, Const_REG, X_REG, count);

  la_set_scalar_dp_reg(Const_REG, F);
  la_AaddBmulC(accum_REG, accum_REG, Const_REG, X_REG, count);

  la_set_scalar_dp_reg(Const_REG, G);
  la_AaddBmulC(accum_REG, accum_REG, Const_REG, X_REG, count);

  la_set_scalar_dp_reg(Const_REG, H);
  la_AaddBmulC(accum_REG, accum_REG, Const_REG, One_REG, count);

#undef A
#undef B
#undef C
#undef D
#undef E
#undef F
#undef G
#undef H

#undef Const_REG
#undef accum_REG
#undef Zero_REG 
#undef X_REG 
}

//STOP OPTIMIZING THIS: doesn't have an effect on total performance
static void compute_omegas(IDX size)
{
  IDX half_size = size/2;
  IDX fourth_size = size/4;

  omegas_size = half_size;
  omegas = (Complex*) malloc(half_size*sizeof(Complex));
  double scale = 2*M_PI/(double)size;

  //precompute all the omegas used (roots-of-unity)
#define BLOCK       256
#define Zero_REG    0
#define Scale_REG   1
#define Vec_Src_REG 2
#define Vec_Dst_REG 2
  if(size > BLOCK) {
    la_set_scalar_dp_reg(Zero_REG,  0.0);
    la_set_scalar_dp_reg(Scale_REG, BLOCK);
    for(IDX i=0; i<BLOCK; ++i){
      omegas[i].im = i*scale;
      //omegas[i].re = cos(omegas[i].re);
    }
    cosine_stream(omegas, BLOCK);
    for(IDX i=0; i<fourth_size-BLOCK; i+=BLOCK){
      la_set_vec_dp_mem(Vec_Src_REG, (LaAddr)(omegas+i), 1, 1, 1);
      la_set_vec_dp_mem(Vec_Dst_REG, (LaAddr)(omegas+i+BLOCK), 1, 1, 1);
      la_AaddBmulC(Vec_Dst_REG, Vec_Src_REG, Zero_REG, Scale_REG, BLOCK);
      //for(IDX j=i; j<(i+BLOCK); ++j){
      //  omegas[j].re = cos(omegas[j].re);
      //}
      cosine_stream(omegas+i+BLOCK, BLOCK);
    }
  } 
  else {
    for(IDX i=0; i<=fourth_size; ++i){
      double scale_i = scale*i;
      omegas[i].re = cos(scale_i);
    }
  }
#undef Vec_Src_REG
#undef Vec_Dst_REG
#undef Scale_REG
#undef Zero_REG
#undef BLOCK

  LaAddr w_2nd      = (LaAddr)(omegas+1);
  LaAddr w_fourth_1 = (LaAddr)(omegas+fourth_size-1);
  LaAddr w_last     = (LaAddr)(omegas+half_size-1);
  const LaRegIdx src_REG  = 0;
  const LaRegIdx dst_REG  = 1;
  const LaRegIdx sign_REG = 2;
  const LaRegIdx zero_REG = 3;

  la_set_scalar_dp_reg(sign_REG, -1.0);
  la_set_scalar_dp_reg(zero_REG, 0.0);
  la_set_vec_dp_mem(src_REG, w_2nd,   2, fourth_size-1, 0);

  //copy/flip real part
  la_set_vec_dp_mem(dst_REG, w_last, -2, fourth_size-1, 0);
  la_AaddBmulC(dst_REG, src_REG, zero_REG, sign_REG, fourth_size-1);

  //copy to imaginary parts
  la_set_vec_dp_mem(dst_REG, (w_fourth_1+8), -2, fourth_size-1, 0);
  la_copy(dst_REG, src_REG, fourth_size-1);
  la_set_vec_dp_mem(dst_REG, (w_fourth_1+24), 2, fourth_size-1, 0);
  la_copy(dst_REG, src_REG, fourth_size-1);

  if(DEBUG){
    printf("omegas=[");
    for(IDX i=0; i<omegas_size;++i){
      printf("{%6.3f,%6.3f},", omegas[i]);
    }
    printf("]\n");
  }
}


//returns depth
IDX bit_reverse(Complex *A, Complex *B, IDX size) 
{
#define A_REG 0
#define B_REG 1
#define MIN_BLOCK 8192

  IDX iterations = 0;

  IDX i = size;
  IDX i_div_2;
  for(; i>MIN_BLOCK; i=i_div_2) {
    i_div_2 = (i >> 1);

    //B[i] = A[i*2];
    la_set_vec_dp_mem(A_REG, (LaAddr)A, 1, 2, 2);
    la_set_vec_dp_mem(B_REG, (LaAddr)B, 1, i, i);
    la_copy(B_REG, A_REG, size);

    //B[size2+i] = A[i*2+1];
    la_set_vec_dp_mem(A_REG, (LaAddr)(A+1),       1, 2, 2);
    la_set_vec_dp_mem(B_REG, (LaAddr)(B+i_div_2), 1, i, i);
    la_copy(B_REG, A_REG, size);

    //now switch A and B vectors
    Complex * tmp = A;
    A = B;
    B = tmp;

    ++iterations;
  } 

  IDX i_orig = i;
  Complex *A_orig = A;
  Complex *B_orig = B;
  for(IDX j=0; j<size; j+=MIN_BLOCK) {
    IDX jsize = MIN(MIN_BLOCK, (size-j));
    A = A_orig;
    B = B_orig;
    for(i=i_orig; i>=2; i=i_div_2) {
      i_div_2 = (i >> 1);

      //B[i] = A[i*2];
      la_set_vec_dp_mem(A_REG, (LaAddr)(A+j), 1, 2, 2);
      la_set_vec_dp_mem(B_REG, (LaAddr)(B+j), 1, i, i);
      la_copy(B_REG, A_REG, jsize);

      //B[size2+i] = A[i*2+1];
      la_set_vec_dp_mem(A_REG, (LaAddr)(A+j+1),       1, 2, 2);
      la_set_vec_dp_mem(B_REG, (LaAddr)(B+j+i_div_2), 1, i, i);
      la_copy(B_REG, A_REG, jsize);

      //now switch A and B vectors
      Complex * tmp = A;
      A = B;
      B = tmp;

      if(j==0){
        ++iterations;
      }
    }
  }

#undef A_REG
#undef B_REG

  return iterations;
}

void dfft_la_core(Complex*A, IDX size)
{
  //set this up front!
  SCRATCH_DIV   = SCRATCH_SIZE/64;
  U_SCH_ADR     = 12*SCRATCH_DIV;
  T_SCH_ADR     = 24*SCRATCH_DIV;
  TMP_SCH_ADR   = 36*SCRATCH_DIV;

  IDX A_size = size*sizeof(Complex);

  compute_omegas(size);

  if(DEBUG){
    printf("before bits reversed\n");
    printf("m=0,j=0,j=0,A=[");
    for(IDX i=0; i<size;++i){
      printf("{%6.3f,%6.3f},", A[i]);
    }
    printf("]\n");
  }
  Complex *A_tmp = (Complex *)malloc(A_size);
  if(bit_reverse(A, A_tmp, size) % 2 == 1) {
    memcpy((char *)A, (char *)A_tmp, A_size);
  }
  if(DEBUG){
    printf("bits reversed\n");
    printf("m=0,j=0,j=0,A=[");
    for(IDX i=0; i<size;++i){
      printf("{%6.3f,%6.3f},", A[i]);
    }
    printf("]\n");
  }
  free(A_tmp);

  //perform kernel
  dfft_la_core_inner(A, size);

  free(omegas);
}
*/
