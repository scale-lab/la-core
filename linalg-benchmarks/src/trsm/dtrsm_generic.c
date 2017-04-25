//==========================================================================
// Copyright (c) 2017 Samuel Steffl
// All rights reserved.
// 
// Generic implementation of BLAS-3 DTRSM function
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

      while (i_left >= 32) {
        *Xjk -= Aji[0] *Xik[0] +
                Aji[1] *Xik[1] +
                Aji[2] *Xik[2] +
                Aji[3] *Xik[3] +
                Aji[4] *Xik[4] +
                Aji[5] *Xik[5] +
                Aji[6] *Xik[6] +
                Aji[7] *Xik[7] +
                Aji[8] *Xik[8] +
                Aji[9] *Xik[9] +
                Aji[10]*Xik[10] +
                Aji[11]*Xik[11] +
                Aji[12]*Xik[12] +
                Aji[13]*Xik[13] +
                Aji[14]*Xik[14] +
                Aji[15]*Xik[15] +
                Aji[16]*Xik[16] +
                Aji[17]*Xik[17] +
                Aji[18]*Xik[18] +
                Aji[19]*Xik[19] +
                Aji[20]*Xik[20] +
                Aji[21]*Xik[21] +
                Aji[22]*Xik[22] +
                Aji[23]*Xik[23] +
                Aji[24]*Xik[24] +
                Aji[25]*Xik[25] +
                Aji[26]*Xik[26] +
                Aji[27]*Xik[27] +
                Aji[28]*Xik[28] +
                Aji[29]*Xik[29] +
                Aji[30]*Xik[30] +
                Aji[31]*Xik[31];
        i_left -= 32;
        if (i_left == 0) {
          goto END_LOOP;
        }
        Aji += 32;
        Xik += 32;
      }
      if (i_left >= 16) {
        *Xjk -= Aji[0] *Xik[0] +
                Aji[1] *Xik[1] +
                Aji[2] *Xik[2] +
                Aji[3] *Xik[3] +
                Aji[4] *Xik[4] +
                Aji[5] *Xik[5] +
                Aji[6] *Xik[6] +
                Aji[7] *Xik[7] +
                Aji[8] *Xik[8] +
                Aji[9] *Xik[9] +
                Aji[10]*Xik[10] +
                Aji[11]*Xik[11] +
                Aji[12]*Xik[12] +
                Aji[13]*Xik[13] +
                Aji[14]*Xik[14] +
                Aji[15]*Xik[15];
        i_left -= 16;
        if (i_left == 0) {
          goto END_LOOP;
        }
        Aji += 16;
        Xik += 16;
      }
      if (i_left >= 8) {
        *Xjk -= Aji[0] *Xik[0] +
                Aji[1] *Xik[1] +
                Aji[2] *Xik[2] +
                Aji[3] *Xik[3] +
                Aji[4] *Xik[4] +
                Aji[5] *Xik[5] +
                Aji[6] *Xik[6] +
                Aji[7] *Xik[7];
        i_left -= 8;
        if (i_left == 0) {
          goto END_LOOP;
        }
        Aji += 8;
        Xik += 8;
      }
      if (i_left >= 4) {
        *Xjk -= Aji[0] *Xik[0] +
                Aji[1] *Xik[1] +
                Aji[2] *Xik[2] +
                Aji[3] *Xik[3];
        i_left -= 4;
        if (i_left == 0) {
          goto END_LOOP;
        }
        Aji += 4;
        Xik += 4;
      }
      if (i_left >= 2) {
        *Xjk -= Aji[0] *Xik[0] +
                Aji[1] *Xik[1];
        i_left -= 2;
        if (i_left == 0) {
          goto END_LOOP;
        }
        Aji += 2;
        Xik += 2;
      }
      if (i_left == 1) {
        *Xjk -= Aji[0] *Xik[0];
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

      while (j_left >= 32) {
        *Xki -= Xjk[0] *Aji[0] +
                Xjk[1] *Aji[1] +
                Xjk[2] *Aji[2] +
                Xjk[3] *Aji[3] +
                Xjk[4] *Aji[4] +
                Xjk[5] *Aji[5] +
                Xjk[6] *Aji[6] +
                Xjk[7] *Aji[7] +
                Xjk[8] *Aji[8] +
                Xjk[9] *Aji[9] +
                Xjk[10]*Aji[10] +
                Xjk[11]*Aji[11] +
                Xjk[12]*Aji[12] +
                Xjk[13]*Aji[13] +
                Xjk[14]*Aji[14] +
                Xjk[15]*Aji[15] +
                Xjk[16]*Aji[16] +
                Xjk[17]*Aji[17] +
                Xjk[18]*Aji[18] +
                Xjk[19]*Aji[19] +
                Xjk[20]*Aji[20] +
                Xjk[21]*Aji[21] +
                Xjk[22]*Aji[22] +
                Xjk[23]*Aji[23] +
                Xjk[24]*Aji[24] +
                Xjk[25]*Aji[25] +
                Xjk[26]*Aji[26] +
                Xjk[27]*Aji[27] +
                Xjk[28]*Aji[28] +
                Xjk[29]*Aji[29] +
                Xjk[30]*Aji[30] +
                Xjk[31]*Aji[31];
        j_left -= 32;
        if (j_left == 0) {
          goto END_LOOP;
        }
        Xjk += 32;
        Aji += 32;
      }
      if (j_left >= 16) {
        *Xki -= Xjk[0] *Aji[0] +
                Xjk[1] *Aji[1] +
                Xjk[2] *Aji[2] +
                Xjk[3] *Aji[3] +
                Xjk[4] *Aji[4] +
                Xjk[5] *Aji[5] +
                Xjk[6] *Aji[6] +
                Xjk[7] *Aji[7] +
                Xjk[8] *Aji[8] +
                Xjk[9] *Aji[9] +
                Xjk[10]*Aji[10] +
                Xjk[11]*Aji[11] +
                Xjk[12]*Aji[12] +
                Xjk[13]*Aji[13] +
                Xjk[14]*Aji[14] +
                Xjk[15]*Aji[15];
        j_left -= 16;
        if (j_left == 0) {
          goto END_LOOP;
        }
        Xjk += 16;
        Aji += 16;
      }
      if (j_left >= 8) {
        *Xki -= Xjk[0] *Aji[0] +
                Xjk[1] *Aji[1] +
                Xjk[2] *Aji[2] +
                Xjk[3] *Aji[3] +
                Xjk[4] *Aji[4] +
                Xjk[5] *Aji[5] +
                Xjk[6] *Aji[6] +
                Xjk[7] *Aji[7];
        j_left -= 8;
        if (j_left == 0) {
          goto END_LOOP;
        }
        Xjk += 8;
        Aji += 8;
      }
      if (j_left >= 4) {
        *Xki -= Xjk[0] *Aji[0] +
                Xjk[1] *Aji[1] +
                Xjk[2] *Aji[2] +
                Xjk[3] *Aji[3];
        j_left -= 4;
        if (j_left == 0) {
          goto END_LOOP;
        }
        Xjk += 4;
        Aji += 4;
      }
      if (j_left >= 2) {
        *Xki -= Xjk[0] *Aji[0] +
                Xjk[1] *Aji[1];
        j_left -= 2;
        if (j_left == 0) {
          goto END_LOOP;
        }
        Xjk += 2;
        Aji += 2;
      }
      if (j_left == 1) {
        *Xki -= Xjk[0] * Aji[0];
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

      while (i_left >= 32) {
        *Xjk -= Aji[-1] *Xik[-1] +
                Aji[-2] *Xik[-2] +
                Aji[-3] *Xik[-3] +
                Aji[-4] *Xik[-4] +
                Aji[-5] *Xik[-5] +
                Aji[-6] *Xik[-6] +
                Aji[-7] *Xik[-7] +
                Aji[-8] *Xik[-8] +
                Aji[-9] *Xik[-9] +
                Aji[-10]*Xik[-10] +
                Aji[-11]*Xik[-11] +
                Aji[-12]*Xik[-12] +
                Aji[-13]*Xik[-13] +
                Aji[-14]*Xik[-14] +
                Aji[-15]*Xik[-15] +
                Aji[-16]*Xik[-16] +
                Aji[-17]*Xik[-17] +
                Aji[-18]*Xik[-18] +
                Aji[-19]*Xik[-19] +
                Aji[-20]*Xik[-20] +
                Aji[-21]*Xik[-21] +
                Aji[-22]*Xik[-22] +
                Aji[-23]*Xik[-23] +
                Aji[-24]*Xik[-24] +
                Aji[-25]*Xik[-25] +
                Aji[-26]*Xik[-26] +
                Aji[-27]*Xik[-27] +
                Aji[-28]*Xik[-28] +
                Aji[-29]*Xik[-29] +
                Aji[-30]*Xik[-30] +
                Aji[-31]*Xik[-31] +
                Aji[-32]*Xik[-32];
        i_left -= 32;
        if (i_left == 0) {
          goto END_LOOP;
        }
        Aji -= 32;
        Xik -= 32;
      }
      if (i_left >= 16) {
        *Xjk -= Aji[-1] *Xik[-1] +
                Aji[-2] *Xik[-2] +
                Aji[-3] *Xik[-3] +
                Aji[-4] *Xik[-4] +
                Aji[-5] *Xik[-5] +
                Aji[-6] *Xik[-6] +
                Aji[-7] *Xik[-7] +
                Aji[-8] *Xik[-8] +
                Aji[-9] *Xik[-9] +
                Aji[-10]*Xik[-10] +
                Aji[-11]*Xik[-11] +
                Aji[-12]*Xik[-12] +
                Aji[-13]*Xik[-13] +
                Aji[-14]*Xik[-14] +
                Aji[-15]*Xik[-15] +
                Aji[-16]*Xik[-16];
        i_left -= 16;
        if (i_left == 0) {
          goto END_LOOP;
        }
        Aji -= 16;
        Xik -= 16;
      }
      if (i_left >= 8) {
        *Xjk -= Aji[-1] *Xik[-1] +
                Aji[-2] *Xik[-2] +
                Aji[-3] *Xik[-3] +
                Aji[-4] *Xik[-4] +
                Aji[-5] *Xik[-5] +
                Aji[-6] *Xik[-6] +
                Aji[-7] *Xik[-7] +
                Aji[-8] *Xik[-8];
        i_left -= 8;
        if (i_left == 0) {
          goto END_LOOP;
        }
        Aji -= 8;
        Xik -= 8;
      }
      if (i_left >= 4) {
        *Xjk -= Aji[-1] *Xik[-1] +
                Aji[-2] *Xik[-2] +
                Aji[-3] *Xik[-3] +
                Aji[-4] *Xik[-4];
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

      while (j_left >= 32) {
        *Xki -= Xkj[-1] *Aji[-1] +
                Xkj[-2] *Aji[-2] +
                Xkj[-3] *Aji[-3] +
                Xkj[-4] *Aji[-4] +
                Xkj[-5] *Aji[-5] +
                Xkj[-6] *Aji[-6] +
                Xkj[-7] *Aji[-7] +
                Xkj[-8] *Aji[-8] +
                Xkj[-9] *Aji[-9] +
                Xkj[-10]*Aji[-10] +
                Xkj[-11]*Aji[-11] +
                Xkj[-12]*Aji[-12] +
                Xkj[-13]*Aji[-13] +
                Xkj[-14]*Aji[-14] +
                Xkj[-15]*Aji[-15] +
                Xkj[-16]*Aji[-16] +
                Xkj[-17]*Aji[-17] +
                Xkj[-18]*Aji[-18] +
                Xkj[-19]*Aji[-19] +
                Xkj[-20]*Aji[-20] +
                Xkj[-21]*Aji[-21] +
                Xkj[-22]*Aji[-22] +
                Xkj[-23]*Aji[-23] +
                Xkj[-24]*Aji[-24] +
                Xkj[-25]*Aji[-25] +
                Xkj[-26]*Aji[-26] +
                Xkj[-27]*Aji[-27] +
                Xkj[-28]*Aji[-28] +
                Xkj[-29]*Aji[-29] +
                Xkj[-30]*Aji[-30] +
                Xkj[-31]*Aji[-31] +
                Xkj[-32]*Aji[-32];
        j_left -= 32;
        if (j_left == 0) {
          goto END_LOOP;
        }
        Xkj -= 32;
        Aji -= 32;
      }
      if (j_left >= 16) {
        *Xki -= Xkj[-1] *Aji[-1] +
                Xkj[-2] *Aji[-2] +
                Xkj[-3] *Aji[-3] +
                Xkj[-4] *Aji[-4] +
                Xkj[-5] *Aji[-5] +
                Xkj[-6] *Aji[-6] +
                Xkj[-7] *Aji[-7] +
                Xkj[-8] *Aji[-8] +
                Xkj[-9] *Aji[-9] +
                Xkj[-10]*Aji[-10] +
                Xkj[-11]*Aji[-11] +
                Xkj[-12]*Aji[-12] +
                Xkj[-13]*Aji[-13] +
                Xkj[-14]*Aji[-14] +
                Xkj[-15]*Aji[-15] +
                Xkj[-16]*Aji[-16];
        j_left -= 16;
        if (j_left == 0) {
          goto END_LOOP;
        }
        Xkj -= 16;
        Aji -= 16;
      }
      if (j_left >= 8) {
        *Xki -= Xkj[-1] *Aji[-1] +
                Xkj[-2] *Aji[-2] +
                Xkj[-3] *Aji[-3] +
                Xkj[-4] *Aji[-4] +
                Xkj[-5] *Aji[-5] +
                Xkj[-6] *Aji[-6] +
                Xkj[-7] *Aji[-7] +
                Xkj[-8] *Aji[-8];
        j_left -= 8;
        if (j_left == 0) {
          goto END_LOOP;
        }
        Xkj -= 8;
        Aji -= 8;
      }
      if (j_left >= 4) {
        *Xki -= Xkj[-1] *Aji[-1] +
                Xkj[-2] *Aji[-2] +
                Xkj[-3] *Aji[-3] +
                Xkj[-4] *Aji[-4];
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

void dtrsm_generic(double *A, double *B, IDX M, IDX B_rows, IDX B_cols,
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