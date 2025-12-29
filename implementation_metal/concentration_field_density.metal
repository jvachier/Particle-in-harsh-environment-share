#include <metal_stdlib>
using namespace metal;

// Metal compute kernel for concentration_field_density
// This replaces the CPU triple-nested loop with GPU parallelization
kernel void compute_concentration_field_density(
    const device double* c [[buffer(0)]],         // Concentration array (flattened)
    const device double* f_n [[buffer(1)]],       // Density array old (flattened)
    device double* f [[buffer(2)]],               // Density array new (flattened)
    const device double* advection [[buffer(3)]], // Advection coefficients [nz]
    const device double* reaction [[buffer(4)]],  // Reaction coefficients [nz]
    const device double* diffusion [[buffer(5)]], // Diffusion coefficients [nz]
    constant int& nx [[buffer(6)]],
    constant int& ny [[buffer(7)]],
    constant int& nz [[buffer(8)]],
    constant double& dt [[buffer(9)]],
    constant double& beta [[buffer(10)]],
    constant double& Cx [[buffer(11)]],
    constant double& Cy [[buffer(12)]],
    constant double& Cz [[buffer(13)]],
    constant double& Cxx [[buffer(14)]],
    constant double& Cyy [[buffer(15)]],
    constant double& Czz [[buffer(16)]],
    constant double& Fx [[buffer(17)]],
    constant double& Fy [[buffer(18)]],
    constant double& Fz [[buffer(19)]],
    constant double& Fxx [[buffer(20)]],
    constant double& Fyy [[buffer(21)]],
    constant double& Fzz [[buffer(22)]],
    uint3 gid [[thread_position_in_grid]]) {

    int i = gid.x + 1;  // Start from 1 to avoid boundary
    int j = gid.y + 1;
    int k = gid.z + 1;

    // Boundary check
    if (i >= nx - 1 || j >= ny - 1 || k >= nz - 1) return;

    // Helper function to calculate linear index from 3D coordinates
    // Index: i * (ny * nz) + j * nz + k
    auto idx = [&](int ii, int jj, int kk) -> int {
        return ii * (ny * nz) + jj * nz + kk;
    };

    // Current cell index
    int current = idx(i, j, k);

    // CONCENTRATION gradients
    double c_x = c[idx(i+1, j, k)] - c[current];
    double c_y = c[idx(i, j+1, k)] - c[current];
    double c_z = c[idx(i, j, k+1)] - c[current];

    double c_xx = c[idx(i-1, j, k)] - 2.0 * c[current] + c[idx(i+1, j, k)];
    double c_yy = c[idx(i, j-1, k)] - 2.0 * c[current] + c[idx(i, j+1, k)];
    double c_zz = c[idx(i, j, k-1)] - 2.0 * c[current] + c[idx(i, j, k+1)];

    // DENSITY gradients
    double u_x = f_n[idx(i+1, j, k)] - f_n[current];
    double u_y = f_n[idx(i, j+1, k)] - f_n[current];
    double u_z = f_n[idx(i, j, k+1)] - f_n[current];

    double u_xx = f_n[idx(i-1, j, k)] - 2.0 * f_n[current] + f_n[idx(i+1, j, k)];
    double u_yy = f_n[idx(i, j-1, k)] - 2.0 * f_n[current] + f_n[idx(i, j+1, k)];
    double u_zz = f_n[idx(i, j, k-1)] - 2.0 * f_n[current] + f_n[idx(i, j, k+1)];

    // Compute new density value
    double chemotaxis = beta * (
        Fx * u_x * Cx * c_x +
        Fy * u_y * Cy * c_y +
        Fz * u_z * Cz * c_z +
        f_n[current] * (Cxx * c_xx + Cyy * c_yy + Czz * c_zz)
    );

    double advection_term = Fz * advection[k] * u_z;
    double reaction_term = -dt * reaction[k] * f_n[current];
    double diffusion_term = diffusion[k] * (Fxx * u_xx + Fyy * u_yy + Fzz * u_zz);

    // Update density
    f[current] = f_n[current] + chemotaxis + advection_term + reaction_term + diffusion_term;
}
