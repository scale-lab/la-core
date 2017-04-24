//==========================================================================
// Copyright (c) 2017 Samuel Steffl
// All rights reserved.
// 
// Intel AVX implementation of BLAS-3 DTRSM function
//==========================================================================

#include "trsm.h"

#include "matrix_utilities.h"

//==========================================================================
// LEFT LOWER KERNELS (traverse top-left to bottom right)
//==========================================================================

//A row-major, B col-major order in memory. Solves for X in place of B
static void dtrsm_left_lower(double *A, double *B,
  IDX M, IDX B_rows, IDX B_cols,
  IDX row_topleft, IDX row_count, IDX col_topleft, IDX col_count,
  bool unit, double alpha)
{
  IDX j_end = row_topleft + row_count;
  IDX k_end = col_topleft + col_count;
  for (IDX k = col_topleft; k < k_end; ++k) {
    for (IDX j = row_topleft; j < j_end; ++j) {
      double *Xjk = &(IDXC(B, j, k, B_rows, B_cols));
      double Ajj = IDXR(A, j, j, M, M);
      
      *Xjk *= alpha;

      //Loop Below is heavily unrolled loop of the following:
      //  for(IDX i = 0; i<j; ++i){
      //    *Xjk -= Aji[i]*Xik[i]
      //  }

      IDX i = 0;
      IDX i_left = j;
      double *Xik = &(IDXC(B, i, k, B_rows, B_cols));
      double *Aji = &(IDXR(A, j, i, M, M));

      __m256d packed_Xjk;
      __m256d packed_Aji[8];
      __m256d packed_Xik[8];

      while (i_left >= 32) {
        packed_Aji[0] = _mm256_load_pd(Aji);
        packed_Aji[1] = _mm256_load_pd(Aji + 4);
        packed_Aji[2] = _mm256_load_pd(Aji + 8);
        packed_Aji[3] = _mm256_load_pd(Aji + 12);
        packed_Aji[4] = _mm256_load_pd(Aji + 16);
        packed_Aji[5] = _mm256_load_pd(Aji + 20);
        packed_Aji[6] = _mm256_load_pd(Aji + 24);
        packed_Aji[7] = _mm256_load_pd(Aji + 28);

        packed_Xik[0] = _mm256_load_pd(Xik);
        packed_Xik[1] = _mm256_load_pd(Xik + 4);
        packed_Xik[2] = _mm256_load_pd(Xik + 8);
        packed_Xik[3] = _mm256_load_pd(Xik + 12);
        packed_Xik[4] = _mm256_load_pd(Xik + 16);
        packed_Xik[5] = _mm256_load_pd(Xik + 20);
        packed_Xik[6] = _mm256_load_pd(Xik + 24);
        packed_Xik[7] = _mm256_load_pd(Xik + 28);

        packed_Xjk = _mm256_add_pd(
          _mm256_add_pd(
            _mm256_add_pd(
              _mm256_mul_pd(packed_Aji[0], packed_Xik[0]),
              _mm256_mul_pd(packed_Aji[1], packed_Xik[1])
            ),
            _mm256_add_pd(
              _mm256_mul_pd(packed_Aji[2], packed_Xik[2]),
              _mm256_mul_pd(packed_Aji[3], packed_Xik[3])
            )
          ),
          _mm256_add_pd(
            _mm256_add_pd(
              _mm256_mul_pd(packed_Aji[4], packed_Xik[4]),
              _mm256_mul_pd(packed_Aji[5], packed_Xik[5])
            ),
            _mm256_add_pd(
              _mm256_mul_pd(packed_Aji[6], packed_Xik[6]),
              _mm256_mul_pd(packed_Aji[7], packed_Xik[7])
            )
          )
        );
        packed_Xjk = _mm256_hadd_pd(packed_Xjk, packed_Xjk);
        *Xjk -= ((double*)&packed_Xjk)[0] + ((double*)&packed_Xjk)[2];
        i_left -= 32;
        if (i_left == 0) {
          goto END_LOOP;
        }
        Aji += 32;
        Xik += 32;
      }
      if (i_left >= 16) {
        packed_Aji[0] = _mm256_load_pd(Aji);
        packed_Aji[1] = _mm256_load_pd(Aji + 4);
        packed_Aji[2] = _mm256_load_pd(Aji + 8);
        packed_Aji[3] = _mm256_load_pd(Aji + 12);

        packed_Xik[0] = _mm256_load_pd(Xik);
        packed_Xik[1] = _mm256_load_pd(Xik + 4);
        packed_Xik[2] = _mm256_load_pd(Xik + 8);
        packed_Xik[3] = _mm256_load_pd(Xik + 12);

        packed_Xjk = _mm256_add_pd(
          _mm256_add_pd(
            _mm256_mul_pd(packed_Aji[0], packed_Xik[0]),
            _mm256_mul_pd(packed_Aji[1], packed_Xik[1])
          ),
          _mm256_add_pd(
            _mm256_mul_pd(packed_Aji[2], packed_Xik[2]),
            _mm256_mul_pd(packed_Aji[3], packed_Xik[3])
          )
        );
        packed_Xjk = _mm256_hadd_pd(packed_Xjk, packed_Xjk);
        *Xjk -= ((double*)&packed_Xjk)[0] + ((double*)&packed_Xjk)[2];
        i_left -= 16;
        if (i_left == 0) {
          goto END_LOOP;
        }
        Aji += 16;
        Xik += 16;
      }
      if (i_left >= 8) {
        packed_Aji[0] = _mm256_load_pd(Aji);
        packed_Aji[1] = _mm256_load_pd(Aji + 4);

        packed_Xik[0] = _mm256_load_pd(Xik);
        packed_Xik[1] = _mm256_load_pd(Xik + 4);

        packed_Xjk = _mm256_add_pd(
          _mm256_mul_pd(packed_Aji[0], packed_Xik[0]),
          _mm256_mul_pd(packed_Aji[1], packed_Xik[1])
        );
        packed_Xjk = _mm256_hadd_pd(packed_Xjk, packed_Xjk);
        *Xjk -= ((double*)&packed_Xjk)[0] + ((double*)&packed_Xjk)[2];
        i_left -= 8;
        if (i_left == 0) {
          goto END_LOOP;
        }
        Aji += 8;
        Xik += 8;
      }
      if (i_left >= 4) {
        packed_Aji[0] = _mm256_load_pd(Aji);

        packed_Xik[0] = _mm256_load_pd(Xik);

        packed_Xjk = _mm256_mul_pd(packed_Aji[0], packed_Xik[0]);
        packed_Xjk = _mm256_hadd_pd(packed_Xjk, packed_Xjk);
        *Xjk -= ((double*)&packed_Xjk)[0] + ((double*)&packed_Xjk)[2];
        i_left -= 4;
        if (i_left == 0) {
          goto END_LOOP;
        }
        Aji += 4;
        Xik += 4;
      }
      if (i_left >= 2) {
        *Xjk -= Aji[0] * Xik[0] +
                Aji[1] * Xik[1];
        i_left -= 2;
        if (i_left == 0) {
          goto END_LOOP;
        }
        Aji += 2;
        Xik += 2;
      }
      if (i_left == 1) {
        *Xjk -= Aji[0] * Xik[0];
      }
    END_LOOP:
      if (!unit) {
        *Xjk /= Ajj;
      };
    }
  }
}

