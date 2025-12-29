#include "headers/concentration_field.h"

using namespace std;

// Optimized version with pointer caching
void concentration_field(
  double ***c_n, double ***c,
  int nx, int ny, int nz,
  double c_xx, double c_yy, double c_zz,
  double D_c, double Cxx, double Cyy, double Czz) {

  #pragma omp parallel for collapse(2)
  for (int i = 1; i < nx - 1; i++) {
    for (int j = 1; j < ny - 1; j++) {
      // Cache pointers to reduce pointer arithmetic
      double *c_n_im1_j = c_n[i-1][j];
      double *c_n_i_j = c_n[i][j];
      double *c_n_ip1_j = c_n[i+1][j];
      double *c_n_i_jm1 = c_n[i][j-1];
      double *c_n_i_jp1 = c_n[i][j+1];
      double *c_i_j = c[i][j];

      #pragma omp simd
      for (int k = 1; k < nz - 1; k++) {
        double c_n_curr = c_n_i_j[k];

        // Compute second derivatives
        double c_xx_local = c_n_im1_j[k] - 2.0 * c_n_curr + c_n_ip1_j[k];
        double c_yy_local = c_n_i_jm1[k] - 2.0 * c_n_curr + c_n_i_jp1[k];
        double c_zz_local = c_n_i_j[k-1] - 2.0 * c_n_curr + c_n_i_j[k+1];

        // Update concentration
        c_i_j[k] = c_n_curr + D_c * (Cxx * c_xx_local + Cyy * c_yy_local + Czz * c_zz_local);
      }
    }
  }
}
