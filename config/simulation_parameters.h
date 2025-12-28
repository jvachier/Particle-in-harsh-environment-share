/*
 * Simulation Parameters Configuration
 * Centralized parameter management
 *
 * IMPORTANT: After changing values here, you need to recompile:
 *   cd implementation_metal && make
 *
 * This is much cleaner than editing hardcoded values scattered throughout main.cpp
 */

#ifndef SIMULATION_PARAMETERS_H
#define SIMULATION_PARAMETERS_H

#include <string>
#include <cmath>

namespace SimulationConfig {

// Physical Constants
struct PhysicalConstants {
    static constexpr double PI = 3.14159265;
    static constexpr double rho_l = 920;           // Liquid density
    static constexpr double q_m = 3.3e5;           // Latent heat
    static constexpr double T_m = 273.15;          // Melting temperature
    static constexpr double delaT = 0.1;           // Temperature difference
    static constexpr double nu = 1e-3;             // Viscosity
    static constexpr double N_i = 100.0e-6;        // Ionic concentration
    static constexpr double rhosq_m = 334.0e6;     // Density parameter
    static constexpr double R_g = 8.31;            // Gas constant
    static constexpr double R = 9.0e-6;            // Particle radius
    static constexpr double kb = 1.38e-23;         // Boltzmann constant
};

// Grid Parameters
struct GridParameters {
    int nx = 50;                    // Grid points in x
    int ny = 50;                    // Grid points in y (should equal nx)
    int nz = 1600;                  // Grid points in z

    double x_length = 10.0;         // Domain length in x
    double y_length = 10.0;         // Domain length in y
    double z_length = 80.0;         // Domain length in z

    // Computed values
    double get_dx() const { return x_length / nx; }
    double get_dy() const { return y_length / ny; }
    double get_dz() const { return z_length / nz; }
};

// Time Integration Parameters
struct TimeParameters {
    // ========================================================================
    // TIME STEP CONFIGURATION
    // ========================================================================
    // Physical time step: dt = 1e5 seconds
    // Real time represented: 1e5 sec = 1.157 days ≈ 1.16 days per step
    //
    // Relation to 1 year:
    //   1 year = 365.25 days = 31,557,600 seconds
    //   Steps per year = 31,557,600 / 1e5 = 315.576 ≈ 316 steps/year
    //
    // Current setting: nt = 15,768 steps
    //   → 15,768 / 316 = 49.9 years ≈ 50 years of simulation
    // ========================================================================

    double dt = 1e5;                // Time step size (seconds)
                                    // 1e5 sec = 1.157 days

    int nt = 15768;                 // Time steps per iteration
                                    // Default: 15,768 steps = ~50 years
                                    // (at dt=1e5, ~316 steps/year)

    int num_years = 12;              // Number of iterations (alpha parameter)
                                    // Total simulation = num_years × nt steps

    // Helper functions for clarity
    int get_total_steps() const {
        return num_years * nt;
    }

    double get_years_simulated() const {
        // Calculate actual years based on dt and nt
        constexpr double SECONDS_PER_YEAR = 365.25 * 24.0 * 3600.0;
        return (nt * dt) / SECONDS_PER_YEAR;
    }

    double get_days_per_step() const {
        constexpr double SECONDS_PER_DAY = 24.0 * 3600.0;
        return dt / SECONDS_PER_DAY;
    }
};

// Chemical Parameters
struct ChemicalParameters {
    double beta = -1e-10;           // Chemotaxis coefficient (positive or negative)
    double D_c = 1e-10;             // Diffusion coefficient for concentration
    double z_0 = 60.0;              // Initial position parameter
};

// Computational Parameters
struct ComputationalParameters {
    int num_threads = 6;            // OpenMP threads
    bool use_metal_gpu = true;      // Use Metal GPU if available (macOS only)
    bool verbose = true;            // Print progress messages
};

// Output Parameters
struct OutputParameters {
    std::string data_dir = "./data/";
    std::string output_suffix = "betan_para";  // "betap_para" or "betan_para"

    // Sample points for profile output
    int sample_x = 25;              // X coordinate for sampling (usually nx/2)
    int sample_y = 25;              // Y coordinate for sampling (usually ny/2)
    int sample_z = 800;             // Z coordinate for sampling (must be < nz=1600)

    bool enable_output = true;      // Enable file output
    int output_frequency = 1;       // Output every N iterations
};

// Complete Configuration Structure
struct Configuration {
    PhysicalConstants physics;
    GridParameters grid;
    TimeParameters time;
    ChemicalParameters chemistry;
    ComputationalParameters compute;
    OutputParameters output;

    // Computed derived parameters
    double get_A_3() const {
        return physics.rhosq_m * physics.delaT *
               pow(physics.R_g * physics.T_m * physics.N_i, 2) /
               (6.0 * physics.nu * physics.R * physics.T_m);
    }

    double get_A_2() const {
        return (physics.rho_l * physics.q_m * physics.delaT) / physics.T_m;
    }