//==========================================================================
// RIGHT UPPER KERNELS (traverse top-left to bottom right)
//==========================================================================

//A col-major, B row-major order in memory. Solves for X in place of B
static void dtrsm_right_upper(double *A, double *B,
  IDX M, IDX B_rows, IDX B_cols,
  IDX row_topleft, IDX row_count, IDX col_topleft, IDX col_count,
  bool unit, double alpha)
{
  IDX k_end = row_topleft + row_count;
  IDX i_end = col_topleft + col_count;
  for (IDX k = row_topleft; k < k_end; ++k) {
    for (IDX i = col_topleft; i < i_end; ++i) {
      double *Xki = &(IDXR(B, k, i, B_rows, B_cols));
      double Aii = IDXC(A, i, i, M, M);
      
      *Xki *= alpha;

      //Loop Below is heavily unrolled loop of the following:
      //  for(IDX j = 0; j<i; ++j){
      //    *Xki -= Xkj[i]*Aji[i]
      //  }

      IDX j = 0;
      IDX j_left = i;
      double *Xkj = &(IDXR(B, k, j, B_rows, B_cols));
      double *Aji = &(IDXC(A, j, i, M, M));

      __m256d packed_Xki;
      __m256d packed_Xkj[8];
      __m256d packed_Aji[8];

      while (j_left >= 32) {
        packed_Xkj[0] = _mm256_load_pd(Xkj);
        packed_Xkj[1] = _mm256_load_pd(Xkj + 4);
        packed_Xkj[2] = _mm256_load_pd(Xkj + 8);
        packed_Xkj[3] = _mm256_load_pd(Xkj + 12);
        packed_Xkj[4] = _mm256_load_pd(Xkj + 16);
        packed_Xkj[5] = _mm256_load_pd(Xkj + 20);
        packed_Xkj[6] = _mm256_load_pd(Xkj + 24);
        packed_Xkj[7] = _mm256_load_pd(Xkj + 28);

        packed_Aji[0] = _mm256_load_pd(Aji);
        packed_Aji[1] = _mm256_load_pd(Aji + 4);
        packed_Aji[2] = _mm256_load_pd(Aji + 8);
        packed_Aji[3] = _mm256_load_pd(Aji + 12);
        packed_Aji[4] = _mm256_load_pd(Aji + 16);
        packed_Aji[5] = _mm256_load_pd(Aji + 20);
        packed_Aji[6] = _mm256_load_pd(Aji + 24);
        packed_Aji[7] = _mm256_load_pd(Aji + 28);

        packed_Xki = _mm256_add_pd(
          _mm256_add_pd(
            _mm256_add_pd(
              _mm256_mul_pd(packed_Xkj[0], packed_Aji[0]),
              _mm256_mul_pd(packed_Xkj[1], packed_Aji[1])
            ), 
            _mm256_add_pd(
              _mm256_mul_pd(packed_Xkj[2], packed_Aji[2]),
              _mm256_mul_pd(packed_Xkj[3], packed_Aji[3])
            )
          ),
          _mm256_add_pd(
            _mm256_add_pd(
              _mm256_mul_pd(packed_Xkj[4], packed_Aji[4]),
              _mm256_mul_pd(packed_Xkj[5], packed_Aji[5])
            ),
            _mm256_add_pd(
              _mm256_mul_pd(packed_Xkj[6], packed_Aji[6]),
              _mm256_mul_pd(packed_Xkj[7], packed_Aji[7])
            )
          )
        );
        packed_Xki = _mm256_hadd_pd(packed_Xki, packed_Xki);
        *Xki -= ((double*)&packed_Xki)[0] + ((double*)&packed_Xki)[2];
        j_left -= 32;
        if (j_left == 0) {
          goto END_LOOP;
        }
        Xkj += 32;
        Aji += 32;
      }
      if (j_left >= 16) {
        packed_Xkj[0] = _mm256_load_pd(Xkj);
        packed_Xkj[1] = _mm256_load_pd(Xkj + 4);
        packed_Xkj[2] = _mm256_load_pd(Xkj + 8);
        packed_Xkj[3] = _mm256_load_pd(Xkj + 12);

        packed_Aji[0] = _mm256_load_pd(Aji);
        packed_Aji[1] = _mm256_load_pd(Aji + 4);
        packed_Aji[2] = _mm256_load_pd(Aji + 8);
        packed_Aji[3] = _mm256_load_pd(Aji + 12);

        packed_Xki = _mm256_add_pd(
          _mm256_add_pd(
            _mm256_mul_pd(packed_Xkj[0], packed_Aji[0]),
            _mm256_mul_pd(packed_Xkj[1], packed_Aji[1])
          ),
          _mm256_add_pd(
            _mm256_mul_pd(packed_Xkj[2], packed_Aji[2]),
            _mm256_mul_pd(packed_Xkj[3], packed_Aji[3])
          )
        );
        packed_Xki = _mm256_hadd_pd(packed_Xki, packed_Xki);
        *Xki -= ((double*)&packed_Xki)[0] + ((double*)&packed_Xki)[2];
        j_left -= 16;
        if (j_left == 0) {
          goto END_LOOP;
        }
        Xkj += 16;
        Aji += 16;
      }
      if (j_left >= 8) {
        packed_Xkj[0] = _mm256_load_pd(Xkj);
        packed_Xkj[1] = _mm256_load_pd(Xkj+4);

        packed_Aji[0] = _mm256_load_pd(Aji);
        packed_Aji[1] = _mm256_load_pd(Aji+4);

        packed_Xki = _mm256_add_pd(
          _mm256_mul_pd(packed_Xkj[0], packed_Aji[0]),
          _mm256_mul_pd(packed_Xkj[1], packed_Aji[1])
        );
        packed_Xki = _mm256_hadd_pd(packed_Xki, packed_Xki);
        *Xki -= ((double*)&packed_Xki)[0] + ((double*)&packed_Xki)[2];
        j_left -= 8;
        if (j_left == 0) {
          goto END_LOOP;
        }
        Xkj += 8;
        Aji += 8;
      }
      if (j_left >= 4) {
        packed_Xkj[0] = _mm256_load_pd(Xkj);

        packed_Aji[0] = _mm256_load_pd(Aji);

        packed_Xki = _mm256_mul_pd(packed_Xkj[0], packed_Aji[0]);
        packed_Xki = _mm256_hadd_pd(packed_Xki, packed_Xki);
        *Xki -= ((double*)&packed_Xki)[0] + ((double*)&packed_Xki)[2];
        j_left -= 4;
        if (j_left == 0) {
          goto END_LOOP;
        }
        Xkj += 4;
        Aji += 4;
      }
      if (j_left >= 2) {
        *Xki -= Xkj[0] * Aji[0] +
                Xkj[1] * Aji[1];
        j_left -= 2;
        if (j_left == 0) {
          goto END_LOOP;
        }
        Xkj += 2;
        Aji += 2;
      }
      if (j_left == 1) {
        *Xki -= Xkj[0] * Aji[0];
      }
    END_LOOP:
      if (!unit) {
        *Xki /= Aii;
      };
    }
  }
}

