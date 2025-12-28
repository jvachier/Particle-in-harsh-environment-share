#ifdef __APPLE__

#include "headers/concentration_field_density.h"
#include "headers/array_utils.h"
#include <Metal/Metal.h>
#include <Foundation/Foundation.h>
#include <cstdio>
#include <cstdlib>
#include <sys/stat.h>

// Global Metal objects for reuse across multiple calls
static id<MTLDevice> g_device = nil;
static id<MTLCommandQueue> g_commandQueue = nil;
static id<MTLComputePipelineState> g_pipeline = nil;
static bool g_metal_initialized = false;

// CRITICAL: Cached GPU buffers to prevent memory leak
static id<MTLBuffer> g_cBuffer_cached = nil;
static id<MTLBuffer> g_fNBuffer_cached = nil;
static id<MTLBuffer> g_fBuffer_cached = nil;
static id<MTLBuffer> g_advectionBuffer_cached = nil;
static id<MTLBuffer> g_reactionBuffer_cached = nil;
static id<MTLBuffer> g_diffusionBuffer_cached = nil;
static size_t g_cached_grid_size = 0;
static size_t g_cached_1d_size = 0;

// Initialize Metal (call once)
static bool initialize_metal() {
    if (g_metal_initialized) return true;

    @autoreleasepool {
        g_device = MTLCreateSystemDefaultDevice();
        if (!g_device) {
            fprintf(stderr, "Error: Metal is not supported on this device\n");
            return false;
        }

        NSError *error = nil;

        // Embedded Metal shader source for reliability
        // This avoids file path issues and makes deployment easier
        // NOTE: Uses float precision as Metal GPUs don't support double
        //       Conversions happen at CPU/GPU boundary
        NSString *shaderSource = @R"(
#include <metal_stdlib>
using namespace metal;

kernel void compute_concentration_field_density(
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

        id<MTLLibrary> library = [g_device newLibraryWithSource:shaderSource
                                                        options:nil
                                                          error:&error];
        if (error || !library) {
            fprintf(stderr, "Error: Could not create Metal library: %s\n",
                    [[error localizedDescription] UTF8String]);
            return false;
        }

        id<MTLFunction> kernelFunction = [library newFunctionWithName:@"compute_concentration_field_density"];
        if (!kernelFunction) {
            fprintf(stderr, "Error: Could not find kernel function\n");
            return false;
        }

        g_pipeline = [g_device newComputePipelineStateWithFunction:kernelFunction error:&error];
        if (error || !g_pipeline) {
            fprintf(stderr, "Error: Could not create compute pipeline: %s\n",
                    [[error localizedDescription] UTF8String]);
            return false;
        }

        g_commandQueue = [g_device newCommandQueue];
        if (!g_commandQueue) {
            fprintf(stderr, "Error: Could not create command queue\n");
            return false;
        }

        g_metal_initialized = true;
        printf("Metal GPU initialized successfully\n");
    }

    return true;
}

// Metal-accelerated version of concentration_field_density
void concentration_field_density_metal(
    const double *c, double *f, const double *f_n,
    const double *advection, const double *reaction, const double *diffusion,
    int nx, int ny, int nz,
    double dt, double beta, double c_x, double c_y, double c_z,
    double c_xx, double c_yy, double c_zz, double u_x, double u_y,
    double u_z, double u_xx, double u_yy, double u_zz, double Cx,
    double Cy, double Cz, double Cxx, double Cyy, double Czz,
    double Fx, double Fy, double Fz, double Fxx, double Fyy, double Fzz) {

    // Initialize Metal if needed
    if (!initialize_metal()) {
        fprintf(stderr, "Metal initialization failed, falling back to CPU version\n");
        // Fall back to CPU version
        concentration_field_density(c, f, f_n, advection, reaction, diffusion,
                                   nx, ny, nz, dt, beta, c_x, c_y, c_z,
                                   c_xx, c_yy, c_zz, u_x, u_y, u_z,
                                   u_xx, u_yy, u_zz, Cx, Cy, Cz,
                                   Cxx, Cyy, Czz, Fx, Fy, Fz, Fxx, Fyy, Fzz);
        return;
    }

    @autoreleasepool {
        size_t totalSize = nx * ny * nz;

        // CRITICAL FIX: Allocate GPU buffers only once, reuse across iterations
        if (g_cBuffer_cached == nil || g_cached_grid_size != totalSize || g_cached_1d_size != nz) {
            // First call or size changed - allocate new buffers
            g_cBuffer_cached = [g_device newBufferWithLength:totalSize * sizeof(float)
                                                     options:MTLResourceStorageModeShared];
            g_fNBuffer_cached = [g_device newBufferWithLength:totalSize * sizeof(float)
                                                      options:MTLResourceStorageModeShared];
            g_fBuffer_cached = [g_device newBufferWithLength:totalSize * sizeof(float)
                                                     options:MTLResourceStorageModeShared];
            g_advectionBuffer_cached = [g_device newBufferWithLength:nz * sizeof(float)
                                                             options:MTLResourceStorageModeShared];
            g_reactionBuffer_cached = [g_device newBufferWithLength:nz * sizeof(float)
                                                            options:MTLResourceStorageModeShared];
            g_diffusionBuffer_cached = [g_device newBufferWithLength:nz * sizeof(float)
                                                             options:MTLResourceStorageModeShared];
            g_cached_grid_size = totalSize;
            g_cached_1d_size = nz;

            if (!g_cBuffer_cached || !g_fNBuffer_cached || !g_fBuffer_cached) {
                fprintf(stderr, "Error: Failed to allocate GPU buffers\n");
                return;
            }
        }

        // Get direct pointers to GPU buffer memory (shared mode)
        float *c_gpu = (float*)[g_cBuffer_cached contents];
        float *f_n_gpu = (float*)[g_fNBuffer_cached contents];
        float *advection_gpu = (float*)[g_advectionBuffer_cached contents];
        float *reaction_gpu = (float*)[g_reactionBuffer_cached contents];
        float *diffusion_gpu = (float*)[g_diffusionBuffer_cached contents];

        // ZERO-COPY: Direct linear conversion (already linear!)
        // Convert double→float for GPU compatibility
        #pragma omp parallel for
        for (size_t i = 0; i < totalSize; i++) {
            c_gpu[i] = (float)c[i];
            f_n_gpu[i] = (float)f_n[i];
        }

        // Convert 1D arrays directly
        #pragma omp parallel for
        for (int i = 0; i < nz; i++) {
            advection_gpu[i] = (float)advection[i];
            reaction_gpu[i] = (float)reaction[i];
            diffusion_gpu[i] = (float)diffusion[i];
        }

        // Create command buffer and encoder
        id<MTLCommandBuffer> commandBuffer = [g_commandQueue commandBuffer];
        id<MTLComputeCommandEncoder> encoder = [commandBuffer computeCommandEncoder];

        [encoder setComputePipelineState:g_pipeline];

        // Set buffers (use cached buffers!)
        [encoder setBuffer:g_cBuffer_cached offset:0 atIndex:0];
        [encoder setBuffer:g_fNBuffer_cached offset:0 atIndex:1];
        [encoder setBuffer:g_fBuffer_cached offset:0 atIndex:2];
        [encoder setBuffer:g_advectionBuffer_cached offset:0 atIndex:3];
        [encoder setBuffer:g_reactionBuffer_cached offset:0 atIndex:4];
        [encoder setBuffer:g_diffusionBuffer_cached offset:0 atIndex:5];

        // Convert scalar constants to float for GPU
        float dt_f = (float)dt;
        float beta_f = (float)beta;
        float Cx_f = (float)Cx;
        float Cy_f = (float)Cy;
        float Cz_f = (float)Cz;
        float Cxx_f = (float)Cxx;
        float Cyy_f = (float)Cyy;
        float Czz_f = (float)Czz;
        float Fx_f = (float)Fx;
        float Fy_f = (float)Fy;
        float Fz_f = (float)Fz;
        float Fxx_f = (float)Fxx;
        float Fyy_f = (float)Fyy;
        float Fzz_f = (float)Fzz;

        // Set constants
        [encoder setBytes:&nx length:sizeof(int) atIndex:6];
        [encoder setBytes:&ny length:sizeof(int) atIndex:7];
        [encoder setBytes:&nz length:sizeof(int) atIndex:8];
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

        // Calculate grid dimensions (exclude boundaries)
        int compute_nx = nx - 2;
        int compute_ny = ny - 2;
        int compute_nz = nz - 2;

        MTLSize gridSize = MTLSizeMake(compute_nx, compute_ny, compute_nz);

        // Calculate optimal thread group size
        NSUInteger maxThreads = g_pipeline.maxTotalThreadsPerThreadgroup;
        NSUInteger threadGroupX = 8;
        NSUInteger threadGroupY = 8;
        NSUInteger maxZ = maxThreads / (threadGroupX * threadGroupY);
        NSUInteger threadGroupZ = (maxZ < 4) ? maxZ : 4;

        MTLSize threadgroupSize = MTLSizeMake(threadGroupX, threadGroupY, threadGroupZ);

        [encoder dispatchThreads:gridSize threadsPerThreadgroup:threadgroupSize];
        [encoder endEncoding];

        [commandBuffer commit];
        [commandBuffer waitUntilCompleted];

        // ZERO-COPY: Direct linear conversion (already linear!)
        // Convert float→double from GPU result
        float *f_result_gpu = (float*)[g_fBuffer_cached contents];
        #pragma omp parallel for
        for (size_t i = 0; i < totalSize; i++) {
            f[i] = (double)f_result_gpu[i];
        }

        // No cleanup needed - no temporary arrays allocated!
    }
}

// Cleanup function (call at program end)
void cleanup_metal() {
    if (g_metal_initialized) {
        // Release cached buffers
        g_cBuffer_cached = nil;
        g_fNBuffer_cached = nil;
        g_fBuffer_cached = nil;
        g_advectionBuffer_cached = nil;
        g_reactionBuffer_cached = nil;
        g_diffusionBuffer_cached = nil;
        g_cached_grid_size = 0;
        g_cached_1d_size = 0;

        // Release Metal objects
        g_device = nil;
        g_commandQueue = nil;
        g_pipeline = nil;
        g_metal_initialized = false;
    }
}

#endif  // __APPLE__
