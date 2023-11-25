#include "headers/initialization_fcts.h"

using namespace std;


void initialization_fcts(
    double *advection, double *reaction, double *diffusion, 
    int nz, 
    double dz, double AA, double BB, double D_a
    )
{
#pragma omp parallel for simd 
	for (int k = 0; k < nz; k++)
	{
		advection[k] = AA / ((k * dz) * (k * dz) * (k * dz)) - 6.0 * BB / ((k * dz) * (k * dz) * (k * dz) * (k * dz));
		reaction[k] = 3.0 * AA / ((k * dz) * (k * dz) * (k * dz) * (k * dz)) + 12.0 * BB / ((k * dz) * (k * dz) * (k * dz) * (k * dz) * (k * dz));
		diffusion[k] = D_a + BB / ((k * dz) * (k * dz) * (k * dz));
	}
	// For the first 10 positions (z = [0,10]), assagning the same value
	double aaa = 10.0 * 10.0 * 10.0;
	double aaaa = 10.0 * 10.0 * 10.0 * 10.0;
	for (int k = 0; k * dz < 10; k++)
	{
		advection[k] = AA / aaa - 6.0 * BB / aaaa;
		reaction[k] = 3.0 * AA / aaaa + 12.0 * BB / (aaaa * 20.0);
		diffusion[k] = D_a + BB / aaa;
	}
}