//==========================================================================
// LEFT UPPER KERNELS (traverse bottom-right to top-left)
//==========================================================================

//A row-major, B col-major order in memory. Solves for X in place of B
static void dtrsm_left_upper(double *A, double *B,
  IDX M, IDX B_rows, IDX B_cols,
  IDX row_topleft, IDX row_count, IDX col_topleft, IDX col_count,
  bool unit, double alpha)
{
  IDX j_end = row_topleft + row_count;
  IDX k_end = col_topleft + col_count;
  for (IDX k = col_topleft; k < k_end; ++k) {
    for (IDX j = j_end - 1; j >= row_topleft; --j) {
      double *Xjk = &(IDXC(B, j, k, B_rows, B_cols));
      double Ajj = IDXR(A, j, j, M, M);
      
      *Xjk *= alpha;

      //Loop Below is heavily unrolled loop of the following (in reverse):
      //  for(IDX i=j+1; i<M; ++i){
      //    *Xjk -= Aji[i]*Xik[i]
      //  }

      IDX i = B_rows;
      IDX i_left = (i-j)-1;
      double *Aji = &(IDXR(A, j, i, M, M));
      double *Xik = &(IDXC(B, i, k, B_rows, B_cols));

      __m256d packed_Xjk;
      __m256d packed_Aji[8];
      __m256d packed_Xik[8];

      while (i_left >= 32) {
        packed_Aji[0] = _mm256_load_pd(Aji - 4);
        packed_Aji[1] = _mm256_load_pd(Aji - 8);
        packed_Aji[2] = _mm256_load_pd(Aji - 12);
        packed_Aji[3] = _mm256_load_pd(Aji - 16);
        packed_Aji[4] = _mm256_load_pd(Aji - 20);
        packed_Aji[5] = _mm256_load_pd(Aji - 24);
        packed_Aji[6] = _mm256_load_pd(Aji - 28);
        packed_Aji[7] = _mm256_load_pd(Aji - 32);

        packed_Xik[0] = _mm256_load_pd(Xik - 4);
        packed_Xik[1] = _mm256_load_pd(Xik - 8);
        packed_Xik[2] = _mm256_load_pd(Xik - 12);
        packed_Xik[3] = _mm256_load_pd(Xik - 16);
        packed_Xik[4] = _mm256_load_pd(Xik - 20);
        packed_Xik[5] = _mm256_load_pd(Xik - 24);
        packed_Xik[6] = _mm256_load_pd(Xik - 28);
        packed_Xik[7] = _mm256_load_pd(Xik - 32);

        packed_Xjk = _mm256_add_pd(
          _mm256_add_pd(
            _mm256_add_pd(
              _mm256_mul_pd(packed_Aji[0], packed_Xik[0]),
              _mm256_mul_pd(packed_Aji[1], packed_Xik[1])
            ),
            _mm256_add_pd(
              _mm256_mul_pd(packed_Aji[2], packed_Xik[2]),
              _mm256_mul_pd(packed_Aji[3], packed_Xik[3])
            )
          ),
          _mm256_add_pd(
            _mm256_add_pd(
              _mm256_mul_pd(packed_Aji[4], packed_Xik[4]),
              _mm256_mul_pd(packed_Aji[5], packed_Xik[5])
            ),
            _mm256_add_pd(
              _mm256_mul_pd(packed_Aji[6], packed_Xik[6]),
              _mm256_mul_pd(packed_Aji[7], packed_Xik[7])
            )
          )
        );
        packed_Xjk = _mm256_hadd_pd(packed_Xjk, packed_Xjk);
        *Xjk -= ((double*)&packed_Xjk)[0] + ((double*)&packed_Xjk)[2];
        i_left -= 32;
        if (i_left == 0) {
          goto END_LOOP;
        }
        Aji -= 32;
        Xik -= 32;
      }
      if (i_left >= 16) {
        packed_Aji[0] = _mm256_load_pd(Aji - 4);
        packed_Aji[1] = _mm256_load_pd(Aji - 8);
        packed_Aji[2] = _mm256_load_pd(Aji - 12);
        packed_Aji[3] = _mm256_load_pd(Aji - 16);

        packed_Xik[0] = _mm256_load_pd(Xik - 4);
        packed_Xik[1] = _mm256_load_pd(Xik - 8);
        packed_Xik[2] = _mm256_load_pd(Xik - 12);
        packed_Xik[3] = _mm256_load_pd(Xik - 16);

        packed_Xjk = _mm256_add_pd(
          _mm256_add_pd(
            _mm256_mul_pd(packed_Aji[0], packed_Xik[0]),
            _mm256_mul_pd(packed_Aji[1], packed_Xik[1])
          ),
          _mm256_add_pd(
            _mm256_mul_pd(packed_Aji[2], packed_Xik[2]),
            _mm256_mul_pd(packed_Aji[3], packed_Xik[3])
          )
        );
        packed_Xjk = _mm256_hadd_pd(packed_Xjk, packed_Xjk);
        *Xjk -= ((double*)&packed_Xjk)[0] + ((double*)&packed_Xjk)[2];
        i_left -= 16;
        if (i_left == 0) {
          goto END_LOOP;
        }
        Aji -= 16;
        Xik -= 16;
      }
      if (i_left >= 8) {
        packed_Aji[0] = _mm256_load_pd(Aji - 4);
        packed_Aji[1] = _mm256_load_pd(Aji - 8);

        packed_Xik[0] = _mm256_load_pd(Xik - 4);
        packed_Xik[1] = _mm256_load_pd(Xik - 8);

        packed_Xjk = _mm256_add_pd(
          _mm256_mul_pd(packed_Aji[0], packed_Xik[0]),
          _mm256_mul_pd(packed_Aji[1], packed_Xik[1])
        );
        packed_Xjk = _mm256_hadd_pd(packed_Xjk, packed_Xjk);
        *Xjk -= ((double*)&packed_Xjk)[0] + ((double*)&packed_Xjk)[2];
        i_left -= 8;
        if (i_left == 0) {
          goto END_LOOP;
        }
        Aji -= 8;
        Xik -= 8;
      }
      if (i_left >= 4) {
        packed_Aji[0] = _mm256_load_pd(Aji - 4);

        packed_Xik[0] = _mm256_load_pd(Xik - 4);

        packed_Xjk = _mm256_mul_pd(packed_Aji[0], packed_Xik[0]);

        packed_Xjk = _mm256_hadd_pd(packed_Xjk, packed_Xjk);
        *Xjk -= ((double*)&packed_Xjk)[0] + ((double*)&packed_Xjk)[2];
        i_left -= 4;
        if (i_left == 0) {
          goto END_LOOP;
        }
        Aji -= 4;
        Xik -= 4;
      }
      if (i_left >= 2) {
        *Xjk -= Aji[-1] *Xik[-1] +
                Aji[-2] *Xik[-2];
        i_left -= 2;
        if (i_left == 0) {
          goto END_LOOP;
        }
        Aji -= 2;
        Xik -= 2;
      }
      if (i_left == 1) {
        *Xjk -= Aji[-1] *Xik[-1];
      }
    END_LOOP:
      if (!unit) {
        *Xjk /= Ajj;
      };
      if (j == row_topleft) {
        break;
      }
    }
  }
}
//==========================================================================
// RIGHT LOWER KERNELS (traverse bottom-right to top-left)
//==========================================================================

