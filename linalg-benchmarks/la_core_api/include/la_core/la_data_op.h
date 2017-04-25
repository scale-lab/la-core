//==========================================================================
// Copyright (c) 2017 Samuel Steffl
// All rights reserved.
// 
//Implements all the data operations for any input register combo:
//  ldr{m|d}{a|s}{b|t}{n|x|s}   lrd, lra, lrb, lrc, lrxa
//  ldrm{m|d}{a|s}{b|t}{n|x|s}  lrd, lra, lrb, lrc, lrxa
//  ldv{m|d}{a|s}{b|t}          lrd, lra, lrb, lrc, lrxa
//  ldvm{m|d}{a|s}{b|t}         lrd, lra, lrb, lrc, lrxa
//
// This header generates ~ 270,000 functions, but they are all inlined so 
// the ones that aren't used just get compiled away -- so the binary size 
// won't be rediculous
//==========================================================================

#ifndef __LA_CORE_LA_DATA_OP_H__
#define __LA_CORE_LA_DATA_OP_H__

#include "la_core/la_core.h"

//==========================================================================
// The Actual Data-OP inline function definitions (262,144 total variants)
//==========================================================================

#define LA_DATA_OP_ASM(opcode, D, A, B, C)                                    \
  asm (#opcode " lr" #D ", lr" #A ", lr" #B ", lr" #C ", %0\n\t"              \
       : : "r" (count));                                                      \

#define LA_DEFINE_DATA_OP_FUNC(op, opcode, _B, _C)                            \
                                                                              \
