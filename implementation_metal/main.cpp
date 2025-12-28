/*
 * Author:	Jeremy Vachier
 * Purpose:	Solving Fokker-Planck equations describing an active particle in a harsh environment using finite difference method
 * Reference: 	Vachier J and Wettlaufer JS (2022) Biolocomotion and Premelting in Ice. Front. Phys. 10:904836.
 * Language: 	C++
 * Date: 	2021-2022
 */
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <iostream>
#include <random>
#include <string>
#include <cmath>

#include "../config/simulation_parameters.h"  // Runtime configuration
#include "headers/definition_file.h"
#include "headers/initialization.h"
#include "headers/initialization_fcts.h"
#include "headers/oldtonew.h"
#include "headers/concentration_field.h"
#include "headers/concentration_field_density.h"
#include "headers/memory_allocation.h"
#include "headers/delocate_memory.h"
#include "headers/print_position.h"
#include "headers/print_initial_position.h"

using namespace std;
using namespace SimulationConfig;

int main(int argc, char *argv[]) {
  FILE *initialz, *initialx, *initialy, *initialcz, *initialcx, *initialcy;

  // Load configuration from command line arguments
  Configuration config = load_config_from_args(argc, argv);

  // Print configuration summary
  config.print_summary();

  omp_set_num_threads(config.compute.num_threads);

  // beta positive
  /*
  initialz = fopen("./data/pz_z_initial_100D_"\
    "N100_0y_30_08_2022_betap_para.dat", "w");
  initialx = fopen("./data/px_x_initial_100D_"\
    "N100_0y_30_08_2022_betap_para.dat", "w");
  initialcz = fopen("./data/c_cz_initial_100D_"\
    "N100_0y_30_08_2022_betap_para.dat", "w");
  initialcx = fopen("./data/c_cx_initial_100D_"\
    "N100_0y_30_08_2022_betap_para.dat", "w");
  */

  // beta negative - using binary format (.bin) for initial positions
  // Create data directory if it doesn't exist
  system("mkdir -p ../data/metal");

  initialz = fopen("../data/metal/pz_z_initial_100D_"\
    "N100_0y_30_08_2022_betan_para.bin", "wb");
  initialx = fopen("../data/metal/px_x_initial_100D_"\
    "N100_0y_30_08_2022_betan_para.bin", "wb");
  initialy = fopen("../data/metal/py_y_initial_100D_"\
    "N100_0y_30_08_2022_betan_para.bin", "wb");
  initialcz = fopen("../data/metal/c_cz_initial_100D_"\
    "N100_0y_30_08_2022_betan_para.bin", "wb");
  initialcx = fopen("../data/metal/c_cx_initial_100D_"\
    "N100_0y_30_08_2022_betan_para.bin", "wb");
  initialcy = fopen("../data/metal/c_cy_initial_100D_"\
    "N100_0y_30_08_2022_betan_para.bin", "wb");

  // Load parameters from runtime config
  double z_0 = config.chemistry.z_0;
  double AA = config.get_AA();
  double BB = config.get_BB();
  double D_a = config.get_D_a();
  double beta = config.chemistry.beta;
  double D_c = config.chemistry.D_c;
  int nz = config.grid.nz;
  double dz = config.grid.get_dz();
  int ny = config.grid.ny;
  double dy = config.grid.get_dy();
  int nx = config.grid.nx;
  double dx = config.grid.get_dx();
  int nt = config.time.nt;
  double dt = config.time.dt;
  double norm1 = config.get_norm1();
  double norm2 = config.get_norm2();

  double *advection = new double[nz];
  double *reaction = new double[nz];
  double *diffusion = new double[nz];
  int n;
  double Fx = dt / dx;
  double Fy = dt / dy;
  double Fz = dt / dz;
  double Cx = 1.0 / dx;
  double Cy = 1.0 / dy;
  double Cz = 1.0 / dz;
  double Fxx = dt / (dx * dx);
  double Fyy = dt / (dy * dy);
  double Fzz = dt / (dz * dz);
  double Cxx = dt / (dx * dx);
  double Cyy = dt / (dy * dy);
  double Czz = dt / (dz * dz);
  double u_x = 0.0, u_y = 0.0, u_z = 0.0, u_xx = 0.0, u_yy = 0.0, u_zz = 0.0;
  double c_x = 0.0, c_y = 0.0, c_z = 0.0, c_xx = 0.0, c_yy = 0.0, c_zz = 0.0;
  int count, bound;

  // Open MP to get execution time
  double itime, ftime, exec_time;
  itime = omp_get_wtime();

  // 3D array declaration - using linear arrays for zero-copy GPU transfers
  double *f = nullptr;
  double *f_n = nullptr;
  double *c = nullptr;
  double *c_n = nullptr;

  memory_allocation_cn(
    &c_n, &c, &f_n, &f,
    nx, ny, nz);
  // initialization DENSITY
  printf("%e\t%lf\t%e\n", BB, AA, D_a);
  initialization(f, c, nx, ny, nz, z_0, dx, dy, dz, norm1, norm2);

  // print the z-position t = 0
  print_initial_position(
    f, c,
    initialz, initialcz, initialx, initialcx, initialy, initialcy,
    dz, nx, ny, nz,
    25, 25, 800);  // Sample points: middle of grid in x,y and z=800

  // initialization for these functions
  initialization_fcts(advection, reaction, diffusion, nz, dz, AA, BB, D_a);

  printf("done with initialization\n");

  printf("start of the time loop\n");
  count = 1;
  bound = 0;
  char namepz[100];  // name for the file
  char namepx[100];  // name for the file
  char namecz[100];  // name for the file
  char namecx[100];  // name for the file
  int year = 0;

  while (bound < config.time.num_years * nt) {
    year += 50;
    printf("year %d\n", year);
    // open files
    FILE *fpz, *fpx, *fpy, *fcz, *fcx, *fcy;
    // beta positive
    /*
	  snprintf(namepz, sizeof(namepz), "./data/pz_100D_N100_"\
      "30_08_2022_betap_para_%d.dat", year);
    snprintf(namepx, sizeof(namepx), "./data/px_100D_N100_"\
      "30_08_2022_betap_para_%d.dat", year);
    snprintf(namecz, sizeof(namecz), "./data/cz_100D_N100_"\
      "30_08_2022_betap_para_%d.dat", year);
    snprintf(namecx, sizeof(namecx), "./data/cx_100D_N100_"\
      "30_08_2022_betap_para_%d.dat", year);
    */
    // beta negative - using binary format (.bin) for faster I/O and smaller files

    snprintf(namepz, sizeof(namepz), "../data/metal/pz_100D_N100_"\
      "30_08_2022_betan_para_%d.bin", year);
    snprintf(namepx, sizeof(namepx), "../data/metal/px_100D_N100_"\
      "30_08_2022_betan_para_%d.bin", year);
    char namepy[100];
    snprintf(namepy, sizeof(namepy), "../data/metal/py_100D_N100_"\
      "30_08_2022_betan_para_%d.bin", year);
    snprintf(namecz, sizeof(namecz), "../data/metal/cz_100D_N100_"\
      "30_08_2022_betan_para_%d.bin", year);
    snprintf(namecx, sizeof(namecx), "../data/metal/cx_100D_N100_"\
      "30_08_2022_betan_para_%d.bin", year);
    char namecy[100];
    snprintf(namecy, sizeof(namecy), "../data/metal/cy_100D_N100_"\
      "30_08_2022_betan_para_%d.bin", year);

    fpz = fopen(namepz, "wb");
    fpx = fopen(namepx, "wb");
    fpy = fopen(namepy, "wb");
    fcz = fopen(namecz, "wb");
    fcx = fopen(namecx, "wb");
    fcy = fopen(namecy, "wb");

    for (n = bound; n < count * nt; n++) {
      // old value to the new one
      oldtonew(c_n, f_n, c, f, nx, ny, nz);

      // update new value - concentration field
      // Use CPU version - simple operation, not worth GPU transfer overhead
      concentration_field(c_n, c, nx, ny, nz,
        c_xx, c_yy, c_zz, D_c, Cxx, Cyy, Czz);

#ifdef __APPLE__
      // Use Metal GPU accelerated version for density
      concentration_field_density_metal(
        c, f, f_n, advection, reaction,
        diffusion, nx, ny, nz, dt, beta,
        c_x, c_y, c_z, c_xx, c_yy, c_zz,
        u_x, u_y, u_z, u_xx, u_yy, u_zz,
        Cx, Cy, Cz, Cxx, Cyy, Czz, Fx, Fy,
        Fz, Fxx, Fyy, Fzz);
#else
      // Fallback to CPU version on non-Apple platforms
      concentration_field_density(
        c, f, f_n, advection, reaction,
        diffusion, nx, ny, nz, dt, beta,
        c_x, c_y, c_z, c_xx, c_yy, c_zz,
        u_x, u_y, u_z, u_xx, u_yy, u_zz,
        Cx, Cy, Cz, Cxx, Cyy, Czz, Fx, Fy,
        Fz, Fxx, Fyy, Fzz);
#endif
    }
    // print out the z and x components
    // Using improved version with configurable sample points
    print_position(
      f, c,
      fpz, fcz, fpx, fcx, fpy, fcy,
      dz, nx, ny, nz, 25, 25, 800);  // Fixed: was 2132 (out of bounds for nz=1600)

    fclose(fpz);
    fclose(fpx);
    fclose(fpy);
    fclose(fcz);
    fclose(fcx);
    fclose(fcy);
    bound += nt;
    count++;
  }

  printf("Simulation Done\n");

  // deallocate memory
  delocate_memory(
    f, f_n, c, c_n,
    nx, ny, nz);

  fclose(initialz);
  fclose(initialx);
  fclose(initialcz);
  fclose(initialcx);

  // time running code
  ftime = omp_get_wtime();
  exec_time = ftime - itime;
  printf("Time taken is %f", exec_time);

#ifdef __APPLE__
  // Cleanup Metal resources
  cleanup_metal();
#endif

  return 0;
}