//A col-major, B row-major order in memory. Solves for X in place of B
static void dtrsm_right_lower(double *A, double *B,
  IDX M, IDX B_rows, IDX B_cols,
  IDX row_topleft, IDX row_count, IDX col_topleft, IDX col_count,
  bool unit, double alpha)
{
  IDX k_end = row_topleft + row_count;
  IDX i_end = col_topleft + col_count;
  for (IDX k = row_topleft; k < k_end; ++k) {
    for (IDX i = i_end - 1; i >= col_topleft; --i) {
      double *Xki = &(IDXR(B, k, i, B_rows, B_cols));
      double Aii = IDXC(A, i, i, M, M);
      
      *Xki *= alpha;

      //Loop Below is heavily unrolled loop of the following (in reverse):
      //  for(IDX i=j+1; i<M; ++i){
      //    *Xki -= Xkj[i]*Aji[i]
      //  }

      IDX j = B_cols;
      IDX j_left = (j - i) - 1;
      double *Xkj = &(IDXR(B, k, j, B_rows, B_cols));
      double *Aji = &(IDXC(A, j, i, M, M));

      __m256d packed_Xki;
      __m256d packed_Xkj[8];
      __m256d packed_Aji[8];

      while (j_left >= 32) {
        packed_Xkj[0] = _mm256_load_pd(Xkj - 4);
        packed_Xkj[1] = _mm256_load_pd(Xkj - 8);
        packed_Xkj[2] = _mm256_load_pd(Xkj - 12);
        packed_Xkj[3] = _mm256_load_pd(Xkj - 16);
        packed_Xkj[4] = _mm256_load_pd(Xkj - 20);
        packed_Xkj[5] = _mm256_load_pd(Xkj - 24);
        packed_Xkj[6] = _mm256_load_pd(Xkj - 28);
        packed_Xkj[7] = _mm256_load_pd(Xkj - 32);

        packed_Aji[0] = _mm256_load_pd(Aji - 4);
        packed_Aji[1] = _mm256_load_pd(Aji - 8);
        packed_Aji[2] = _mm256_load_pd(Aji - 12);
        packed_Aji[3] = _mm256_load_pd(Aji - 16);
        packed_Aji[4] = _mm256_load_pd(Aji - 20);
        packed_Aji[5] = _mm256_load_pd(Aji - 24);
        packed_Aji[6] = _mm256_load_pd(Aji - 28);
        packed_Aji[7] = _mm256_load_pd(Aji - 32);

        packed_Xki = _mm256_add_pd(
          _mm256_add_pd(
            _mm256_add_pd(
              _mm256_mul_pd(packed_Xkj[0], packed_Aji[0]),
              _mm256_mul_pd(packed_Xkj[1], packed_Aji[1])
            ),
            _mm256_add_pd(
              _mm256_mul_pd(packed_Xkj[2], packed_Aji[2]),
              _mm256_mul_pd(packed_Xkj[3], packed_Aji[3])
            )
          ),
          _mm256_add_pd(
            _mm256_add_pd(
              _mm256_mul_pd(packed_Xkj[4], packed_Aji[4]),
              _mm256_mul_pd(packed_Xkj[5], packed_Aji[5])
            ),
            _mm256_add_pd(
              _mm256_mul_pd(packed_Xkj[6], packed_Aji[6]),
              _mm256_mul_pd(packed_Xkj[7], packed_Aji[7])
            )
          )
        );
        packed_Xki = _mm256_hadd_pd(packed_Xki, packed_Xki);
        *Xki -= ((double*)&packed_Xki)[0] + ((double*)&packed_Xki)[2];
        j_left -= 32;
        if (j_left == 0) {
          goto END_LOOP;
        }
        Xkj -= 32;
        Aji -= 32;
      }
      if (j_left >= 16) {
        packed_Xkj[0] = _mm256_load_pd(Xkj - 4);
        packed_Xkj[1] = _mm256_load_pd(Xkj - 8);
        packed_Xkj[2] = _mm256_load_pd(Xkj - 12);
        packed_Xkj[3] = _mm256_load_pd(Xkj - 16);

        packed_Aji[0] = _mm256_load_pd(Aji - 4);
        packed_Aji[1] = _mm256_load_pd(Aji - 8);
        packed_Aji[2] = _mm256_load_pd(Aji - 12);
        packed_Aji[3] = _mm256_load_pd(Aji - 16);

        packed_Xki = _mm256_add_pd(
          _mm256_add_pd(
            _mm256_mul_pd(packed_Xkj[0], packed_Aji[0]),
            _mm256_mul_pd(packed_Xkj[1], packed_Aji[1])
          ),
          _mm256_add_pd(
            _mm256_mul_pd(packed_Xkj[2], packed_Aji[2]),
            _mm256_mul_pd(packed_Xkj[3], packed_Aji[3])
          )
        );
        packed_Xki = _mm256_hadd_pd(packed_Xki, packed_Xki);
        *Xki -= ((double*)&packed_Xki)[0] + ((double*)&packed_Xki)[2];
        j_left -= 16;
        if (j_left == 0) {
          goto END_LOOP;
        }
        Xkj -= 16;
        Aji -= 16;
      }
      if (j_left >= 8) {
        packed_Xkj[0] = _mm256_load_pd(Xkj - 4);
        packed_Xkj[1] = _mm256_load_pd(Xkj - 8);

        packed_Aji[0] = _mm256_load_pd(Aji - 4);
        packed_Aji[1] = _mm256_load_pd(Aji - 8);

        packed_Xki = _mm256_add_pd(
          _mm256_mul_pd(packed_Xkj[0], packed_Aji[0]),
          _mm256_mul_pd(packed_Xkj[1], packed_Aji[1])
        );
        packed_Xki = _mm256_hadd_pd(packed_Xki, packed_Xki);
        *Xki -= ((double*)&packed_Xki)[0] + ((double*)&packed_Xki)[2];
        j_left -= 8;
        if (j_left == 0) {
          goto END_LOOP;
        }
        Xkj -= 8;
        Aji -= 8;
      }
      if (j_left >= 4) {
        packed_Xkj[0] = _mm256_load_pd(Xkj - 4);

        packed_Aji[0] = _mm256_load_pd(Aji - 4);

        packed_Xki = _mm256_mul_pd(packed_Xkj[0], packed_Aji[0]);
        packed_Xki = _mm256_hadd_pd(packed_Xki, packed_Xki);
        *Xki -= ((double*)&packed_Xki)[0] + ((double*)&packed_Xki)[2];
        j_left -= 4;
        if (j_left == 0) {
          goto END_LOOP;
        }
        Xkj -= 4;
        Aji -= 4;
      }
      if (j_left >= 2) {
        *Xki -= Xkj[-1] *Aji[-1] +
                Xkj[-2] *Aji[-2];
        j_left -= 2;
        if (j_left == 0) {
          goto END_LOOP;
        }
        Xkj -= 2;
        Aji -= 2;
      }
      if (j_left == 1) {
        *Xki -= Xkj[-1] *Aji[-1];
      }
    END_LOOP:
      if (!unit) {
        *Xki /= Aii;
      };
      if (i == col_topleft) {
        break;
      }
    }
  }
}


