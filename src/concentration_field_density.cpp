#include "headers/concentration_field_density.h"

using namespace std;

void concentration_field_density(
  double ***c, double ***f, double ***f_n,
  double *advection, double *reaction, double *diffusion,
  int nx, int ny, int nz,
  double dt, double beta, double c_x, double c_y, double c_z,
  double c_xx, double c_yy, double c_zz, double u_x, double u_y,
  double u_z, double u_xx, double u_yy, double u_zz, double Cx,
  double Cy, double Cz, double Cxx, double Cyy, double Czz,
  double Fx, double Fy, double Fz, double Fxx, double Fyy, double Fzz) {
#pragma omp parallel for simd collapse(3)
    for (int i = 1; i < nx - 1; i++) {
      for (int j = 1; j < ny - 1; j++) {
        for (int k = 1; k < nz - 1; k++) {
          // CONCENTRATION.
          c_x = c[i + 1][j][k] - c[i][j][k];
          c_y = c[i][j + 1][k] - c[i][j][k];
          c_z = c[i][j][k + 1] - c[i][j][k];

          c_xx = c[i - 1][j][k] - 2.0 * c[i][j][k] + c[i + 1][j][k];
          c_yy = c[i][j - 1][k] - 2.0 * c[i][j][k] + c[i][j + 1][k];
          c_zz = c[i][j][k - 1] - 2.0 * c[i][j][k] + c[i][j][k + 1];

          // DENSITY
          u_x = f_n[i + 1][j][k] - f_n[i][j][k];
          u_y = f_n[i][j + 1][k] - f_n[i][j][k];
          u_z = f_n[i][j][k + 1] - f_n[i][j][k];
          u_xx = f_n[i - 1][j][k] - 2.0 * f_n[i][j][k] + f_n[i + 1][j][k];
          u_yy = f_n[i][j - 1][k] - 2.0 * f_n[i][j][k] + f_n[i][j + 1][k];
          u_zz = f_n[i][j][k - 1] - 2.0 * f_n[i][j][k] + f_n[i][j][k + 1];

          f[i][j][k] = f_n[i][j][k] + beta * \
            (Fx * u_x * Cx * c_x + Fy * u_y * Cy * c_y + \
            Fz * u_z * Cz * c_z + f_n[i][j][k] * \
            (Cxx * c_xx + Cyy * c_yy + Czz * c_zz)) + \
            Fz * advection[k] * u_z - dt * reaction[k] * f_n[i][j][k] + \
            diffusion[k] * (Fxx * u_xx + Fyy * u_yy + Fzz * u_zz);
        }
      }
    }
}
