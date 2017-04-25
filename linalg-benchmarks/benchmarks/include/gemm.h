#ifndef __GEPP_H__
#define __GEPP_H__

#include <intrin.h>

#include "matrix_utilities.h"

static __inline void gepp_u16(double *c, double *a, double *b)
{
  *c += a[0] * b[0] +
        a[1] * b[1] +
        a[2] * b[2] +
        a[3] * b[3] +
        a[4] * b[4] +
        a[5] * b[5] +
        a[6] * b[6] +
        a[7] * b[7] +
        a[8] * b[8] +
        a[9] * b[9] +
        a[10] * b[10] +
        a[11] * b[11] +
        a[12] * b[12] +
        a[13] * b[13] +
        a[14] * b[14] +
        a[15] * b[15];
}

static __inline void gepp_u8(double *c, double *a, double *b)
{
  *c += a[0] * b[0] +
        a[1] * b[1] +
        a[2] * b[2] +
        a[3] * b[3] +
        a[4] * b[4] +
        a[5] * b[5] +
        a[6] * b[6] +
        a[7] * b[7];
}

static __inline void gepp_u4(double *C, double *A, double *B,
  IDX N, IDX M, IDX P, IDX i, IDX j, IDX k)
{
  double *c = &(IDXC(C, i, j, N, M));
  double *a = &(IDXR(A, i, k, N, P));
  double *b = &(IDXC(B, k, j, P, M));
  *c += a[0] * b[0] +
        a[1] * b[1] +
        a[2] * b[2] +
        a[3] * b[3];
}

static __inline void gepp_u2(double *C, double *A, double *B,
  IDX N, IDX M, IDX P, IDX i, IDX j, IDX k)
{
  double *c = &(IDXC(C, i, j, N, M));
  double *a = &(IDXR(A, i, k, N, P));
  double *b = &(IDXC(B, k, j, P, M));
  *c += a[0] * b[0] +
        a[1] * b[1];
}


static __inline void gepp_u1(double *C, double *A, double *B,
  IDX N, IDX M, IDX P, IDX i, IDX j, IDX k)
{
  double *c = &(IDXC(C, i, j, N, M));
  double *a = &(IDXR(A, i, k, N, P));
  double *b = &(IDXC(B, k, j, P, M));
  *c += a[0] * b[0];
}


//==========================================================================
// ASSUMES C=column-major, A=row-major, B=column-major order in memory
//
// C[n][m] := A[n][p] * B[p][m] + C[n][m]
//   for (Noffset < N < (Noffset + Ncount))
//   for (Moffset < N < (Moffset + Mcount))
//   for (Poffset < N < (Poffset + Pcount))
//==========================================================================
static __inline void gepp_d_internal(double *C, double *A, double *B,
  IDX N, IDX M, IDX P,
  IDX Noffset, IDX Ncount, IDX Moffset, IDX Mcount, IDX Poffset, IDX Pcount,
  bool zero)
{
  for (IDX j = Moffset; j < (Moffset + Mcount); ++j) {
    for (IDX i = Noffset; i < (Noffset + Ncount); ++i) {
      IDX k = Poffset;
      IDX k_left = Pcount;
      double *c = &(IDXC(C, i, j, N, M));
      double *a = &(IDXR(A, i, k, N, P));
      double *b = &(IDXC(B, k, j, P, M));

      *c = zero ? 0.0 : *c;
      while(k_left >= 16){
        //gepp_u16(c, a, b)
        *c += a[0] * b[0] +
              a[1] * b[1] +
              a[2] * b[2] +
              a[3] * b[3] +
              a[4] * b[4] +
              a[5] * b[5] +
              a[6] * b[6] +
              a[7] * b[7] +
              a[8] * b[8] +
              a[9] * b[9] +
              a[10] * b[10] +
              a[11] * b[11] +
              a[12] * b[12] +
              a[13] * b[13] +
              a[14] * b[14] +
              a[15] * b[15];
        k += 16;
        k_left -= 16;
        a = &(IDXR(A, i, k, N, P));
        b = &(IDXC(B, k, j, P, M));
      }
      if (k_left >= 8) {
        //gepp_u8(c, a, b);
        *c += a[0] * b[0] +
              a[1] * b[1] +
              a[2] * b[2] +
              a[3] * b[3] +
              a[4] * b[4] +
              a[5] * b[5] +
              a[6] * b[6] +
              a[7] * b[7];
        k += 8;
        k_left -= 8;
        a = &(IDXR(A, i, k, N, P));
        b = &(IDXC(B, k, j, P, M));
      }
      if (k_left >= 4) {
        //gepp_u4(c, a, b);
        *c += a[0] * b[0] +
              a[1] * b[1] +
              a[2] * b[2] +
              a[3] * b[3];
        k += 4;
        k_left -= 4;
        a = &(IDXR(A, i, k, N, P));
        b = &(IDXC(B, k, j, P, M));
      }
      if (k_left >= 2) {
        //gepp_u2(c, a, b);
        *c += a[0] * b[0] +
              a[1] * b[1];
        k += 2;
        k_left -= 2;
        a = &(IDXR(A, i, k, N, P));
        b = &(IDXC(B, k, j, P, M));
      }
      if (k_left == 1) {
        //gepp_u1(c, a, b);
        *c += a[0] * b[0];
      }
    }
  }
}

