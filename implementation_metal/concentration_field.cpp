#include "headers/concentration_field.h"
#include "headers/array_utils.h"

using namespace std;

/**
 * Update concentration field using diffusion equation
 *
 * Solves: dc/dt = D_c * (Cxx * d²c/dx² + Cyy * d²c/dy² + Czz * d²c/dz²)
 *
 * @param c_n Input: concentration at current time step
 * @param c   Output: concentration at next time step
 * @param nx  Grid size in x direction
 * @param ny  Grid size in y direction
 * @param nz  Grid size in z direction
 * @param c_xx Unused (kept for compatibility)
 * @param c_yy Unused (kept for compatibility)
 * @param c_zz Unused (kept for compatibility)
 * @param D_c  Diffusion coefficient
 * @param Cxx  Grid spacing coefficient in x
 * @param Cyy  Grid spacing coefficient in y
 * @param Czz  Grid spacing coefficient in z
 */
void concentration_field(
    const double *c_n, double *c,
    int nx, int ny, int nz,
    double c_xx, double c_yy, double c_zz,
    double D_c, double Cxx, double Cyy, double Czz) {

    #pragma omp parallel for collapse(2)
    for (int i = 1; i < nx - 1; i++) {
        for (int j = 1; j < ny - 1; j++) {
            #pragma omp simd
            for (int k = 1; k < nz - 1; k++) {
                // Current index and neighbors
                int idx = IDX3D(i, j, k, ny, nz);
                int idx_im1 = IDX3D(i-1, j, k, ny, nz);
                int idx_ip1 = IDX3D(i+1, j, k, ny, nz);
                int idx_jm1 = IDX3D(i, j-1, k, ny, nz);
                int idx_jp1 = IDX3D(i, j+1, k, ny, nz);

                double c_n_curr = c_n[idx];

                // Compute second derivatives
                double c_xx_local = c_n[idx_im1] - 2.0 * c_n_curr + c_n[idx_ip1];
                double c_yy_local = c_n[idx_jm1] - 2.0 * c_n_curr + c_n[idx_jp1];
                double c_zz_local = c_n[idx-1] - 2.0 * c_n_curr + c_n[idx+1];

                // Update concentration
                c[idx] = c_n_curr + D_c * (Cxx * c_xx_local + Cyy * c_yy_local + Czz * c_zz_local);
            }
        }
    }
}