//==========================================================================
// Select the approprate Sub-TRSM algorithm and Panelize it
//
// The below algorithm calls the subroutine in small 'blocks' in order to 
// attempt to improve caching performance
//==========================================================================

void dtrsm_avx(double *A, double *B, IDX M, IDX B_rows, IDX B_cols,
  bool right, bool upper, bool unit, double alpha)
{
  if ((upper && right) || (!upper && !right)) {
    for (IDX row_topleft = 0; row_topleft < B_rows; row_topleft += BLOCK_D) {
      IDX row_count = MIN(BLOCK_D, (B_rows - row_topleft));
      for (IDX col_topleft = 0; col_topleft < B_cols; col_topleft += BLOCK_D) {
        IDX col_count = MIN(BLOCK_D, (B_cols - col_topleft));
        if (upper) {
          dtrsm_right_upper(A, B, M, B_rows, B_cols,
            row_topleft, row_count, col_topleft, col_count, unit, alpha);
        }
        else {
          dtrsm_left_lower(A, B, M, B_rows, B_cols,
            row_topleft, row_count, col_topleft, col_count, unit, alpha);
        }
      }
    }
  }
  else {
    IDX i = B_rows - (B_rows % BLOCK_D);
    IDX jstart = B_cols - (B_cols % BLOCK_D);
    while (true) {
      IDX j = jstart;
      while (true) {
        IDX icount = MIN(BLOCK_D, (B_rows - i));
        IDX jcount = MIN(BLOCK_D, (B_cols - j));
        if (upper) {
          dtrsm_left_upper(A, B, M, B_rows, B_cols,
            i, icount, j, jcount, unit, alpha);
        }
        else {
          dtrsm_right_lower(A, B, M, B_rows, B_cols,
            i, icount, j, jcount, unit, alpha);
        }
        if (j == 0) {
          break;
        }
        j -= BLOCK_D;
      }
      if (i == 0) {
        break;
      }
      i -= BLOCK_D;
    }
  }
}