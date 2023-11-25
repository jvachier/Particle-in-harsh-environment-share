#include "headers/print_initial_position.h"

using namespace std;

void print_initial_position(
    double ***f, double ***c,
    FILE *initialz, FILE *initialcz, FILE *initialx, FILE *initialcx,
    double dz, int nx, int nz
)
{
    for (int k = 0; k < nz; k++)
	{
		fprintf(initialz, "%.7f\t%.7f\n ", k * dz, f[25][25][k]);  // nx = ny = 50
		fprintf(initialcz, "%.7f\t%.7f\n ", k * dz, c[25][25][k]); // nx = ny = 50
	}

	// print the x-position t = 0
	for (int i = 0; i < nx; i++)
	{
		fprintf(initialx, "%.7f\t%.7f\n ", i * dz, f[i][25][2132]); // ny = 50 nz = 1600
		fprintf(initialcx, "%.7f\t%.e\n ", i * dz, c[i][25][2132]); // ny = 50 nz = 1600
	}
}