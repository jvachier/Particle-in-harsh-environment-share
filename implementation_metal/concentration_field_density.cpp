#include "headers/concentration_field_density.h"
#include "headers/array_utils.h"

using namespace std;

/**
 * Update density field with chemotaxis, advection, reaction, and diffusion
 *
 * Solves the full PDE for population density with:
 * - Chemotaxis (movement towards chemical gradient)
 * - Advection (transport by environmental flow)
 * - Reaction (birth/death processes)
 * - Diffusion (random motion)
 *
 * @param c         Input: concentration field
 * @param f         Output: density field at next time step
 * @param f_n       Input: density field at current time step
 * @param advection 1D array of advection coefficients (size nz)
 * @param reaction  1D array of reaction coefficients (size nz)
 * @param diffusion 1D array of diffusion coefficients (size nz)
 * @param nx        Grid size in x direction
 * @param ny        Grid size in y direction
 * @param nz        Grid size in z direction
 * @param dt        Time step size
 * @param beta      Chemotaxis strength parameter
 * @param c_x, c_y, c_z     Grid spacing in concentration derivatives
 * @param c_xx, c_yy, c_zz  Grid spacing squared for concentration
 * @param u_x, u_y, u_z     Grid spacing in density derivatives
 * @param u_xx, u_yy, u_zz  Grid spacing squared for density
 * @param Cx, Cy, Cz        Coefficients for concentration first derivatives
 * @param Cxx, Cyy, Czz     Coefficients for concentration second derivatives
 * @param Fx, Fy, Fz        Coefficients for density first derivatives
 * @param Fxx, Fyy, Fzz     Coefficients for density second derivatives
 */
void concentration_field_density(
    const double *c, double *f, const double *f_n,
    const double *advection, const double *reaction, const double *diffusion,
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
            // Innermost loop vectorizes well
            #pragma omp simd
            for (int k = 1; k < nz - 1; k++) {
                // Calculate all required indices
                int idx = IDX3D(i, j, k, ny, nz);
                int idx_im1 = IDX3D(i-1, j, k, ny, nz);
                int idx_ip1 = IDX3D(i+1, j, k, ny, nz);
                int idx_jm1 = IDX3D(i, j-1, k, ny, nz);
                int idx_jp1 = IDX3D(i, j+1, k, ny, nz);

                // Cache current values
                double c_curr = c[idx];
                double f_n_curr = f_n[idx];

                // CONCENTRATION derivatives
                double c_x_local = c[idx_ip1] - c_curr;
                double c_y_local = c[idx_jp1] - c_curr;
                double c_z_local = c[idx+1] - c_curr;

                double c_xx_local = c[idx_im1] - 2.0 * c_curr + c[idx_ip1];
                double c_yy_local = c[idx_jm1] - 2.0 * c_curr + c[idx_jp1];
                double c_zz_local = c[idx-1] - 2.0 * c_curr + c[idx+1];

                // DENSITY derivatives
                double u_x_local = f_n[idx_ip1] - f_n_curr;
                double u_y_local = f_n[idx_jp1] - f_n_curr;
                double u_z_local = f_n[idx+1] - f_n_curr;

                double u_xx_local = f_n[idx_im1] - 2.0 * f_n_curr + f_n[idx_ip1];
                double u_yy_local = f_n[idx_jm1] - 2.0 * f_n_curr + f_n[idx_jp1];
                double u_zz_local = f_n[idx-1] - 2.0 * f_n_curr + f_n[idx+1];

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
                f[idx] = f_n_curr + chemotaxis + advection_term + reaction_term + diffusion_term;
            }
        }
    }
}
