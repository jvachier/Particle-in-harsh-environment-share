#ifdef __APPLE__

#include "headers/gpu_pipeline.h"
#include <Metal/Metal.h>
#include <Foundation/Foundation.h>
#include <cstdio>
#include <cstdlib>

// Global Metal objects
static id<MTLDevice> g_gpu_device = nil;
static id<MTLCommandQueue> g_gpu_commandQueue = nil;
static id<MTLComputePipelineState> g_oldtonew_pipeline = nil;
static id<MTLComputePipelineState> g_concentration_field_pipeline = nil;
static id<MTLComputePipelineState> g_density_field_pipeline = nil;
static bool g_gpu_pipeline_initialized = false;

// GPU-resident buffers (stay on GPU between iterations)
static id<MTLBuffer> g_c_buffer = nil;
static id<MTLBuffer> g_c_n_buffer = nil;
static id<MTLBuffer> g_f_buffer = nil;
static id<MTLBuffer> g_f_n_buffer = nil;
static id<MTLBuffer> g_advection_buffer = nil;
static id<MTLBuffer> g_reaction_buffer = nil;
static id<MTLBuffer> g_diffusion_buffer = nil;
static size_t g_gpu_cached_size = 0;
static size_t g_gpu_cached_1d_size = 0;

// Initialize GPU pipeline (call once at startup)
static bool initialize_gpu_pipeline() {
    if (g_gpu_pipeline_initialized) return true;

    @autoreleasepool {
        g_gpu_device = MTLCreateSystemDefaultDevice();
        if (!g_gpu_device) {
            fprintf(stderr, "Error: Metal is not supported\n");
            return false;
        }

        NSError *error = nil;

        // Embedded Metal shaders for all three kernels
        NSString *shaderSource = @R"(
#include <metal_stdlib>
using namespace metal;

// Kernel 1: oldtonew - copy arrays
kernel void oldtonew_kernel(
    const device float* c [[buffer(0)]],
    const device float* f [[buffer(1)]],
    device float* c_n [[buffer(2)]],
    device float* f_n [[buffer(3)]],
    uint gid [[thread_position_in_grid]]) {

    c_n[gid] = c[gid];
    f_n[gid] = f[gid];
}

// Kernel 2: concentration_field
kernel void concentration_field_kernel(
    const device float* c_n [[buffer(0)]],
    device float* c [[buffer(1)]],
    constant int& nx [[buffer(2)]],
    constant int& ny [[buffer(3)]],
    constant int& nz [[buffer(4)]],
    constant float& D_c [[buffer(5)]],
    constant float& Cxx [[buffer(6)]],
    constant float& Cyy [[buffer(7)]],
    constant float& Czz [[buffer(8)]],
    uint3 gid [[thread_position_in_grid]]) {

    int i = gid.x + 1;
    int j = gid.y + 1;
    int k = gid.z + 1;

    if (i >= nx - 1 || j >= ny - 1 || k >= nz - 1) return;

    auto idx = [&](int ii, int jj, int kk) -> int {
        return ii * (ny * nz) + jj * nz + kk;
    };

    int current = idx(i, j, k);
    float c_n_curr = c_n[current];

    float c_xx_local = c_n[idx(i-1, j, k)] - 2.0f * c_n_curr + c_n[idx(i+1, j, k)];
    float c_yy_local = c_n[idx(i, j-1, k)] - 2.0f * c_n_curr + c_n[idx(i, j+1, k)];
    float c_zz_local = c_n[idx(i, j, k-1)] - 2.0f * c_n_curr + c_n[idx(i, j, k+1)];

    c[current] = c_n_curr + D_c * (Cxx * c_xx_local + Cyy * c_yy_local + Czz * c_zz_local);
}

// Kernel 3: concentration_field_density
kernel void concentration_field_density_kernel(
    const device float* c [[buffer(0)]],
    const device float* f_n [[buffer(1)]],
    device float* f [[buffer(2)]],
    const device float* advection [[buffer(3)]],
    const device float* reaction [[buffer(4)]],
    const device float* diffusion [[buffer(5)]],
    constant int& nx [[buffer(6)]],
    constant int& ny [[buffer(7)]],
    constant int& nz [[buffer(8)]],
    constant float& dt [[buffer(9)]],
    constant float& beta [[buffer(10)]],
    constant float& Cx [[buffer(11)]],
    constant float& Cy [[buffer(12)]],
    constant float& Cz [[buffer(13)]],
    constant float& Cxx [[buffer(14)]],
    constant float& Cyy [[buffer(15)]],
    constant float& Czz [[buffer(16)]],
    constant float& Fx [[buffer(17)]],
    constant float& Fy [[buffer(18)]],
    constant float& Fz [[buffer(19)]],
    constant float& Fxx [[buffer(20)]],
    constant float& Fyy [[buffer(21)]],
    constant float& Fzz [[buffer(22)]],
    uint3 gid [[thread_position_in_grid]]) {

    int i = gid.x + 1;
    int j = gid.y + 1;
    int k = gid.z + 1;

    if (i >= nx - 1 || j >= ny - 1 || k >= nz - 1) return;

    auto idx = [&](int ii, int jj, int kk) -> int {
        return ii * (ny * nz) + jj * nz + kk;
    };

    int current = idx(i, j, k);

    float c_x = c[idx(i+1, j, k)] - c[current];
    float c_y = c[idx(i, j+1, k)] - c[current];
    float c_z = c[idx(i, j, k+1)] - c[current];

    float c_xx = c[idx(i-1, j, k)] - 2.0f * c[current] + c[idx(i+1, j, k)];
    float c_yy = c[idx(i, j-1, k)] - 2.0f * c[current] + c[idx(i, j+1, k)];
    float c_zz = c[idx(i, j, k-1)] - 2.0f * c[current] + c[idx(i, j, k+1)];

    float u_x = f_n[idx(i+1, j, k)] - f_n[current];
    float u_y = f_n[idx(i, j+1, k)] - f_n[current];
    float u_z = f_n[idx(i, j, k+1)] - f_n[current];

    float u_xx = f_n[idx(i-1, j, k)] - 2.0f * f_n[current] + f_n[idx(i+1, j, k)];
    float u_yy = f_n[idx(i, j-1, k)] - 2.0f * f_n[current] + f_n[idx(i, j+1, k)];
    float u_zz = f_n[idx(i, j, k-1)] - 2.0f * f_n[current] + f_n[idx(i, j, k+1)];

    float chemotaxis = beta * (
        Fx * u_x * Cx * c_x +
        Fy * u_y * Cy * c_y +
        Fz * u_z * Cz * c_z +
        f_n[current] * (Cxx * c_xx + Cyy * c_yy + Czz * c_zz)
    );

    float advection_term = Fz * advection[k] * u_z;
    float reaction_term = -dt * reaction[k] * f_n[current];
    float diffusion_term = diffusion[k] * (Fxx * u_xx + Fyy * u_yy + Fzz * u_zz);

    f[current] = f_n[current] + chemotaxis + advection_term + reaction_term + diffusion_term;
}
)";

        id<MTLLibrary> library = [g_gpu_device newLibraryWithSource:shaderSource
                                                        options:nil
                                                          error:&error];
        if (error || !library) {
            fprintf(stderr, "Error: Failed to compile GPU pipeline shaders: %s\n",
                    [[error localizedDescription] UTF8String]);
            return false;
        }

        // Create pipelines for all three kernels
        id<MTLFunction> oldtonew_func = [library newFunctionWithName:@"oldtonew_kernel"];
        id<MTLFunction> cf_func = [library newFunctionWithName:@"concentration_field_kernel"];
        id<MTLFunction> cfd_func = [library newFunctionWithName:@"concentration_field_density_kernel"];

        if (!oldtonew_func || !cf_func || !cfd_func) {
            fprintf(stderr, "Error: Could not find GPU pipeline kernel functions\n");
            return false;
        }

        g_oldtonew_pipeline = [g_gpu_device newComputePipelineStateWithFunction:oldtonew_func error:&error];
        g_concentration_field_pipeline = [g_gpu_device newComputePipelineStateWithFunction:cf_func error:&error];
        g_density_field_pipeline = [g_gpu_device newComputePipelineStateWithFunction:cfd_func error:&error];

        if (!g_oldtonew_pipeline || !g_concentration_field_pipeline || !g_density_field_pipeline) {
            fprintf(stderr, "Error: Failed to create GPU pipeline states\n");
            return false;
        }

        g_gpu_commandQueue = [g_gpu_device newCommandQueue];
        if (!g_gpu_commandQueue) {
            fprintf(stderr, "Error: Failed to create GPU command queue\n");
            return false;
        }

        g_gpu_pipeline_initialized = true;
        printf("✓ GPU Pipeline initialized (all 3 kernels ready)\n");
        return true;
    }
}

