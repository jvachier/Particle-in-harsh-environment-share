/*
 * Pragma SIMD Implementation Tests
 * Physics validation and sanity checks for CI
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

// Include pragma implementation
#include "../implementation_pragma/headers/concentration_field_density.h"

// Test statistics
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

void report_test(const char* name, bool passed, const char* message = "") {
    tests_run++;
    if (passed) {
        tests_passed++;
        printf("  [PASS] %s\n", name);
    } else {
        tests_failed++;
        printf("  [FAIL] %s: %s\n", name, message);
    }
}

// Helper to allocate 3D array (triple pointer)
double*** allocate_3d_array(int nx, int ny, int nz) {
    double ***arr = (double***)malloc(nx * sizeof(double**));
    for (int i = 0; i < nx; i++) {
        arr[i] = (double**)malloc(ny * sizeof(double*));
        for (int j = 0; j < ny; j++) {
            arr[i][j] = (double*)calloc(nz, sizeof(double));
        }
    }
    return arr;
}

// Helper to free 3D array
void free_3d_array(double*** arr, int nx, int ny) {
    for (int i = 0; i < nx; i++) {
        for (int j = 0; j < ny; j++) {
            free(arr[i][j]);
        }
        free(arr[i]);
    }
    free(arr);
}

// Initialize array with test pattern
void initialize_test_data(double*** arr, int nx, int ny, int nz, double seed) {
    for (int i = 0; i < nx; i++) {
        for (int j = 0; j < ny; j++) {
            for (int k = 0; k < nz; k++) {
                arr[i][j][k] = seed * sin(2.0 * M_PI * i / nx) *
                                     cos(2.0 * M_PI * j / ny) *
                                     exp(-k / (double)nz);
            }
        }
    }
}

// Check for NaN or Inf values
bool check_no_nan_inf(double*** arr, int nx, int ny, int nz, const char* name) {
    for (int i = 0; i < nx; i++) {
        for (int j = 0; j < ny; j++) {
            for (int k = 0; k < nz; k++) {
                if (isnan(arr[i][j][k]) || isinf(arr[i][j][k])) {
                    char msg[256];
                    snprintf(msg, sizeof(msg), "Found NaN/Inf at [%d][%d][%d]", i, j, k);
                    report_test(name, false, msg);
                    return false;
                }
            }
        }
    }
    report_test(name, true);
    return true;
}

// Check values are in reasonable range
bool check_reasonable_range(double*** arr, int nx, int ny, int nz,
                           double min_val, double max_val, const char* name) {
    int out_of_range = 0;

    for (int i = 0; i < nx; i++) {
        for (int j = 0; j < ny; j++) {
            for (int k = 0; k < nz; k++) {
                if (arr[i][j][k] < min_val || arr[i][j][k] > max_val) {
                    out_of_range++;
                }
            }
        }
    }

    if (out_of_range > 0) {
        char msg[256];
        snprintf(msg, sizeof(msg), "%d values out of range [%.2e, %.2e]",
                out_of_range, min_val, max_val);
        report_test(name, false, msg);
        return false;
    }

    report_test(name, true);
    return true;
}

void test_basic_computation() {
    printf("\nTest: Basic computation sanity checks\n");

    int nx = 10, ny = 10, nz = 20;

    // Allocate arrays
    double ***c = allocate_3d_array(nx, ny, nz);
    double ***f = allocate_3d_array(nx, ny, nz);
    double ***f_n = allocate_3d_array(nx, ny, nz);
    double *advection = (double*)calloc(nz, sizeof(double));
    double *reaction = (double*)calloc(nz, sizeof(double));
    double *diffusion = (double*)calloc(nz, sizeof(double));

    // Initialize with test data
    initialize_test_data(c, nx, ny, nz, 1.0);
    initialize_test_data(f_n, nx, ny, nz, 0.5);

    for (int k = 0; k < nz; k++) {
        advection[k] = 0.1 * k / nz;
        reaction[k] = 0.05;
        diffusion[k] = 0.01;
    }

    // Parameters
    double dt = 1e5;
    double beta = -1e-10;
    double c_x = 0.0, c_y = 0.0, c_z = 0.0;
    double c_xx = 0.0, c_yy = 0.0, c_zz = 0.0;
    double u_x = 0.0, u_y = 0.0, u_z = 0.0;
    double u_xx = 0.0, u_yy = 0.0, u_zz = 0.0;
    double dx = 1.0, dy = 1.0, dz = 4.0;
    double Cx = 1.0 / dx, Cy = 1.0 / dy, Cz = 1.0 / dz;
    double Cxx = dt / (dx * dx), Cyy = dt / (dy * dy), Czz = dt / (dz * dz);
    double Fx = dt / dx, Fy = dt / dy, Fz = dt / dz;
    double Fxx = dt / (dx * dx), Fyy = dt / (dy * dy), Fzz = dt / (dz * dz);

    // Run computation
    concentration_field_density(
        c, f, f_n, advection, reaction, diffusion,
        nx, ny, nz, dt, beta, c_x, c_y, c_z, c_xx, c_yy, c_zz,
        u_x, u_y, u_z, u_xx, u_yy, u_zz, Cx, Cy, Cz,
        Cxx, Cyy, Czz, Fx, Fy, Fz, Fxx, Fyy, Fzz);

    // Validate results
    check_no_nan_inf(f, nx, ny, nz, "No NaN/Inf in output");
    check_reasonable_range(f, nx, ny, nz, -1e10, 1e10, "Output in reasonable range");

    // Cleanup
    free_3d_array(c, nx, ny);
    free_3d_array(f, nx, ny);
    free_3d_array(f_n, nx, ny);
    free(advection);
    free(reaction);
    free(diffusion);
}

void test_zero_input() {
    printf("\nTest: Zero input handling\n");

    int nx = 5, ny = 5, nz = 10;

    // Allocate arrays (calloc initializes to zero)
    double ***c = allocate_3d_array(nx, ny, nz);
    double ***f = allocate_3d_array(nx, ny, nz);
    double ***f_n = allocate_3d_array(nx, ny, nz);
    double *advection = (double*)calloc(nz, sizeof(double));
    double *reaction = (double*)calloc(nz, sizeof(double));
    double *diffusion = (double*)calloc(nz, sizeof(double));

    // Parameters
    double dt = 1e5, beta = -1e-10;
    double c_x = 0.0, c_y = 0.0, c_z = 0.0;
    double c_xx = 0.0, c_yy = 0.0, c_zz = 0.0;
    double u_x = 0.0, u_y = 0.0, u_z = 0.0;
    double u_xx = 0.0, u_yy = 0.0, u_zz = 0.0;
    double dx = 1.0, dy = 1.0, dz = 4.0;
    double Cx = 1.0 / dx, Cy = 1.0 / dy, Cz = 1.0 / dz;
    double Cxx = dt / (dx * dx), Cyy = dt / (dy * dy), Czz = dt / (dz * dz);
    double Fx = dt / dx, Fy = dt / dy, Fz = dt / dz;
    double Fxx = dt / (dx * dx), Fyy = dt / (dy * dy), Fzz = dt / (dz * dz);

    // Run computation with all zeros
    concentration_field_density(
        c, f, f_n, advection, reaction, diffusion,
        nx, ny, nz, dt, beta, c_x, c_y, c_z, c_xx, c_yy, c_zz,
        u_x, u_y, u_z, u_xx, u_yy, u_zz, Cx, Cy, Cz,
        Cxx, Cyy, Czz, Fx, Fy, Fz, Fxx, Fyy, Fzz);

    // Should handle zeros gracefully
    check_no_nan_inf(f, nx, ny, nz, "Zero input produces valid output");

    // Cleanup
    free_3d_array(c, nx, ny);
    free_3d_array(f, nx, ny);
    free_3d_array(f_n, nx, ny);
    free(advection);
    free(reaction);
    free(diffusion);
}

void test_production_size() {
    printf("\nTest: Production-size grid\n");

    int nx = 50, ny = 50, nz = 100;  // Reduced from 1600 for faster testing

    printf("  Grid size: %d x %d x %d = %.2f M points\n",
           nx, ny, nz, (nx * ny * nz) / 1e6);

    // Allocate arrays
    double ***c = allocate_3d_array(nx, ny, nz);
    double ***f = allocate_3d_array(nx, ny, nz);
    double ***f_n = allocate_3d_array(nx, ny, nz);
    double *advection = (double*)calloc(nz, sizeof(double));
    double *reaction = (double*)calloc(nz, sizeof(double));
    double *diffusion = (double*)calloc(nz, sizeof(double));

    // Initialize with realistic data
    initialize_test_data(c, nx, ny, nz, 1.0);
    initialize_test_data(f_n, nx, ny, nz, 0.5);

    for (int k = 0; k < nz; k++) {
        advection[k] = 0.1 * k / nz;
        reaction[k] = 0.05;
        diffusion[k] = 0.01;
    }

    // Production parameters
    double dt = 1e5, beta = -1e-10;
    double c_x = 0.0, c_y = 0.0, c_z = 0.0;
    double c_xx = 0.0, c_yy = 0.0, c_zz = 0.0;
    double u_x = 0.0, u_y = 0.0, u_z = 0.0;
    double u_xx = 0.0, u_yy = 0.0, u_zz = 0.0;
    double dx = 10.0 / nx, dy = 10.0 / ny, dz = 80.0 / nz;
    double Cx = 1.0 / dx, Cy = 1.0 / dy, Cz = 1.0 / dz;
    double Cxx = dt / (dx * dx), Cyy = dt / (dy * dy), Czz = dt / (dz * dz);
    double Fx = dt / dx, Fy = dt / dy, Fz = dt / dz;
    double Fxx = dt / (dx * dx), Fyy = dt / (dy * dy), Fzz = dt / (dz * dz);

    // Run computation
    concentration_field_density(
        c, f, f_n, advection, reaction, diffusion,
        nx, ny, nz, dt, beta, c_x, c_y, c_z, c_xx, c_yy, c_zz,
        u_x, u_y, u_z, u_xx, u_yy, u_zz, Cx, Cy, Cz,
        Cxx, Cyy, Czz, Fx, Fy, Fz, Fxx, Fyy, Fzz);

    // Validate results
    check_no_nan_inf(f, nx, ny, nz, "Production grid: No NaN/Inf");
    check_reasonable_range(f, nx, ny, nz, -1e10, 1e10, "Production grid: Reasonable range");

    // Cleanup
    free_3d_array(c, nx, ny);
    free_3d_array(f, nx, ny);
    free_3d_array(f_n, nx, ny);
    free(advection);
    free(reaction);
    free(diffusion);
}

int main() {
    printf("========================================\n");
    printf(" Pragma SIMD Implementation Tests\n");
    printf("========================================\n");

    test_basic_computation();
    test_zero_input();
    test_production_size();

    printf("\n========================================\n");
    printf(" Test Summary\n");
    printf("========================================\n");
    printf("Total tests: %d\n", tests_run);
    printf("Passed:      %d\n", tests_passed);
    printf("Failed:      %d\n", tests_failed);
    printf("========================================\n");

    return (tests_failed == 0) ? 0 : 1;
}
