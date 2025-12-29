#ifndef GPU_PIPELINE_H
#define GPU_PIPELINE_H

#ifdef __APPLE__

// Initialize GPU buffers with initial data (call once at startup)
bool gpu_pipeline_init_buffers(
    const double *c_init, const double *c_n_init,
    const double *f_init, const double *f_n_init,
    const double *advection, const double *reaction,
    const double *diffusion,
    int nx, int ny, int nz);

// Execute one iteration entirely on GPU
// All three kernels (oldtonew, concentration_field, density_field) run on GPU
// Data stays resident on GPU - NO CPU-GPU transfers!
void gpu_pipeline_execute_iteration(
    int nx, int ny, int nz,
    double D_c, double Cxx, double Cyy, double Czz,
    double dt, double beta,
    double Cx, double Cy, double Cz,
    double Fx, double Fy, double Fz,
    double Fxx, double Fyy, double Fzz);

// Wait for GPU to finish all pending work
void gpu_pipeline_wait();

// Read results from GPU to CPU (only when needed for output/visualization)
void gpu_pipeline_read_results(double *c_out, double *f_out, int nx, int ny, int nz);

// Cleanup GPU resources
void gpu_pipeline_cleanup();

#endif // __APPLE__

#endif // GPU_PIPELINE_H
