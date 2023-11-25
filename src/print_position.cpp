#include "headers/print_position.h"

using namespace std;

void print_position(
    double ***f, double ***c,
    FILE *fpz, FILE *fcz, FILE *fpx, FILE *fcx,
    double dz, int nx, int nz
)
{
    for (int k = 0; k < nz; k++)
    {
        fprintf(fpz, "%lf\t%lf\n ", k * dz, f[25][25][k]); // nx = ny = 50
        fprintf(fcz, "%lf\t%lf\n ", k * dz, c[25][25][k]); // nx = ny = 50
    }
    for (int i = 0; i < nx; i++)
    {
        fprintf(fpx, "%lf\t%lf\n ", i * dz, f[i][25][2132]); // nx = 50 nz = 1600
        fprintf(fcx, "%lf\t%e\n ", i * dz, c[i][25][2132]);	 // nx = 50 nz = 1600
    }
}