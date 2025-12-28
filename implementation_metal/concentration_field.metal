#include <metal_stdlib>
using namespace metal;

// Metal GPU kernel for concentration field update
// Simpler than concentration_field_density - only handles diffusion
kernel void concentration_field_kernel(
    device const float *c_n [[buffer(0)]],     // Input: old concentration
    device float *c [[buffer(1)]],              // Output: new concentration
    constant uint &nx [[buffer(2)]],
    constant uint &ny [[buffer(3)]],
    constant uint &nz [[buffer(4)]],
    constant float &D_c [[buffer(5)]],          // Diffusion coefficient
    constant float &Cxx [[buffer(6)]],          // Grid spacing factors
    constant float &Cyy [[buffer(7)]],
    constant float &Czz [[buffer(8)]],
    uint3 gid [[thread_position_in_grid]])
{
    // Thread indices correspond to grid points (i, j, k)
    uint i = gid.x + 1;  // Start from 1 (skip boundary)
    uint j = gid.y + 1;
    uint k = gid.z + 1;

    // Skip boundary points and out-of-bounds threads
    if (i >= nx - 1 || j >= ny - 1 || k >= nz - 1) return;

    // Calculate linear indices for 3D array access
    uint idx = i * ny * nz + j * nz + k;
    uint idx_im1 = (i-1) * ny * nz + j * nz + k;  // i-1, j, k
    uint idx_ip1 = (i+1) * ny * nz + j * nz + k;  // i+1, j, k
    uint idx_jm1 = i * ny * nz + (j-1) * nz + k;  // i, j-1, k
    uint idx_jp1 = i * ny * nz + (j+1) * nz + k;  // i, j+1, k
    // k-1 and k+1 are just idx-1 and idx+1

    // Get current value
    float c_n_curr = c_n[idx];

    // Compute second derivatives (finite difference)
    // c_xx = c(i-1,j,k) - 2*c(i,j,k) + c(i+1,j,k)
    float c_xx_local = c_n[idx_im1] - 2.0f * c_n_curr + c_n[idx_ip1];

    // c_yy = c(i,j-1,k) - 2*c(i,j,k) + c(i,j+1,k)
    float c_yy_local = c_n[idx_jm1] - 2.0f * c_n_curr + c_n[idx_jp1];

    // c_zz = c(i,j,k-1) - 2*c(i,j,k) + c(i,j,k+1)
    float c_zz_local = c_n[idx-1] - 2.0f * c_n_curr + c_n[idx+1];

    // Update concentration: c = c_n + D_c * (Cxx*c_xx + Cyy*c_yy + Czz*c_zz)
    c[idx] = c_n_curr + D_c * (Cxx * c_xx_local + Cyy * c_yy_local + Czz * c_zz_local);
}
