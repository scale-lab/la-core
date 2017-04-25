//==========================================================================
// Copyright (c) 2017 Samuel Steffl
// All rights reserved.
// 
// CFG vector instructions
//==========================================================================

#ifndef __LA_CORE_LA_SET_SCALAR_H__
#define __LA_CORE_LA_SET_SCALAR_H__

#include "la_core/la_core.h"

//see gcc stringizing and concatenation
#define LA_DEFINE_SET_SCALAR_FUNCS(X)                                         \
                                                                              \
inline void la_set ## X ## _scalar_dp_reg(double scalar) {                    \
  asm volatile("" ::: "memory");                                              \
  asm volatile( "lcfgsfd lr" #X ", %0\n\t" : : "f" (scalar));                 \
}                                                                             \
                                                                              \
inline void la_set ## X ## _scalar_sp_reg(float scalar) {                     \
  asm volatile("" ::: "memory");                                              \
  asm volatile( "lcfgsfs lr" #X ", %0\n\t" : : "f" (scalar));                 \
}                                                                             \
                                                                              \
inline void la_set ## X ## _scalar_dp_mem(LaAddr addr) {                      \
  asm volatile("" ::: "memory");                                              \
  asm volatile( "lcfgsxd (lr" #X "), %0\n\t" : : "r" (addr));                 \
}                                                                             \
                                                                              \
inline void la_set ## X ## _scalar_sp_mem(LaAddr addr) {                      \
  asm volatile("" ::: "memory");                                              \
  asm volatile( "lcfgsxs (lr" #X "), %0\n\t" : : "r" (addr));                 \
}                                                                             \
                                                                              \
inline void la_set ## X ## _scalar_dp_sch(LaAddr addr) {                      \
  asm volatile("" ::: "memory");                                              \
  asm volatile( "lcfgsxd {lr" #X "}, %0\n\t" : : "r" (addr));                 \
}                                                                             \
                                                                              \
inline void la_set ## X ## _scalar_sp_sch(LaAddr addr) {                      \
  asm volatile("" ::: "memory");                                              \
  asm volatile( "lcfgsxs {lr" #X "}, %0\n\t" : : "r" (addr));                 \
}   

LA_DEFINE_SET_SCALAR_FUNCS(0);
LA_DEFINE_SET_SCALAR_FUNCS(1);
LA_DEFINE_SET_SCALAR_FUNCS(2);
LA_DEFINE_SET_SCALAR_FUNCS(3);
LA_DEFINE_SET_SCALAR_FUNCS(4);
LA_DEFINE_SET_SCALAR_FUNCS(5);
LA_DEFINE_SET_SCALAR_FUNCS(6);
LA_DEFINE_SET_SCALAR_FUNCS(7);


#define LA_SET_SCALAR_SWITCH(suffix, idx, scalar)                             \
                                                                              \
  switch(idx){                                                                \
    case 0: la_set0_scalar_ ## suffix (scalar); break;                        \
    case 1: la_set1_scalar_ ## suffix (scalar); break;                        \
    case 2: la_set2_scalar_ ## suffix (scalar); break;                        \
    case 3: la_set3_scalar_ ## suffix (scalar); break;                        \
    case 4: la_set4_scalar_ ## suffix (scalar); break;                        \
    case 5: la_set5_scalar_ ## suffix (scalar); break;                        \
    case 6: la_set6_scalar_ ## suffix (scalar); break;                        \
    case 7: la_set7_scalar_ ## suffix (scalar); break;                        \
  }                                                                           \

                                                                 
inline void la_set_scalar_dp_reg(LaRegIdx idx, double scalar) {
  LA_SET_SCALAR_SWITCH(dp_reg, idx, scalar);
} 
inline void la_set_scalar_sp_reg(LaRegIdx idx, float scalar) {
  LA_SET_SCALAR_SWITCH(sp_reg, idx, scalar);
}  
inline void la_set_scalar_dp_mem(LaRegIdx idx, LaAddr addr) {
  LA_SET_SCALAR_SWITCH(dp_mem, idx, addr);
} 
inline void la_set_scalar_sp_mem(LaRegIdx idx, LaAddr addr) {
  LA_SET_SCALAR_SWITCH(sp_mem, idx, addr);
}  
inline void la_set_scalar_dp_sch(LaRegIdx idx, LaAddr addr) {
  LA_SET_SCALAR_SWITCH(dp_sch, idx, addr);
} 
inline void la_set_scalar_sp_sch(LaRegIdx idx, LaAddr addr) {
  LA_SET_SCALAR_SWITCH(sp_sch, idx, addr);
}
      
#endif // __LA_CORE_LA_SET_SCALAR_H__