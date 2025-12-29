#ifdef __APPLE__

#include "headers/concentration_field.h"
#include "headers/array_utils.h"
#include <Metal/Metal.h>
#include <Foundation/Foundation.h>
#include <cstdio>
#include <cstdlib>

// Global Metal objects for concentration_field (separate from density)
static id<MTLDevice> g_cf_device = nil;
static id<MTLCommandQueue> g_cf_commandQueue = nil;
static id<MTLComputePipelineState> g_cf_pipeline = nil;
static bool g_cf_metal_initialized = false;

// CRITICAL: Cached GPU buffers - reuse across iterations to prevent memory leak
static id<MTLBuffer> g_cf_c_n_buffer = nil;
static id<MTLBuffer> g_cf_c_buffer = nil;
static size_t g_cf_cached_buffer_size = 0;

// Note: No conversion functions needed - arrays are already linear!

// Initialize Metal for concentration_field
static bool initialize_metal_cf() {
    if (g_cf_metal_initialized) return true;

    @autoreleasepool {
        g_cf_device = MTLCreateSystemDefaultDevice();
        if (!g_cf_device) {
            fprintf(stderr, "Error: Metal is not supported on this device\n");
            return false;
        }

        NSError *error = nil;

        // Embedded Metal shader source (avoids file path issues)
        NSString *shaderSource = @R"(
#include <metal_stdlib>
using namespace metal;

kernel void concentration_field_kernel(
    device const float *c_n [[buffer(0)]],
    device float *c [[buffer(1)]],
    constant uint &nx [[buffer(2)]],
    constant uint &ny [[buffer(3)]],
    constant uint &nz [[buffer(4)]],
    constant float &D_c [[buffer(5)]],
    constant float &Cxx [[buffer(6)]],
    constant float &Cyy [[buffer(7)]],
    constant float &Czz [[buffer(8)]],
    uint3 gid [[thread_position_in_grid]])
{
    uint i = gid.x + 1;
    uint j = gid.y + 1;
    uint k = gid.z + 1;

    if (i >= nx - 1 || j >= ny - 1 || k >= nz - 1) return;

    uint idx = i * ny * nz + j * nz + k;
    uint idx_im1 = (i-1) * ny * nz + j * nz + k;
    uint idx_ip1 = (i+1) * ny * nz + j * nz + k;
    uint idx_jm1 = i * ny * nz + (j-1) * nz + k;
    uint idx_jp1 = i * ny * nz + (j+1) * nz + k;

    float c_n_curr = c_n[idx];

    float c_xx_local = c_n[idx_im1] - 2.0f * c_n_curr + c_n[idx_ip1];
    float c_yy_local = c_n[idx_jm1] - 2.0f * c_n_curr + c_n[idx_jp1];
    float c_zz_local = c_n[idx-1] - 2.0f * c_n_curr + c_n[idx+1];

    c[idx] = c_n_curr + D_c * (Cxx * c_xx_local + Cyy * c_yy_local + Czz * c_zz_local);
}
)";

        // Compile Metal shader
        id<MTLLibrary> library = [g_cf_device newLibraryWithSource:shaderSource
                                                           options:nil
                                                             error:&error];
        if (error || !library) {
            fprintf(stderr, "Error: Failed to compile Metal shader\n");
            fprintf(stderr, "Error details: %s\n", [[error localizedDescription] UTF8String]);
            return false;
        }

        // Get kernel function
        id<MTLFunction> kernelFunction = [library newFunctionWithName:@"concentration_field_kernel"];
        if (!kernelFunction) {
            fprintf(stderr, "Error: Could not find kernel function 'concentration_field_kernel'\n");
            return false;
        }

        // Create compute pipeline
        g_cf_pipeline = [g_cf_device newComputePipelineStateWithFunction:kernelFunction
                                                                   error:&error];
        if (error || !g_cf_pipeline) {
            fprintf(stderr, "Error: Failed to create compute pipeline\n");
            fprintf(stderr, "Error details: %s\n", [[error localizedDescription] UTF8String]);
            return false;
        }

        // Create command queue
        g_cf_commandQueue = [g_cf_device newCommandQueue];
        if (!g_cf_commandQueue) {
            fprintf(stderr, "Error: Failed to create command queue\n");
            return false;
        }

        g_cf_metal_initialized = true;
        printf("Metal GPU initialized successfully for concentration_field\n");
        return true;
    }
}