#define DO_ZERO 1
#define DONT_ZERO 0

#define gepp_min(a,b) (((a) < (b)) ? (a) : (b))

//Typical L1 D$ is around 32kB, so to fit 3 square panels of doubles for 
// A, B, and C, we need 64k/8 = 3x^2. 
// So x = 24 is decent size for doubles. For floats, we can fit 2x as many,
// So x = 48
#define BLOCK_D 64
#define BLOCK_S 128

// C[n][m] = A[n][p] * B[p][m], for (N_offset < N < (N_offset + N_chunk))
void gepp_d(double *C, double *A, double *B,
  IDX N, IDX M, IDX P, IDX Noffset, IDX Ncount, IDX Moffset, IDX Mcount)
{
  IDX Mend = (Moffset + Mcount);
  IDX Nend = (Noffset + Ncount);
//this pragma doesn't seem to do anything
//#pragma omp parallel for
  for (IDX j = Moffset; j<Mend; j += BLOCK_D) {       //M
    for (IDX i = Noffset; i<Nend; i += BLOCK_D) {     //N
      for (IDX k = 0; k<P; k += BLOCK_D) {            //P
        IDX icount = gepp_min(BLOCK_D, (Nend - i));
        IDX jcount = gepp_min(BLOCK_D, (Mend - j));
        IDX kcount = gepp_min(BLOCK_D, (P - k));

        //gepp_d_internal(C, A, B, N, M, P, i, icount, j, jcount, 
        //  k, kcount, do_zero);
        for (IDX __j = j; __j < (j + jcount); ++__j) {
          for (IDX __i = i; __i < (i + icount); ++__i) {
            IDX k_left = kcount;
            double *c = &(IDXC(C, __i, __j, N, M));
            double *a = &(IDXR(A, __i, k, N, P));
            double *b = &(IDXC(B, k, __j, P, M));

            *c = (k == 0) ? 0.0 : *c;

            __m256d packed_c;
            __m256d packed_a[8];
            __m256d packed_b[8];
            while (k_left >= 32) {
              packed_a[0] = _mm256_load_pd(a);
              packed_a[1] = _mm256_load_pd(a+4);
              packed_a[2] = _mm256_load_pd(a+8);
              packed_a[3] = _mm256_load_pd(a+12);
              packed_a[4] = _mm256_load_pd(a+16);
              packed_a[5] = _mm256_load_pd(a+20);
              packed_a[6] = _mm256_load_pd(a+24);
              packed_a[7] = _mm256_load_pd(a+28);

              packed_b[0] = _mm256_load_pd(b);
              packed_b[1] = _mm256_load_pd(b+4);
              packed_b[2] = _mm256_load_pd(b+8);
              packed_b[3] = _mm256_load_pd(b+12);
              packed_b[4] = _mm256_load_pd(b+16);
              packed_b[5] = _mm256_load_pd(b+20);
              packed_b[6] = _mm256_load_pd(b+24);
              packed_b[7] = _mm256_load_pd(b+28);

              packed_c    = _mm256_add_pd(
                              _mm256_add_pd(
                                _mm256_add_pd(
                                  _mm256_mul_pd(packed_a[0], packed_b[0]),
                                  _mm256_mul_pd(packed_a[1], packed_b[1])
                                ),
                                _mm256_add_pd(
                                  _mm256_mul_pd(packed_a[2], packed_b[2]),
                                  _mm256_mul_pd(packed_a[3], packed_b[3])
                                )
                              ),
                              _mm256_add_pd(
                                _mm256_add_pd(
                                  _mm256_mul_pd(packed_a[4], packed_b[4]),
                                  _mm256_mul_pd(packed_a[5], packed_b[5])
                                ),
                                _mm256_add_pd(
                                  _mm256_mul_pd(packed_a[6], packed_b[6]),
                                  _mm256_mul_pd(packed_a[7], packed_b[7])
                                )
                              )
                            );
              packed_c = _mm256_hadd_pd(packed_c, packed_c);
              *c += ((double*)&packed_c)[0] + ((double*)&packed_c)[2];
              /*
              *c += a[0] * b[0] +
                a[1] * b[1] +
                a[2] * b[2] +
                a[3] * b[3] +
                a[4] * b[4] +
                a[5] * b[5] +
                a[6] * b[6] +
                a[7] * b[7] +
                a[8] * b[8] +
                a[9] * b[9] +
                a[10] * b[10] +
                a[11] * b[11] +
                a[12] * b[12] +
                a[13] * b[13] +
                a[14] * b[14] +
                a[15] * b[15] +
                a[16] * b[16] +
                a[17] * b[17] +
                a[18] * b[18] +
                a[19] * b[19] +
                a[20] * b[20] +
                a[21] * b[21] +
                a[22] * b[22] +
                a[23] * b[23] +
                a[24] * b[24] +
                a[25] * b[25] +
                a[26] * b[26] +
                a[27] * b[27] +
                a[28] * b[28] +
                a[29] * b[29] +
                a[30] * b[30] +
                a[31] * b[31];
                */
              k_left -= 32;
              if (k_left == 0) {
                goto END_LOOP;
              }
              a += 32;
              b += 32;
            }
            if (k_left >= 16) {
              packed_a[0] = _mm256_load_pd(a);
              packed_a[1] = _mm256_load_pd(a + 4);
              packed_a[2] = _mm256_load_pd(a + 8);
              packed_a[3] = _mm256_load_pd(a + 12);

              packed_b[0] = _mm256_load_pd(b);
              packed_b[1] = _mm256_load_pd(b + 4);
              packed_b[2] = _mm256_load_pd(b + 8);
              packed_b[3] = _mm256_load_pd(b + 12);

              packed_c    =  _mm256_add_pd(
                              _mm256_add_pd(
                                _mm256_mul_pd(packed_a[0], packed_b[0]),
                                _mm256_mul_pd(packed_a[1], packed_b[1])
                              ),
                              _mm256_add_pd(
                                _mm256_mul_pd(packed_a[2], packed_b[2]),
                                _mm256_mul_pd(packed_a[3], packed_b[3])
                              )
                            );
              packed_c = _mm256_hadd_pd(packed_c, packed_c);
              *c += ((double*)&packed_c)[0] + ((double*)&packed_c)[2];
              /*
              //gepp_u16(c, a, b)
              *c += a[0] * b[0] +
                a[1] * b[1] +
                a[2] * b[2] +
                a[3] * b[3] +
                a[4] * b[4] +
                a[5] * b[5] +
                a[6] * b[6] +
                a[7] * b[7] +
                a[8] * b[8] +
                a[9] * b[9] +
                a[10] * b[10] +
                a[11] * b[11] +
                a[12] * b[12] +
                a[13] * b[13] +
                a[14] * b[14] +
                a[15] * b[15];
                */
              k_left -= 16;
              if (k_left == 0) {
                goto END_LOOP;
              }
              a += 16;
              b += 16;
            }
            if (k_left >= 8) {
              packed_a[0] = _mm256_load_pd(a);
              packed_a[1] = _mm256_load_pd(a + 4);

              packed_b[0] = _mm256_load_pd(b);
              packed_b[1] = _mm256_load_pd(b + 4);

              packed_c    = _mm256_add_pd(
                            _mm256_mul_pd(packed_a[0], packed_b[0]),
                            _mm256_mul_pd(packed_a[1], packed_b[1])
                          );
              packed_c = _mm256_hadd_pd(packed_c, packed_c);
              *c += ((double*)&packed_c)[0] + ((double*)&packed_c)[2];
              /*
              //gepp_u8(c, a, b);
              *c += a[0] * b[0] +
                a[1] * b[1] +
                a[2] * b[2] +
                a[3] * b[3] +
                a[4] * b[4] +
                a[5] * b[5] +
                a[6] * b[6] +
                a[7] * b[7];
                */
              k_left -= 8;
              if (k_left == 0) {
                goto END_LOOP;
              }
              a += 8;
              b += 8;
            }
            if (k_left >= 4) {
              packed_a[0] = _mm256_load_pd(a);

              packed_b[0] = _mm256_load_pd(b);

              packed_c = _mm256_mul_pd(packed_a[1], packed_b[1]);
              packed_c = _mm256_hadd_pd(packed_c, packed_c);
              *c += ((double*)&packed_c)[0] + ((double*)&packed_c)[2];
              /*
              //gepp_u4(c, a, b);
              *c += a[0] * b[0] +
                a[1] * b[1] +
                a[2] * b[2] +
                a[3] * b[3];
                */
              k_left -= 4;
              if (k_left == 0) {
                goto END_LOOP;
              }
              a += 4;
              b += 4;
            }
            if (k_left >= 2) {
              //gepp_u2(c, a, b);
              *c += a[0] * b[0] +
                a[1] * b[1];
              k_left -= 2;
              if (k_left == 0) {
                goto END_LOOP;
              }
              a += 2;
              b += 2;
            }
            if (k_left == 1) {
              //gepp_u1(c, a, b);
              *c += a[0] * b[0];
            }
          END_LOOP:
            ;
          }
        }
      }
    }
  }
}

#endif //__GEPP_H__