// Initialize GPU buffers (call once with initial data)
bool gpu_pipeline_init_buffers(
    const double *c_init, const double *c_n_init,
    const double *f_init, const double *f_n_init,
    const double *advection, const double *reaction,
    const double *diffusion,
    int nx, int ny, int nz) {

    if (!initialize_gpu_pipeline()) {
        return false;
    }

    @autoreleasepool {
        size_t totalSize = nx * ny * nz;
        size_t buffer_size = totalSize * sizeof(float);

        // Allocate GPU buffers (will stay resident)
        if (g_c_buffer == nil || g_gpu_cached_size != totalSize) {
            g_c_buffer = [g_gpu_device newBufferWithLength:buffer_size
                                                   options:MTLResourceStorageModeShared];
            g_c_n_buffer = [g_gpu_device newBufferWithLength:buffer_size
                                                     options:MTLResourceStorageModeShared];
            g_f_buffer = [g_gpu_device newBufferWithLength:buffer_size
                                                   options:MTLResourceStorageModeShared];
            g_f_n_buffer = [g_gpu_device newBufferWithLength:buffer_size
                                                     options:MTLResourceStorageModeShared];
            g_advection_buffer = [g_gpu_device newBufferWithLength:nz * sizeof(float)
                                                           options:MTLResourceStorageModeShared];
            g_reaction_buffer = [g_gpu_device newBufferWithLength:nz * sizeof(float)
                                                          options:MTLResourceStorageModeShared];
            g_diffusion_buffer = [g_gpu_device newBufferWithLength:nz * sizeof(float)
                                                           options:MTLResourceStorageModeShared];

            if (!g_c_buffer || !g_c_n_buffer || !g_f_buffer || !g_f_n_buffer ||
                !g_advection_buffer || !g_reaction_buffer || !g_diffusion_buffer) {
                fprintf(stderr, "Error: Failed to allocate GPU buffers\n");
                return false;
            }

            g_gpu_cached_size = totalSize;
            g_gpu_cached_1d_size = nz;
        }

        // Transfer initial data to GPU (ONE TIME ONLY)
        printf("✓ Transferring initial data to GPU...\n");
        float *c_gpu = (float*)[g_c_buffer contents];
        float *c_n_gpu = (float*)[g_c_n_buffer contents];
        float *f_gpu = (float*)[g_f_buffer contents];
        float *f_n_gpu = (float*)[g_f_n_buffer contents];
        float *adv_gpu = (float*)[g_advection_buffer contents];
        float *react_gpu = (float*)[g_reaction_buffer contents];
        float *diff_gpu = (float*)[g_diffusion_buffer contents];

        #pragma omp parallel for
        for (size_t i = 0; i < totalSize; i++) {
            c_gpu[i] = (float)c_init[i];
            c_n_gpu[i] = (float)c_n_init[i];
            f_gpu[i] = (float)f_init[i];
            f_n_gpu[i] = (float)f_n_init[i];
        }

        #pragma omp parallel for
        for (int i = 0; i < nz; i++) {
            adv_gpu[i] = (float)advection[i];
            react_gpu[i] = (float)reaction[i];
            diff_gpu[i] = (float)diffusion[i];
        }

        printf("✓ GPU buffers initialized (%zu MB on GPU)\n",
               (buffer_size * 4 + nz * sizeof(float) * 3) / (1024 * 1024));
        return true;
    }
}

