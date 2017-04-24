//==========================================================================
// Copyright (c) 2017 Samuel Steffl
// All rights reserved.
// 
// CFG sparse instructions
//==========================================================================

#ifndef __LA_CORE_LA_SET_SPV_H__
#define __LA_CORE_LA_SET_SPV_H__

#include "la_core/la_core.h"

//see explicit compiler barriers section:
//  http://preshing.com/20120625/memory-ordering-at-compile-time/
//see gcc stringizing and concatenation
#define LA_DEFINE_SET_SPV_FUNCS(X)                                            \
                                                                              \
inline void la_set ## X ## _spv_nrm_dp_mem(                                   \
  LaAddr data_ptr, LaAddr idx_ptr, LaAddr jdx_ptr,                            \
  LaSpvCount idx_cnt, LaSpvCount jdx_cnt, LaSpvCount data_skip) {             \
  asm volatile("" ::: "memory");                                              \
  asm volatile("lcfgvadrs lr" #X ", %0, %1, %2\n\t"                           \
    : : "r" (data_ptr), "r" (idx_ptr), "r" (jdx_ptr));                        \
  asm volatile("" ::: "memory");                                              \
  asm volatile("lcfgvlayd (lr" #X "), %0, %1, %2\n\t"                         \
    : : "r" (idx_cnt), "r" (jdx_cnt), "r" (data_skip));                       \
}                                                                             \
                                                                              \
inline void la_set ## X ## _spv_tns_dp_mem(                                   \
  LaAddr data_ptr, LaAddr idx_ptr, LaAddr jdx_ptr,                            \
  LaSpvCount idx_cnt, LaSpvCount jdx_cnt, LaSpvCount data_skip) {             \
  asm volatile("" ::: "memory");                                              \
  asm volatile("lcfgvadrt lr" #X ", %0, %1, %2\n\t"                           \
    : : "r" (data_ptr), "r" (idx_ptr), "r" (jdx_ptr));                        \
  asm volatile("" ::: "memory");                                              \
  asm volatile("lcfgvlayd (lr" #X "), %0, %1, %2\n\t"                         \
    : : "r" (idx_cnt), "r" (jdx_cnt), "r" (data_skip));                       \
}                                                                             \
                                                                              \
inline void la_set ## X ## _spv_nrm_sp_mem(                                   \
  LaAddr data_ptr, LaAddr idx_ptr, LaAddr jdx_ptr,                            \
  LaSpvCount idx_cnt, LaSpvCount jdx_cnt, LaSpvCount data_skip) {             \
  asm volatile("" ::: "memory");                                              \
  asm volatile("lcfgvadrs lr" #X ", %0, %1, %2\n\t"                           \
    : : "r" (data_ptr), "r" (idx_ptr), "r" (jdx_ptr));                        \
  asm volatile("" ::: "memory");                                              \
  asm volatile("lcfgvlays (lr" #X "), %0, %1, %2\n\t"                         \
    : : "r" (idx_cnt), "r" (jdx_cnt), "r" (data_skip));                       \
}                                                                             \
                                                                              \
inline void la_set ## X ## _spv_tns_sp_mem(                                   \
  LaAddr data_ptr, LaAddr idx_ptr, LaAddr jdx_ptr,                            \
  LaSpvCount idx_cnt, LaSpvCount jdx_cnt, LaSpvCount data_skip) {             \
  asm volatile("" ::: "memory");                                              \
  asm volatile("lcfgvadrt lr" #X ", %0, %1, %2\n\t"                           \
    : : "r" (data_ptr), "r" (idx_ptr), "r" (jdx_ptr));                        \
  asm volatile("" ::: "memory");                                              \
  asm volatile("lcfgvlays (lr" #X "), %0, %1, %2\n\t"                         \
    : : "r" (idx_cnt), "r" (jdx_cnt), "r" (data_skip));                       \
}                                                                             \
                                                                              \
inline void la_set ## X ## _spv_nrm_dp_sch(                                   \
  LaAddr data_ptr, LaAddr idx_ptr, LaAddr jdx_ptr,                            \
  LaSpvCount idx_cnt, LaSpvCount jdx_cnt, LaSpvCount data_skip) {             \
  asm volatile("" ::: "memory");                                              \
  asm volatile("lcfgvadrs lr" #X ", %0, %1, %2\n\t"                           \
    : : "r" (data_ptr), "r" (idx_ptr), "r" (jdx_ptr));                        \
  asm volatile("" ::: "memory");                                              \
  asm volatile("lcfgvlayd {lr" #X "}, %0, %1, %2\n\t"                         \
    : : "r" (idx_cnt), "r" (jdx_cnt), "r" (data_skip));                       \
}                                                                             \
                                                                              \