inline void la_##op##_B##_C (LaRegIdx D, LaRegIdx A, LaCount count) {         \
  switch(8*D+A){                                                              \
    case 0: LA_DATA_OP_ASM(opcode, 0, 0, _B, _C); break;                      \
    case 1: LA_DATA_OP_ASM(opcode, 0, 1, _B, _C); break;                      \
    case 2: LA_DATA_OP_ASM(opcode, 0, 2, _B, _C); break;                      \
    case 3: LA_DATA_OP_ASM(opcode, 0, 3, _B, _C); break;                      \
    case 4: LA_DATA_OP_ASM(opcode, 0, 4, _B, _C); break;                      \
    case 5: LA_DATA_OP_ASM(opcode, 0, 5, _B, _C); break;                      \
    case 6: LA_DATA_OP_ASM(opcode, 0, 6, _B, _C); break;                      \
    case 7: LA_DATA_OP_ASM(opcode, 0, 7, _B, _C); break;                      \
                                                                              \
    case 8+0: LA_DATA_OP_ASM(opcode, 1, 0, _B, _C); break;                    \
    case 8+1: LA_DATA_OP_ASM(opcode, 1, 1, _B, _C); break;                    \
    case 8+2: LA_DATA_OP_ASM(opcode, 1, 2, _B, _C); break;                    \
    case 8+3: LA_DATA_OP_ASM(opcode, 1, 3, _B, _C); break;                    \
    case 8+4: LA_DATA_OP_ASM(opcode, 1, 4, _B, _C); break;                    \
    case 8+5: LA_DATA_OP_ASM(opcode, 1, 5, _B, _C); break;                    \
    case 8+6: LA_DATA_OP_ASM(opcode, 1, 6, _B, _C); break;                    \
    case 8+7: LA_DATA_OP_ASM(opcode, 1, 7, _B, _C); break;                    \
                                                                              \
    case 16+0: LA_DATA_OP_ASM(opcode, 2, 0, _B, _C); break;                   \
    case 16+1: LA_DATA_OP_ASM(opcode, 2, 1, _B, _C); break;                   \
    case 16+2: LA_DATA_OP_ASM(opcode, 2, 2, _B, _C); break;                   \
    case 16+3: LA_DATA_OP_ASM(opcode, 2, 3, _B, _C); break;                   \
    case 16+4: LA_DATA_OP_ASM(opcode, 2, 4, _B, _C); break;                   \
    case 16+5: LA_DATA_OP_ASM(opcode, 2, 5, _B, _C); break;                   \
    case 16+6: LA_DATA_OP_ASM(opcode, 2, 6, _B, _C); break;                   \
    case 16+7: LA_DATA_OP_ASM(opcode, 2, 7, _B, _C); break;                   \
                                                                              \
    case 24+0: LA_DATA_OP_ASM(opcode, 3, 0, _B, _C); break;                   \
    case 24+1: LA_DATA_OP_ASM(opcode, 3, 1, _B, _C); break;                   \
    case 24+2: LA_DATA_OP_ASM(opcode, 3, 2, _B, _C); break;                   \
    case 24+3: LA_DATA_OP_ASM(opcode, 3, 3, _B, _C); break;                   \
    case 24+4: LA_DATA_OP_ASM(opcode, 3, 4, _B, _C); break;                   \
    case 24+5: LA_DATA_OP_ASM(opcode, 3, 5, _B, _C); break;                   \
    case 24+6: LA_DATA_OP_ASM(opcode, 3, 6, _B, _C); break;                   \
    case 24+7: LA_DATA_OP_ASM(opcode, 3, 7, _B, _C); break;                   \
                                                                              \
    case 32+0: LA_DATA_OP_ASM(opcode, 4, 0, _B, _C); break;                   \
    case 32+1: LA_DATA_OP_ASM(opcode, 4, 1, _B, _C); break;                   \
    case 32+2: LA_DATA_OP_ASM(opcode, 4, 2, _B, _C); break;                   \
    case 32+3: LA_DATA_OP_ASM(opcode, 4, 3, _B, _C); break;                   \
    case 32+4: LA_DATA_OP_ASM(opcode, 4, 4, _B, _C); break;                   \
    case 32+5: LA_DATA_OP_ASM(opcode, 4, 5, _B, _C); break;                   \
    case 32+6: LA_DATA_OP_ASM(opcode, 4, 6, _B, _C); break;                   \
    case 32+7: LA_DATA_OP_ASM(opcode, 4, 7, _B, _C); break;                   \
                                                                              \
    case 40+0: LA_DATA_OP_ASM(opcode, 5, 0, _B, _C); break;                   \
    case 40+1: LA_DATA_OP_ASM(opcode, 5, 1, _B, _C); break;                   \
    case 40+2: LA_DATA_OP_ASM(opcode, 5, 2, _B, _C); break;                   \
    case 40+3: LA_DATA_OP_ASM(opcode, 5, 3, _B, _C); break;                   \
    case 40+4: LA_DATA_OP_ASM(opcode, 5, 4, _B, _C); break;                   \
    case 40+5: LA_DATA_OP_ASM(opcode, 5, 5, _B, _C); break;                   \
    case 40+6: LA_DATA_OP_ASM(opcode, 5, 6, _B, _C); break;                   \
    case 40+7: LA_DATA_OP_ASM(opcode, 5, 7, _B, _C); break;                   \
                                                                              \
    case 48+0: LA_DATA_OP_ASM(opcode, 6, 0, _B, _C); break;                   \
    case 48+1: LA_DATA_OP_ASM(opcode, 6, 1, _B, _C); break;                   \
    case 48+2: LA_DATA_OP_ASM(opcode, 6, 2, _B, _C); break;                   \
    case 48+3: LA_DATA_OP_ASM(opcode, 6, 3, _B, _C); break;                   \
    case 48+4: LA_DATA_OP_ASM(opcode, 6, 4, _B, _C); break;                   \
    case 48+5: LA_DATA_OP_ASM(opcode, 6, 5, _B, _C); break;                   \
    case 48+6: LA_DATA_OP_ASM(opcode, 6, 6, _B, _C); break;                   \
    case 48+7: LA_DATA_OP_ASM(opcode, 6, 7, _B, _C); break;                   \
                                                                              \
    case 56+0: LA_DATA_OP_ASM(opcode, 7, 0, _B, _C); break;                   \
    case 56+1: LA_DATA_OP_ASM(opcode, 7, 1, _B, _C); break;                   \
    case 56+2: LA_DATA_OP_ASM(opcode, 7, 2, _B, _C); break;                   \
    case 56+3: LA_DATA_OP_ASM(opcode, 7, 3, _B, _C); break;                   \
    case 56+4: LA_DATA_OP_ASM(opcode, 7, 4, _B, _C); break;                   \
    case 56+5: LA_DATA_OP_ASM(opcode, 7, 5, _B, _C); break;                   \
    case 56+6: LA_DATA_OP_ASM(opcode, 7, 6, _B, _C); break;                   \
    case 56+7: LA_DATA_OP_ASM(opcode, 7, 7, _B, _C); break;                   \
  }                                                                           \
}                                                                             \

//==========================================================================
// Create definitions for the 262,144 total variants of DataOps
//==========================================================================

#define LA_DEFINE_DATA_OP_FUNC_BY_BC(op, opcode, B)                           \
  LA_DEFINE_DATA_OP_FUNC(op, opcode, B, 0);                                   \
  LA_DEFINE_DATA_OP_FUNC(op, opcode, B, 1);                                   \
  LA_DEFINE_DATA_OP_FUNC(op, opcode, B, 2);                                   \
  LA_DEFINE_DATA_OP_FUNC(op, opcode, B, 3);                                   \
  LA_DEFINE_DATA_OP_FUNC(op, opcode, B, 4);                                   \
  LA_DEFINE_DATA_OP_FUNC(op, opcode, B, 5);                                   \
  LA_DEFINE_DATA_OP_FUNC(op, opcode, B, 6);                                   \
  LA_DEFINE_DATA_OP_FUNC(op, opcode, B, 7);                                   \


