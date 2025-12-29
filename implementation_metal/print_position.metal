#include <metal_stdlib>
using namespace metal;

// Kernel for extracting z-profile data
kernel void extract_z_profile(
    const device double* f [[buffer(0)]],
    const device double* c [[buffer(1)]],
    device double* z_positions [[buffer(2)]],
    device double* f_values [[buffer(3)]],
    device double* c_values [[buffer(4)]],
    constant double& dz [[buffer(5)]],
    constant int& nx [[buffer(6)]],
    constant int& ny [[buffer(7)]],
    constant int& nz [[buffer(8)]],
    constant int& sample_x [[buffer(9)]],
    constant int& sample_y [[buffer(10)]],
    uint k [[thread_position_in_grid]]) {

    if (k >= nz) return;

    // Calculate 3D array index: f[sample_x][sample_y][k]
    int idx = sample_x * (ny * nz) + sample_y * nz + k;

    z_positions[k] = k * dz;
    f_values[k] = f[idx];
    c_values[k] = c[idx];
}

// Kernel for extracting x-profile data
kernel void extract_x_profile(
    const device double* f [[buffer(0)]],
    const device double* c [[buffer(1)]],
    device double* x_positions [[buffer(2)]],
    device double* f_values [[buffer(3)]],
    device double* c_values [[buffer(4)]],
    constant double& dz [[buffer(5)]],
    constant int& nx [[buffer(6)]],
    constant int& ny [[buffer(7)]],
    constant int& nz [[buffer(8)]],
    constant int& sample_y [[buffer(9)]],
    constant int& sample_z [[buffer(10)]],
    uint i [[thread_position_in_grid]]) {

    if (i >= nx) return;

    // Calculate 3D array index: f[i][sample_y][sample_z]
    int idx = i * (ny * nz) + sample_y * nz + sample_z;

    x_positions[i] = i * dz;
    f_values[i] = f[idx];
    c_values[i] = c[idx];
}
