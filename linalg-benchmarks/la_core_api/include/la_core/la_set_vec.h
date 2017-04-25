//==========================================================================
// Copyright (c) 2017 Samuel Steffl
// All rights reserved.
// 
// CFG vector instructions
//==========================================================================

#ifndef __LA_CORE_LA_SET_VEC_H__
#define __LA_CORE_LA_SET_VEC_H__

#include "la_core/la_core.h"

#define DEF_STRIDE 1
#define DEF_COUNT 1
#define DEF_SKIP 0

//see explicit compiler barriers section:
//  http://preshing.com/20120625/memory-ordering-at-compile-time/
//see gcc stringizing and concatenation
#define LA_DEFINE_SET_VEC_FUNCS(X)                                            \
                                                                              \
inline void la_set ## X ## _vec_dp_mem(                                       \
  LaAddr addr, LaVecStride stride, LaVecCount count, LaVecSkip skip) {        \
  asm volatile("" ::: "memory");                                              \
  asm volatile("lcfgvadrv lr" #X ", %0\n\t"                                   \
    : : "r" (addr));                                                          \
  asm volatile("" ::: "memory");                                              \
  asm volatile("lcfgvlayd (lr" #X "), %0, %1, %2\n\t"                         \
    : : "r" (stride), "r" (count), "r" (skip));                               \
}                                                                             \
                                                                              \
inline void la_set ## X ## _vec_sp_mem(                                       \
  LaAddr addr, LaVecStride stride, LaVecCount count, LaVecSkip skip) {        \
  asm volatile("" ::: "memory");                                              \
  asm volatile("lcfgvadrv lr" #X ", %0\n\t"                                   \
    : : "r" (addr));                                                          \
  asm volatile("" ::: "memory");                                              \
  asm volatile("lcfgvlays (lr" #X "), %0, %1, %2\n\t"                         \
    : : "r" (stride), "r" (count), "r" (skip));                               \
}                                                                             \
                                                                              \
inline void la_set ## X ## _vec_dp_sch(                                       \
  LaAddr addr, LaVecStride stride, LaVecCount count, LaVecSkip skip) {        \
  asm volatile("" ::: "memory");                                              \
  asm volatile("lcfgvadrv lr" #X ", %0\n\t"                                   \
    : : "r" (addr));                                                          \
  asm volatile("" ::: "memory");                                              \
  asm volatile("lcfgvlayd {lr" #X "}, %0, %1, %2\n\t"                         \
    : : "r" (stride), "r" (count), "r" (skip));                               \
}                                                                             \
                                                                              \
inline void la_set ## X ## _vec_sp_sch(                                       \
  LaAddr addr, LaVecStride stride, LaVecCount count, LaVecSkip skip) {        \
  asm volatile("" ::: "memory");                                              \
  asm volatile("lcfgvadrv lr" #X ", %0\n\t"                                   \
    : : "r" (addr));                                                          \
  asm volatile("" ::: "memory");                                              \
  asm volatile("lcfgvlays {lr" #X "}, %0, %1, %2\n\t"                         \
    : : "r" (stride), "r" (count), "r" (skip));                               \
}                                                                             \

LA_DEFINE_SET_VEC_FUNCS(0);
LA_DEFINE_SET_VEC_FUNCS(1);
LA_DEFINE_SET_VEC_FUNCS(2);
LA_DEFINE_SET_VEC_FUNCS(3);
LA_DEFINE_SET_VEC_FUNCS(4);
LA_DEFINE_SET_VEC_FUNCS(5);
LA_DEFINE_SET_VEC_FUNCS(6);
LA_DEFINE_SET_VEC_FUNCS(7);


#define LA_SET_VEC_SWITCH(suffix, idx, addr, stride, count, skip)             \
                                                                              \
  switch(idx){                                                                \
    case 0: la_set0_vec_ ## suffix (addr, stride, count, skip); break;        \
    case 1: la_set1_vec_ ## suffix (addr, stride, count, skip); break;        \
    case 2: la_set2_vec_ ## suffix (addr, stride, count, skip); break;        \
    case 3: la_set3_vec_ ## suffix (addr, stride, count, skip); break;        \
    case 4: la_set4_vec_ ## suffix (addr, stride, count, skip); break;        \
    case 5: la_set5_vec_ ## suffix (addr, stride, count, skip); break;        \
    case 6: la_set6_vec_ ## suffix (addr, stride, count, skip); break;        \
    case 7: la_set7_vec_ ## suffix (addr, stride, count, skip); break;        \
  }

#define LA_DEFINE_SET_VEC_VARIANTS(suffix)                                    \
                                                                              \
inline void la_set_vec_adr_ ## suffix (LaRegIdx idx, LaAddr addr) {           \
  LA_SET_VEC_SWITCH(suffix, idx, addr, DEF_STRIDE, DEF_COUNT, DEF_SKIP);      \
}                                                                             \
                                                                              \
inline void la_set_vec_adrcnt_ ## suffix (LaRegIdx idx, LaAddr addr,          \
  LaVecCount count) {                                                         \
  LA_SET_VEC_SWITCH(suffix, idx, addr, DEF_STRIDE, count, DEF_SKIP);          \
}                                                                             \
                                                                              \
inline void la_set_vec_ ## suffix (LaRegIdx idx, LaAddr addr,                 \
  LaVecStride stride, LaVecCount count, LaVecSkip skip) {                     \
  LA_SET_VEC_SWITCH(suffix, idx, addr, stride, count, skip);                  \
}                                                                             \

LA_DEFINE_SET_VEC_VARIANTS(dp_mem);
LA_DEFINE_SET_VEC_VARIANTS(dp_sch);
LA_DEFINE_SET_VEC_VARIANTS(sp_mem);
LA_DEFINE_SET_VEC_VARIANTS(sp_sch);


#endif // __LA_CORE_LA_SET_VEC_H__