inline void la_set ## X ## _spv_tns_dp_sch(                                   \
  LaAddr data_ptr, LaAddr idx_ptr, LaAddr jdx_ptr,                            \
  LaSpvCount idx_cnt, LaSpvCount jdx_cnt, LaSpvCount data_skip) {             \
  asm volatile("" ::: "memory");                                              \
  asm volatile("lcfgvadrt lr" #X ", %0, %1, %2\n\t"                           \
    : : "r" (data_ptr), "r" (idx_ptr), "r" (jdx_ptr));                        \
  asm volatile("" ::: "memory");                                              \
  asm volatile("lcfgvlayd {lr" #X "}, %0, %1, %2\n\t"                         \
    : : "r" (idx_cnt), "r" (jdx_cnt), "r" (data_skip));                       \
}                                                                             \
                                                                              \
inline void la_set ## X ## _spv_nrm_sp_sch(                                   \
  LaAddr data_ptr, LaAddr idx_ptr, LaAddr jdx_ptr,                            \
  LaSpvCount idx_cnt, LaSpvCount jdx_cnt, LaSpvCount data_skip) {             \
  asm volatile("" ::: "memory");                                              \
  asm volatile("lcfgvadrs lr" #X ", %0, %1, %2\n\t"                           \
    : : "r" (data_ptr), "r" (idx_ptr), "r" (jdx_ptr));                        \
  asm volatile("" ::: "memory");                                              \
  asm volatile("lcfgvlays {lr" #X "}, %0, %1, %2\n\t"                         \
    : : "r" (idx_cnt), "r" (jdx_cnt), "r" (data_skip));                       \
}                                                                             \
                                                                              \
inline void la_set ## X ## _spv_tns_sp_sch(                                   \
  LaAddr data_ptr, LaAddr idx_ptr, LaAddr jdx_ptr,                            \
  LaSpvCount idx_cnt, LaSpvCount jdx_cnt, LaSpvCount data_skip) {             \
  asm volatile("" ::: "memory");                                              \
  asm volatile("lcfgvadrt lr" #X ", %0, %1, %2\n\t"                           \
    : : "r" (data_ptr), "r" (idx_ptr), "r" (jdx_ptr));                        \
  asm volatile("" ::: "memory");                                              \
  asm volatile("lcfgvlays {lr" #X "}, %0, %1, %2\n\t"                         \
    : : "r" (idx_cnt), "r" (jdx_cnt), "r" (data_skip));                       \
}                                                                             \

LA_DEFINE_SET_SPV_FUNCS(0);
LA_DEFINE_SET_SPV_FUNCS(1);
LA_DEFINE_SET_SPV_FUNCS(2);
LA_DEFINE_SET_SPV_FUNCS(3);
LA_DEFINE_SET_SPV_FUNCS(4);
LA_DEFINE_SET_SPV_FUNCS(5);
LA_DEFINE_SET_SPV_FUNCS(6);
LA_DEFINE_SET_SPV_FUNCS(7);


#define LA_SET_SPV_SWITCH(suffix, idx, dptr, iptr, jptr, icnt, jcnt, dskip)      \
                                                                                 \
  switch(idx){                                                                   \
    case 0: la_set0_spv_ ## suffix (dptr, iptr, jptr, icnt, jcnt, dskip); break; \
    case 1: la_set1_spv_ ## suffix (dptr, iptr, jptr, icnt, jcnt, dskip); break; \
    case 2: la_set2_spv_ ## suffix (dptr, iptr, jptr, icnt, jcnt, dskip); break; \
    case 3: la_set3_spv_ ## suffix (dptr, iptr, jptr, icnt, jcnt, dskip); break; \
    case 4: la_set4_spv_ ## suffix (dptr, iptr, jptr, icnt, jcnt, dskip); break; \
    case 5: la_set5_spv_ ## suffix (dptr, iptr, jptr, icnt, jcnt, dskip); break; \
    case 6: la_set6_spv_ ## suffix (dptr, iptr, jptr, icnt, jcnt, dskip); break; \
    case 7: la_set7_spv_ ## suffix (dptr, iptr, jptr, icnt, jcnt, dskip); break; \
  }

#define LA_DEFINE_SET_SPV_VARIANTS(suffix)                                    \
                                                                              \
inline void la_set_spv_ ## suffix (LaRegIdx idx,                              \
  LaAddr data_ptr, LaAddr idx_ptr, LaAddr jdx_ptr,                            \
  LaSpvCount idx_cnt, LaSpvCount jdx_cnt, LaSpvCount data_skip) {             \
  LA_SET_SPV_SWITCH(suffix, idx, data_ptr, idx_ptr, jdx_ptr, idx_cnt,         \
    jdx_cnt, data_skip);                                                      \
}                                                                             \

LA_DEFINE_SET_SPV_VARIANTS(nrm_dp_mem);
LA_DEFINE_SET_SPV_VARIANTS(nrm_dp_sch);
LA_DEFINE_SET_SPV_VARIANTS(nrm_sp_mem);
LA_DEFINE_SET_SPV_VARIANTS(nrm_sp_sch);
LA_DEFINE_SET_SPV_VARIANTS(tns_dp_mem);
LA_DEFINE_SET_SPV_VARIANTS(tns_dp_sch);
LA_DEFINE_SET_SPV_VARIANTS(tns_sp_mem);
LA_DEFINE_SET_SPV_VARIANTS(tns_sp_sch);


#endif // __LA_CORE_LA_SET_SPV_H__
