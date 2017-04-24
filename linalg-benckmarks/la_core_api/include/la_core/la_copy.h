//==========================================================================
// Copyright (c) 2017 Samuel Steffl
// All rights reserved.
// 
// XFER instructions for moving data from one location to another
//==========================================================================

#ifndef __LA_CORE_LA_COPY_H__
#define __LA_CORE_LA_COPY_H__

#include "la_core/la_core.h"


//see gcc stringizing and concatenation
#define LA_DEFINE_COPY_FUNCS(DST, SRC)                                        \
                                                                              \
inline void la_copy_ ## SRC ## _to_ ## DST (LaCount count) {                  \
  asm volatile("" ::: "memory");                                              \
  asm volatile("lxferdata lr" #DST ", lr" #SRC ", %0\n\t" : : "r" (count));   \
}                                                                             \

#define LA_DEFINE_COPY_FUNCS_BY_DST(dst)                                      \
  LA_DEFINE_COPY_FUNCS(dst, 0);                                               \
  LA_DEFINE_COPY_FUNCS(dst, 1);                                               \
  LA_DEFINE_COPY_FUNCS(dst, 2);                                               \
  LA_DEFINE_COPY_FUNCS(dst, 3);                                               \
  LA_DEFINE_COPY_FUNCS(dst, 4);                                               \
  LA_DEFINE_COPY_FUNCS(dst, 5);                                               \
  LA_DEFINE_COPY_FUNCS(dst, 6);                                               \
  LA_DEFINE_COPY_FUNCS(dst, 7);                                               \

LA_DEFINE_COPY_FUNCS_BY_DST(0);
LA_DEFINE_COPY_FUNCS_BY_DST(1);
LA_DEFINE_COPY_FUNCS_BY_DST(2);
LA_DEFINE_COPY_FUNCS_BY_DST(3);
LA_DEFINE_COPY_FUNCS_BY_DST(4);
LA_DEFINE_COPY_FUNCS_BY_DST(5);
LA_DEFINE_COPY_FUNCS_BY_DST(6);
LA_DEFINE_COPY_FUNCS_BY_DST(7);


#define LA_COPY_SWITCH_BY_SRC(DST, src, count)                                \
                                                                              \
  switch(src){                                                                \
    case 0: la_copy_0_to_ ## DST (count); break;                              \
    case 1: la_copy_1_to_ ## DST (count); break;                              \
    case 2: la_copy_2_to_ ## DST (count); break;                              \
    case 3: la_copy_3_to_ ## DST (count); break;                              \
    case 4: la_copy_4_to_ ## DST (count); break;                              \
    case 5: la_copy_5_to_ ## DST (count); break;                              \
    case 6: la_copy_6_to_ ## DST (count); break;                              \
    case 7: la_copy_7_to_ ## DST (count); break;                              \
  }                                                                           \

inline void la_copy(LaRegIdx dst, LaRegIdx src, LaCount count) {
  switch(dst) {
    case 0: LA_COPY_SWITCH_BY_SRC(0, src, count) break;
    case 1: LA_COPY_SWITCH_BY_SRC(1, src, count) break;
    case 2: LA_COPY_SWITCH_BY_SRC(2, src, count) break;
    case 3: LA_COPY_SWITCH_BY_SRC(3, src, count) break;
    case 4: LA_COPY_SWITCH_BY_SRC(4, src, count) break;
    case 5: LA_COPY_SWITCH_BY_SRC(5, src, count) break;
    case 6: LA_COPY_SWITCH_BY_SRC(6, src, count) break;
    case 7: LA_COPY_SWITCH_BY_SRC(7, src, count) break;
  }
}

#endif // __LA_CORE_LA_COPY_H__