    double get_AA() const {
        return get_A_3() / pow(get_A_2(), 3);
    }

    double get_BB() const {
        double A_2 = get_A_2();
        return (pow(physics.R_g * physics.T_m * physics.N_i, 3) /
                (8.0 * physics.PI * physics.nu * pow(physics.R, 4) * pow(A_2, 3))) *
                physics.kb * physics.T_m;
    }

    double get_D_a() const {
        return 100.0 * get_BB();
    }

    double get_norm1() const {
        return physics.PI * 7.926;
    }

    double get_norm2() const {
        return pow(physics.PI, 3.0 / 2.0);
    }

    void print_summary() const {
        printf("════════════════════════════════════════════════════════════════\n");
        printf("  Simulation Configuration\n");
        printf("════════════════════════════════════════════════════════════════\n");
        printf("\n");
        printf("Grid Parameters:\n");
        printf("  Dimensions: %d × %d × %d = %.2f M grid points\n",
               grid.nx, grid.ny, grid.nz,
               (grid.nx * grid.ny * grid.nz) / 1e6);
        printf("  Domain size: %.1f × %.1f × %.1f\n",
               grid.x_length, grid.y_length, grid.z_length);
        printf("  Grid spacing: Δx=%.6f, Δy=%.6f, Δz=%.6f\n",
               grid.get_dx(), grid.get_dy(), grid.get_dz());
        printf("\n");
        printf("Time Integration:\n");
        printf("  Time step (dt): %.2e seconds (%.2f days)\n",
               time.dt, time.get_days_per_step());
        printf("  Steps per iteration: %d\n", time.nt);
        printf("  Years per iteration: %.1f years\n", time.get_years_simulated());
        printf("  Number of iterations: %d\n", time.num_years);
        printf("  Total simulation time: %.1f years (%d total steps)\n",
               time.get_years_simulated() * time.num_years,
               time.get_total_steps());
        printf("\n");
        printf("Physical Parameters:\n");
        printf("  Beta (chemotaxis): %.2e\n", chemistry.beta);
        printf("  D_c (diffusion): %.2e\n", chemistry.D_c);
        printf("  z_0 (initial position): %.1f\n", chemistry.z_0);
        printf("\n");
        printf("Computational Settings:\n");
        printf("  OpenMP threads: %d\n", compute.num_threads);
#ifdef __APPLE__
        printf("  Metal GPU acceleration: %s\n",
               compute.use_metal_gpu ? "Enabled" : "Disabled (CPU fallback)");
#else
        printf("  Metal GPU acceleration: Not available (not macOS)\n");
#endif
        printf("  Verbose output: %s\n", compute.verbose ? "Yes" : "No");
        printf("\n");
        printf("Output Settings:\n");
        printf("  Output directory: %s\n", output.data_dir.c_str());
        printf("  Output suffix: %s\n", output.output_suffix.c_str());
        printf("  Sample points: x=%d, y=%d, z=%d\n",
               output.sample_x, output.sample_y, output.sample_z);
        printf("\n");
        printf("════════════════════════════════════════════════════════════════\n");
    }
};

// Default configuration factory
inline Configuration create_default_config() {
    Configuration config;
    // All defaults are set in the struct definitions above
    return config;
}

// Load configuration from command line arguments
inline Configuration load_config_from_args(int argc, char* argv[]) {
    Configuration config = create_default_config();

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "--beta" && i + 1 < argc) {
            config.chemistry.beta = std::stod(argv[++i]);
        } else if (arg == "--nz" && i + 1 < argc) {
            config.grid.nz = std::stoi(argv[++i]);
        } else if (arg == "--nt" && i + 1 < argc) {
            config.time.nt = std::stoi(argv[++i]);
        } else if (arg == "--years" && i + 1 < argc) {
            config.time.num_years = std::stoi(argv[++i]);
        } else if (arg == "--threads" && i + 1 < argc) {
            config.compute.num_threads = std::stoi(argv[++i]);
        } else if (arg == "--no-gpu") {
            config.compute.use_metal_gpu = false;
        } else if (arg == "--output-dir" && i + 1 < argc) {
            config.output.data_dir = argv[++i];
        } else if (arg == "--help") {
            printf("Usage: %s [options]\n", argv[0]);
            printf("Options:\n");
            printf("  --beta VALUE       Set chemotaxis coefficient\n");
            printf("  --nz VALUE         Set number of z grid points\n");
            printf("  --nt VALUE         Set time steps per output\n");
            printf("  --years VALUE      Set number of years to simulate\n");
            printf("  --threads VALUE    Set number of OpenMP threads\n");
            printf("  --no-gpu           Disable Metal GPU acceleration\n");
            printf("  --output-dir PATH  Set output directory\n");
            printf("  --help             Show this help message\n");
            exit(0);
        }
    }

    return config;
}

} // namespace SimulationConfig

#endif // SIMULATION_PARAMETERS_H