#define LA_DEFINE_DATA_OP_FUNC_BY_B(op, opcode)                               \
  LA_DEFINE_DATA_OP_FUNC_BY_BC(op, opcode, 0);                                \
  LA_DEFINE_DATA_OP_FUNC_BY_BC(op, opcode, 1);                                \
  LA_DEFINE_DATA_OP_FUNC_BY_BC(op, opcode, 2);                                \
  LA_DEFINE_DATA_OP_FUNC_BY_BC(op, opcode, 3);                                \
  LA_DEFINE_DATA_OP_FUNC_BY_BC(op, opcode, 4);                                \
  LA_DEFINE_DATA_OP_FUNC_BY_BC(op, opcode, 5);                                \
  LA_DEFINE_DATA_OP_FUNC_BY_BC(op, opcode, 6);                                \
  LA_DEFINE_DATA_OP_FUNC_BY_BC(op, opcode, 7);                                \

//==========================================================================
// Switch hierarchy to reduce the number of user-facing functions from 
// 262,144 down to 64
//==========================================================================

#define LA_CREATE_DATA_OP_CASES_B(op, _B)                                     \
  case (_B*8)+0: la_##op##_B##0(D,A,count); break;                            \
  case (_B*8)+1: la_##op##_B##1(D,A,count); break;                            \
  case (_B*8)+2: la_##op##_B##2(D,A,count); break;                            \
  case (_B*8)+3: la_##op##_B##3(D,A,count); break;                            \
  case (_B*8)+4: la_##op##_B##4(D,A,count); break;                            \
  case (_B*8)+5: la_##op##_B##5(D,A,count); break;                            \
  case (_B*8)+6: la_##op##_B##6(D,A,count); break;                            \
  case (_B*8)+7: la_##op##_B##7(D,A,count); break;                            \
  

#define LA_CREATE_DATA_OP_SWITCH(op)                                          \
  inline void la_##op(LaRegIdx D, LaRegIdx A, LaRegIdx B, LaRegIdx C,         \
    LaCount count) {                                                          \
    switch(B*8+C){                                                            \
      LA_CREATE_DATA_OP_CASES_B(op, 0);                                       \
      LA_CREATE_DATA_OP_CASES_B(op, 1);                                       \
      LA_CREATE_DATA_OP_CASES_B(op, 2);                                       \
      LA_CREATE_DATA_OP_CASES_B(op, 3);                                       \
      LA_CREATE_DATA_OP_CASES_B(op, 4);                                       \
      LA_CREATE_DATA_OP_CASES_B(op, 5);                                       \
      LA_CREATE_DATA_OP_CASES_B(op, 6);                                       \
      LA_CREATE_DATA_OP_CASES_B(op, 7);                                       \
    }                                                                         \
  }                                                                           \


//==========================================================================
// Create all definitions and switches for each DataOp function
//==========================================================================

#define LA_CONFIG_DATA_OP_FUNC(op, opcode)                                    \
  LA_DEFINE_DATA_OP_FUNC_BY_B(op, opcode);                                    \
  LA_CREATE_DATA_OP_SWITCH(op);                                               \

//all the sinle-output single-stream functions
LA_CONFIG_DATA_OP_FUNC(AaddBmulC_min, ldrmatn);
LA_CONFIG_DATA_OP_FUNC(AmulBaddC_min, ldrmabn);
LA_CONFIG_DATA_OP_FUNC(AsubBmulC_min, ldrmstn);
LA_CONFIG_DATA_OP_FUNC(AmulBsubC_min, ldrmsbn);
LA_CONFIG_DATA_OP_FUNC(AaddBdivC_min, ldrdatn);
LA_CONFIG_DATA_OP_FUNC(AdivBaddC_min, ldrdabn);
LA_CONFIG_DATA_OP_FUNC(AsubBdivC_min, ldrdstn);
LA_CONFIG_DATA_OP_FUNC(AdivBsubC_min, ldrdsbn);

LA_CONFIG_DATA_OP_FUNC(AaddBmulC_max, ldrmatx);
LA_CONFIG_DATA_OP_FUNC(AmulBaddC_max, ldrmabx);
LA_CONFIG_DATA_OP_FUNC(AsubBmulC_max, ldrmstx);
LA_CONFIG_DATA_OP_FUNC(AmulBsubC_max, ldrmsbx);
LA_CONFIG_DATA_OP_FUNC(AaddBdivC_max, ldrdatx);
LA_CONFIG_DATA_OP_FUNC(AdivBaddC_max, ldrdabx);
LA_CONFIG_DATA_OP_FUNC(AsubBdivC_max, ldrdstx);
LA_CONFIG_DATA_OP_FUNC(AdivBsubC_max, ldrdsbx);