// Execute one iteration entirely on GPU
void gpu_pipeline_execute_iteration(
    int nx, int ny, int nz,
    double D_c, double Cxx, double Cyy, double Czz,
    double dt, double beta,
    double Cx, double Cy, double Cz,
    double Fx, double Fy, double Fz,
    double Fxx, double Fyy, double Fzz) {

    @autoreleasepool {
        size_t totalSize = nx * ny * nz;

        // Create single command buffer for all three kernels
        id<MTLCommandBuffer> commandBuffer = [g_gpu_commandQueue commandBuffer];
        commandBuffer.label = @"GPU Pipeline Iteration";

        // ===== KERNEL 1: oldtonew (c→c_n, f→f_n) =====
        {
            id<MTLComputeCommandEncoder> encoder = [commandBuffer computeCommandEncoder];
            encoder.label = @"oldtonew";
            [encoder setComputePipelineState:g_oldtonew_pipeline];
            [encoder setBuffer:g_c_buffer offset:0 atIndex:0];
            [encoder setBuffer:g_f_buffer offset:0 atIndex:1];
            [encoder setBuffer:g_c_n_buffer offset:0 atIndex:2];
            [encoder setBuffer:g_f_n_buffer offset:0 atIndex:3];

            // Optimized thread group size for 1D memory copy
            MTLSize gridSize = MTLSizeMake(totalSize, 1, 1);
            MTLSize threadgroupSize = MTLSizeMake(256, 1, 1);
            [encoder dispatchThreads:gridSize threadsPerThreadgroup:threadgroupSize];
            [encoder endEncoding];
        }

        // ===== KERNEL 2: concentration_field (c_n → c) =====
        {
            id<MTLComputeCommandEncoder> encoder = [commandBuffer computeCommandEncoder];
            encoder.label = @"concentration_field";
            [encoder setComputePipelineState:g_concentration_field_pipeline];
            [encoder setBuffer:g_c_n_buffer offset:0 atIndex:0];
            [encoder setBuffer:g_c_buffer offset:0 atIndex:1];
            [encoder setBytes:&nx length:sizeof(int) atIndex:2];
            [encoder setBytes:&ny length:sizeof(int) atIndex:3];
            [encoder setBytes:&nz length:sizeof(int) atIndex:4];

            float D_c_f = (float)D_c;
            float Cxx_f = (float)Cxx;
            float Cyy_f = (float)Cyy;
            float Czz_f = (float)Czz;

            [encoder setBytes:&D_c_f length:sizeof(float) atIndex:5];
            [encoder setBytes:&Cxx_f length:sizeof(float) atIndex:6];
            [encoder setBytes:&Cyy_f length:sizeof(float) atIndex:7];
            [encoder setBytes:&Czz_f length:sizeof(float) atIndex:8];

            // Thread group size for 3D stencil operations
            MTLSize gridSize = MTLSizeMake(nx - 2, ny - 2, nz - 2);
            MTLSize threadgroupSize = MTLSizeMake(8, 8, 4);
            [encoder dispatchThreads:gridSize threadsPerThreadgroup:threadgroupSize];
            [encoder endEncoding];
        }

        // ===== KERNEL 3: concentration_field_density (c, f_n → f) =====
        {
            id<MTLComputeCommandEncoder> encoder = [commandBuffer computeCommandEncoder];
            encoder.label = @"density_field";
            [encoder setComputePipelineState:g_density_field_pipeline];
            [encoder setBuffer:g_c_buffer offset:0 atIndex:0];
            [encoder setBuffer:g_f_n_buffer offset:0 atIndex:1];
            [encoder setBuffer:g_f_buffer offset:0 atIndex:2];
            [encoder setBuffer:g_advection_buffer offset:0 atIndex:3];
            [encoder setBuffer:g_reaction_buffer offset:0 atIndex:4];
            [encoder setBuffer:g_diffusion_buffer offset:0 atIndex:5];

            [encoder setBytes:&nx length:sizeof(int) atIndex:6];
            [encoder setBytes:&ny length:sizeof(int) atIndex:7];
            [encoder setBytes:&nz length:sizeof(int) atIndex:8];

            float dt_f = (float)dt, beta_f = (float)beta;
            float Cx_f = (float)Cx, Cy_f = (float)Cy, Cz_f = (float)Cz;
            float Cxx_f = (float)Cxx, Cyy_f = (float)Cyy, Czz_f = (float)Czz;
            float Fx_f = (float)Fx, Fy_f = (float)Fy, Fz_f = (float)Fz;
            float Fxx_f = (float)Fxx, Fyy_f = (float)Fyy, Fzz_f = (float)Fzz;

            [encoder setBytes:&dt_f length:sizeof(float) atIndex:9];
            [encoder setBytes:&beta_f length:sizeof(float) atIndex:10];
            [encoder setBytes:&Cx_f length:sizeof(float) atIndex:11];
            [encoder setBytes:&Cy_f length:sizeof(float) atIndex:12];
            [encoder setBytes:&Cz_f length:sizeof(float) atIndex:13];
            [encoder setBytes:&Cxx_f length:sizeof(float) atIndex:14];
            [encoder setBytes:&Cyy_f length:sizeof(float) atIndex:15];
            [encoder setBytes:&Czz_f length:sizeof(float) atIndex:16];
            [encoder setBytes:&Fx_f length:sizeof(float) atIndex:17];
            [encoder setBytes:&Fy_f length:sizeof(float) atIndex:18];
            [encoder setBytes:&Fz_f length:sizeof(float) atIndex:19];
            [encoder setBytes:&Fxx_f length:sizeof(float) atIndex:20];
            [encoder setBytes:&Fyy_f length:sizeof(float) atIndex:21];
            [encoder setBytes:&Fzz_f length:sizeof(float) atIndex:22];

            // Thread group size for complex 3D stencil
            MTLSize gridSize = MTLSizeMake(nx - 2, ny - 2, nz - 2);
            MTLSize threadgroupSize = MTLSizeMake(8, 8, 4);
            [encoder dispatchThreads:gridSize threadsPerThreadgroup:threadgroupSize];
            [encoder endEncoding];
        }

        // Submit all three kernels asynchronously
        [commandBuffer commit];

        // Don't wait - let GPU run async!
        // We only wait when we need to read results
    }
}