// Main Metal GPU function for concentration_field
void concentration_field_metal(
    const double *c_n, double *c,
    int nx, int ny, int nz,
    double c_xx, double c_yy, double c_zz,
    double D_c, double Cxx, double Cyy, double Czz) {

    // Initialize Metal if needed
    if (!initialize_metal_cf()) {
        fprintf(stderr, "Warning: Falling back to CPU version\n");
        concentration_field(c_n, c, nx, ny, nz, c_xx, c_yy, c_zz, D_c, Cxx, Cyy, Czz);
        return;
    }

    @autoreleasepool {
        int total_elements = nx * ny * nz;
        size_t buffer_size = total_elements * sizeof(float);

        // CRITICAL FIX: Allocate GPU buffers only once, reuse on subsequent calls
        if (g_cf_c_n_buffer == nil || g_cf_cached_buffer_size != buffer_size) {
            // First call or grid size changed - allocate new buffers
            g_cf_c_n_buffer = [g_cf_device newBufferWithLength:buffer_size
                                                        options:MTLResourceStorageModeShared];
            g_cf_c_buffer = [g_cf_device newBufferWithLength:buffer_size
                                                      options:MTLResourceStorageModeShared];
            g_cf_cached_buffer_size = buffer_size;

            if (!g_cf_c_n_buffer || !g_cf_c_buffer) {
                fprintf(stderr, "Error: Failed to allocate GPU buffers\n");
                return;
            }
        }

        // Get pointers to GPU buffer memory (shared mode allows direct access)
        float *c_n_gpu = (float*)[g_cf_c_n_buffer contents];
        float *c_gpu = (float*)[g_cf_c_buffer contents];

        // ZERO-COPY: Direct linear conversion (already linear!)
        // Convert double→float for GPU compatibility
        #pragma omp parallel for
        for (int i = 0; i < total_elements; i++) {
            c_n_gpu[i] = (float)c_n[i];
        }

        // Create buffers for scalar parameters
        unsigned int nx_uint = (unsigned int)nx;
        unsigned int ny_uint = (unsigned int)ny;
        unsigned int nz_uint = (unsigned int)nz;
        float D_c_float = (float)D_c;
        float Cxx_float = (float)Cxx;
        float Cyy_float = (float)Cyy;
        float Czz_float = (float)Czz;

        id<MTLBuffer> nx_buffer = [g_cf_device newBufferWithBytes:&nx_uint
                                                            length:sizeof(unsigned int)
                                                           options:MTLResourceStorageModeShared];
        id<MTLBuffer> ny_buffer = [g_cf_device newBufferWithBytes:&ny_uint
                                                            length:sizeof(unsigned int)
                                                           options:MTLResourceStorageModeShared];
        id<MTLBuffer> nz_buffer = [g_cf_device newBufferWithBytes:&nz_uint
                                                            length:sizeof(unsigned int)
                                                           options:MTLResourceStorageModeShared];
        id<MTLBuffer> D_c_buffer = [g_cf_device newBufferWithBytes:&D_c_float
                                                             length:sizeof(float)
                                                            options:MTLResourceStorageModeShared];
        id<MTLBuffer> Cxx_buffer = [g_cf_device newBufferWithBytes:&Cxx_float
                                                             length:sizeof(float)
                                                            options:MTLResourceStorageModeShared];
        id<MTLBuffer> Cyy_buffer = [g_cf_device newBufferWithBytes:&Cyy_float
                                                             length:sizeof(float)
                                                            options:MTLResourceStorageModeShared];
        id<MTLBuffer> Czz_buffer = [g_cf_device newBufferWithBytes:&Czz_float
                                                             length:sizeof(float)
                                                            options:MTLResourceStorageModeShared];

        // Create command buffer and encoder
        id<MTLCommandBuffer> commandBuffer = [g_cf_commandQueue commandBuffer];
        id<MTLComputeCommandEncoder> encoder = [commandBuffer computeCommandEncoder];

        [encoder setComputePipelineState:g_cf_pipeline];

        // Set buffers (use cached buffers!)
        [encoder setBuffer:g_cf_c_n_buffer offset:0 atIndex:0];
        [encoder setBuffer:g_cf_c_buffer offset:0 atIndex:1];
        [encoder setBuffer:nx_buffer offset:0 atIndex:2];
        [encoder setBuffer:ny_buffer offset:0 atIndex:3];
        [encoder setBuffer:nz_buffer offset:0 atIndex:4];
        [encoder setBuffer:D_c_buffer offset:0 atIndex:5];
        [encoder setBuffer:Cxx_buffer offset:0 atIndex:6];
        [encoder setBuffer:Cyy_buffer offset:0 atIndex:7];
        [encoder setBuffer:Czz_buffer offset:0 atIndex:8];

        // Calculate thread groups
        MTLSize threadsPerThreadgroup = MTLSizeMake(8, 8, 8);  // 512 threads per group
        MTLSize threadgroupsPerGrid = MTLSizeMake(
            (nx - 2 + 7) / 8,  // Ceiling division
            (ny - 2 + 7) / 8,
            (nz - 2 + 7) / 8
        );

        [encoder dispatchThreadgroups:threadgroupsPerGrid
                threadsPerThreadgroup:threadsPerThreadgroup];

        [encoder endEncoding];
        [commandBuffer commit];
        [commandBuffer waitUntilCompleted];

        // Check for errors
        if (commandBuffer.error) {
            fprintf(stderr, "Error: Metal command buffer failed\n");
            fprintf(stderr, "Error details: %s\n",
                    [[commandBuffer.error localizedDescription] UTF8String]);
        }

        // ZERO-COPY: Direct linear conversion (already linear!)
        // Convert float→double from GPU result
        #pragma omp parallel for
        for (int i = 0; i < total_elements; i++) {
            c[i] = (double)c_gpu[i];
        }

        // No cleanup needed - buffers are cached!
    }
}

// Cleanup function
void cleanup_metal_cf() {
    @autoreleasepool {
        // Release cached buffers
        g_cf_c_n_buffer = nil;
        g_cf_c_buffer = nil;
        g_cf_cached_buffer_size = 0;

        // Release Metal objects
        g_cf_pipeline = nil;
        g_cf_commandQueue = nil;
        g_cf_device = nil;
        g_cf_metal_initialized = false;
    }
}

#endif  // __APPLE__