LA_CONFIG_DATA_OP_FUNC(AaddBmulC_sum, ldrmats);
LA_CONFIG_DATA_OP_FUNC(AmulBaddC_sum, ldrmabs);
LA_CONFIG_DATA_OP_FUNC(AsubBmulC_sum, ldrmsts);
LA_CONFIG_DATA_OP_FUNC(AmulBsubC_sum, ldrmsbs);
LA_CONFIG_DATA_OP_FUNC(AaddBdivC_sum, ldrdats);
LA_CONFIG_DATA_OP_FUNC(AdivBaddC_sum, ldrdabs);
LA_CONFIG_DATA_OP_FUNC(AsubBdivC_sum, ldrdsts);
LA_CONFIG_DATA_OP_FUNC(AdivBsubC_sum, ldrdsbs);

//all the sinle-output multi-stream functions
LA_CONFIG_DATA_OP_FUNC(AaddBmulC_min_multi, ldrmmatn);
LA_CONFIG_DATA_OP_FUNC(AmulBaddC_min_multi, ldrmmabn);
LA_CONFIG_DATA_OP_FUNC(AsubBmulC_min_multi, ldrmmstn);
LA_CONFIG_DATA_OP_FUNC(AmulBsubC_min_multi, ldrmmsbn);
LA_CONFIG_DATA_OP_FUNC(AaddBdivC_min_multi, ldrmdatn);
LA_CONFIG_DATA_OP_FUNC(AdivBaddC_min_multi, ldrmdabn);
LA_CONFIG_DATA_OP_FUNC(AsubBdivC_min_multi, ldrmdstn);
LA_CONFIG_DATA_OP_FUNC(AdivBsubC_min_multi, ldrmdsbn);

LA_CONFIG_DATA_OP_FUNC(AaddBmulC_max_multi, ldrmmatx);
LA_CONFIG_DATA_OP_FUNC(AmulBaddC_max_multi, ldrmmabx);
LA_CONFIG_DATA_OP_FUNC(AsubBmulC_max_multi, ldrmmstx);
LA_CONFIG_DATA_OP_FUNC(AmulBsubC_max_multi, ldrmmsbx);
LA_CONFIG_DATA_OP_FUNC(AaddBdivC_max_multi, ldrmdatx);
LA_CONFIG_DATA_OP_FUNC(AdivBaddC_max_multi, ldrmdabx);
LA_CONFIG_DATA_OP_FUNC(AsubBdivC_max_multi, ldrmdstx);
LA_CONFIG_DATA_OP_FUNC(AdivBsubC_max_multi, ldrmdsbx);

LA_CONFIG_DATA_OP_FUNC(AaddBmulC_sum_multi, ldrmmats);
LA_CONFIG_DATA_OP_FUNC(AmulBaddC_sum_multi, ldrmmabs);
LA_CONFIG_DATA_OP_FUNC(AsubBmulC_sum_multi, ldrmmsts);
LA_CONFIG_DATA_OP_FUNC(AmulBsubC_sum_multi, ldrmmsbs);
LA_CONFIG_DATA_OP_FUNC(AaddBdivC_sum_multi, ldrmdats);
LA_CONFIG_DATA_OP_FUNC(AdivBaddC_sum_multi, ldrmdabs);
LA_CONFIG_DATA_OP_FUNC(AsubBdivC_sum_multi, ldrmdsts);
LA_CONFIG_DATA_OP_FUNC(AdivBsubC_sum_multi, ldrmdsbs);

//all the vector-output single-stream functions
LA_CONFIG_DATA_OP_FUNC(AaddBmulC, ldvmat);
LA_CONFIG_DATA_OP_FUNC(AmulBaddC, ldvmab);
LA_CONFIG_DATA_OP_FUNC(AsubBmulC, ldvmst);
LA_CONFIG_DATA_OP_FUNC(AmulBsubC, ldvmsb);
LA_CONFIG_DATA_OP_FUNC(AaddBdivC, ldvdat);
LA_CONFIG_DATA_OP_FUNC(AdivBaddC, ldvdab);
LA_CONFIG_DATA_OP_FUNC(AsubBdivC, ldvdst);
LA_CONFIG_DATA_OP_FUNC(AdivBsubC, ldvdsb);

#endif // __LA_CORE_LA_DATA_OP_H__
