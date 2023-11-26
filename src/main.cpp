/*
 * Author:	Jeremy Vachier
 * Purpose:	Solving Fokker-Planck equations describing an active particle in a harsh environment using finite difference method
 * Reference: 	Vachier J and Wettlaufer JS (2022) Biolocomotion and Premelting in Ice. Front. Phys. 10:904836.
 * Language: 	C++
 * Date: 	2021-2022
 */

#include <iostream>
#include <random>
#include <string>
#include <cmath>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

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

int main(int argc, char *argv[])
{

	FILE *initialz, *initialx, *initialcz, *initialcx;

    omp_set_num_threads(N_thread);

	// beta positive
	/*
	initialz	= fopen ("./data/pz_z_initial_100D_N100_0y_30_08_2022_betap_para.dat", "w");
	initialx	= fopen ("./data/px_x_initial_100D_N100_0y_30_08_2022_betap_para.dat", "w");
	initialcz	= fopen ("./data/c_cz_initial_100D_N100_0y_30_08_2022_betap_para.dat", "w");
	initialcx       = fopen ("./data/c_cx_initial_100D_N100_0y_30_08_2022_betap_para.dat", "w");
	*/

	// beta negative

	initialz = fopen("./data/pz_z_initial_100D_N100_0y_30_08_2022_betan_para.dat", "w");
	initialx = fopen("./data/px_x_initial_100D_N100_0y_30_08_2022_betan_para.dat", "w");
	initialcz = fopen("./data/c_cz_initial_100D_N100_0y_30_08_2022_betan_para.dat", "w");
	initialcx = fopen("./data/c_cx_initial_100D_N100_0y_30_08_2022_betan_para.dat", "w");

	// parameters
	double z_0 = 60.0; 
	double A_3 = rhosq_m * delaT * pow(R_g * T_m * N_i, 2) / (6.0 * nu * R * T_m);
	double A_2 = (rho_l * q_m * delaT) / T_m;
	double AA = A_3 / pow(A_2, 3);
	double BB = (pow(R_g * T_m * N_i, 3) / (8. * PI * nu * pow(R, 4) * pow(A_2, 3))) * kb * T_m;
	double D_a = 100.0 * BB;
	// chemotaxis
	double beta = -1e-10; 
	double D_c = 1e-10; 
	int nz = 1600; 
	double dz = 80.0 / nz; 
	int ny = 50;   // need to be equal to nx
	double dy = 10.0 / ny; // need to be equal to nx
	int nx = 50; 
	double dx = 10.0 / nx; 
	int nt = 31536 / 2;  // 50years with dt 1E5
	int dt = 1e5; 
	double norm1 = (PI * 7.926); 
	double norm2 = (pow(PI, 3 / 2)); 
	double advection[nz];
	double reaction[nz];
	double diffusion[nz];
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
	double u_x, u_y, u_z, u_xx, u_yy, u_zz;
	double c_x, c_y, c_z, c_xx, c_yy, c_zz;
	int count, bound;

    // Open MP to get execution time
	double itime, ftime, exec_time;
    itime = omp_get_wtime(); 

	// 3D array declaration
	// DENSITY ARRAY
	double ***f = (double ***)malloc(nx * sizeof(double **));
	double ***f_n = (double ***)malloc(nx * sizeof(double **));

	// CONCENTRATION ARRAY - use the same size than DENSITY ARRAY
	double ***c = (double ***)malloc(nx * sizeof(double **));
	double ***c_n = (double ***)malloc(nx * sizeof(double **));

    memory_allocation_cn(
        c_n, c, f_n, f,
        nx, ny, nz
    );

	// initialization DENSITY
	printf("%e\t%lf\t%e\n", BB, AA, D_a);
	initialization(f, c, nx, ny, nz, z_0, dx, dy, dz, norm1, norm2);

	// print the z-position t = 0
    print_initial_position(
        f, c,
        initialz, initialcz, initialx, initialcx,
        dz, nx, nz
    );

	// initialization for these functions
	initialization_fcts(advection, reaction, diffusion, nz, dz, AA, BB, D_a);

	printf("done with initialization\n");

	printf("start of the time loop\n");
	count = 1;
	bound = 0;
	char namepz[100]; // name for the file
	char namepx[100]; // name for the file
	char namecz[100]; // name for the file
	char namecx[100]; // name for the file
	int year = 0;
	while (bound < alpha * nt)
	{
		year += 50;
		printf("year %d\n", year);
		// open files
		FILE *fpz, *fpx, *fcz, *fcx;
		// beta positive
		/*
		sprintf(namepz, "./data/pz_100D_N100_30_08_2022_betap_para_%d.dat", year);
		sprintf(namepx, "./data/px_100D_N100_30_08_2022_betap_para_%d.dat", year);
		sprintf(namecz, "./data/cz_100D_N100_30_08_2022_betap_para_%d.dat", year);
		sprintf(namecx, "./data/cx_100D_N100_30_08_2022_betap_para_%d.dat", year);
		*/
		// beta negative

		sprintf(namepz, "./data/pz_100D_N100_30_08_2022_betan_para_%d.dat", year);
		sprintf(namepx, "./data/px_100D_N100_30_08_2022_betan_para_%d.dat", year);
		sprintf(namecz, "./data/cz_100D_N100_30_08_2022_betan_para_%d.dat", year);
		sprintf(namecx, "./data/cx_100D_N100_30_08_2022_betan_para_%d.dat", year);

		fpz = fopen(namepz, "w");
		fpx = fopen(namepx, "w");
		fcz = fopen(namecz, "w");
		fcx = fopen(namecx, "w");

		for (n = bound; n < count * nt; n++)
		{
			// old value to the new one
			oldtonew(c_n, f_n, c, f, nx, ny, nz);
			// update new value
			concentration_field(c_n, c, nx, ny, nz, c_xx, c_yy, c_zz, D_c, Cxx, Cyy, Czz);
			concentration_field_density(
                c, f, f_n, advection, reaction, 
                diffusion, nx, ny, nz, dt, beta, 
                c_x, c_y, c_z, c_xx, c_yy, c_zz, 
                u_x, u_y, u_z, u_xx, u_yy, u_zz, 
                Cx, Cy, Cz, Cxx, Cyy, Czz, Fx, Fy, 
                Fz, Fxx, Fyy, Fzz
            );
		}
		// print out the z and x components
        print_position(
            f, c,
            fpz, fcz, fpx, fcx,
            dz, nx, nz
        );

		fclose(fpz);
		fclose(fpx);
		fclose(fcz);
		fclose(fcx);
		bound += nt;
		count++;
	}

	printf("Simulation Done");

	// deallocate memory
    delocate_memory(
        f, f_n, c, c_n,
        nx, ny, nz
    );
	
	fclose(initialz);
	fclose(initialx);
	fclose(initialcz);
	fclose(initialcx);

	// time running code
    ftime = omp_get_wtime();
    exec_time = ftime - itime;
    printf("Time taken is %f", exec_time);

	return 0;
}
