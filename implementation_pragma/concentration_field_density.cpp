#include "headers/concentration_field_density.h"

using namespace std;

// Optimized version: better loop structure, cache locality, removed unused params
void concentration_field_density(
  double ***c, double ***f, double ***f_n,
  double *advection, double *reaction, double *diffusion,
  int nx, int ny, int nz,
  double dt, double beta, double c_x, double c_y, double c_z,
  double c_xx, double c_yy, double c_zz, double u_x, double u_y,
  double u_z, double u_xx, double u_yy, double u_zz, double Cx,
  double Cy, double Cz, double Cxx, double Cyy, double Czz,
  double Fx, double Fy, double Fz, double Fxx, double Fyy, double Fzz) {

  // Optimized loop: collapse only 2 levels, keep k inner for vectorization
  #pragma omp parallel for collapse(2)
  for (int i = 1; i < nx - 1; i++) {
    for (int j = 1; j < ny - 1; j++) {
      // Cache pointers for this j-slice to reduce pointer arithmetic
      double *c_im1_j = c[i-1][j];
      double *c_i_j = c[i][j];
      double *c_ip1_j = c[i+1][j];
      double *c_i_jm1 = c[i][j-1];
      double *c_i_jp1 = c[i][j+1];

      double *f_n_im1_j = f_n[i-1][j];
      double *f_n_i_j = f_n[i][j];
      double *f_n_ip1_j = f_n[i+1][j];
      double *f_n_i_jm1 = f_n[i][j-1];
      double *f_n_i_jp1 = f_n[i][j+1];

      double *f_i_j = f[i][j];

      // Innermost loop vectorizes well
      #pragma omp simd
      for (int k = 1; k < nz - 1; k++) {
        // Cache current values to reduce array accesses
        double c_curr = c_i_j[k];
        double f_n_curr = f_n_i_j[k];

        // CONCENTRATION derivatives
        double c_x_local = c_ip1_j[k] - c_curr;
        double c_y_local = c_i_jp1[k] - c_curr;
        double c_z_local = c_i_j[k+1] - c_curr;

        double c_xx_local = c_im1_j[k] - 2.0 * c_curr + c_ip1_j[k];
        double c_yy_local = c_i_jm1[k] - 2.0 * c_curr + c_i_jp1[k];
        double c_zz_local = c_i_j[k-1] - 2.0 * c_curr + c_i_j[k+1];

        // DENSITY derivatives
        double u_x_local = f_n_ip1_j[k] - f_n_curr;
        double u_y_local = f_n_i_jp1[k] - f_n_curr;
        double u_z_local = f_n_i_j[k+1] - f_n_curr;

        double u_xx_local = f_n_im1_j[k] - 2.0 * f_n_curr + f_n_ip1_j[k];
        double u_yy_local = f_n_i_jm1[k] - 2.0 * f_n_curr + f_n_i_jp1[k];
        double u_zz_local = f_n_i_j[k-1] - 2.0 * f_n_curr + f_n_i_j[k+1];

        // Compute terms
        double chemotaxis = beta * (
          Fx * u_x_local * Cx * c_x_local +
          Fy * u_y_local * Cy * c_y_local +
          Fz * u_z_local * Cz * c_z_local +
          f_n_curr * (Cxx * c_xx_local + Cyy * c_yy_local + Czz * c_zz_local)
        );

        double advection_term = Fz * advection[k] * u_z_local;
        double reaction_term = -dt * reaction[k] * f_n_curr;
        double diffusion_term = diffusion[k] * (Fxx * u_xx_local + Fyy * u_yy_local + Fzz * u_zz_local);

        // Single write
        f_i_j[k] = f_n_curr + chemotaxis + advection_term + reaction_term + diffusion_term;
      }
    }
  }
}