// Wait for GPU to finish (call before reading results)
void gpu_pipeline_wait() {
    // Just commit an empty buffer and wait - ensures all previous work is done
    @autoreleasepool {
        id<MTLCommandBuffer> syncBuffer = [g_gpu_commandQueue commandBuffer];
        [syncBuffer commit];
        [syncBuffer waitUntilCompleted];
    }
}

// Read results from GPU to CPU (only when needed for output)
void gpu_pipeline_read_results(double *c_out, double *f_out, int nx, int ny, int nz) {
    gpu_pipeline_wait();  // Ensure GPU is done

    @autoreleasepool {
        size_t totalSize = nx * ny * nz;
        float *c_gpu = (float*)[g_c_buffer contents];
        float *f_gpu = (float*)[g_f_buffer contents];

        #pragma omp parallel for
        for (size_t i = 0; i < totalSize; i++) {
            c_out[i] = (double)c_gpu[i];
            f_out[i] = (double)f_gpu[i];
        }
    }
}

// Cleanup
void gpu_pipeline_cleanup() {
    @autoreleasepool {
        g_c_buffer = nil;
        g_c_n_buffer = nil;
        g_f_buffer = nil;
        g_f_n_buffer = nil;
        g_advection_buffer = nil;
        g_reaction_buffer = nil;
        g_diffusion_buffer = nil;
        g_oldtonew_pipeline = nil;
        g_concentration_field_pipeline = nil;
        g_density_field_pipeline = nil;
        g_gpu_commandQueue = nil;
        g_gpu_device = nil;
        g_gpu_pipeline_initialized = false;
    }
}

#endif // __APPLE__
