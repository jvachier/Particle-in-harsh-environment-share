#include "headers/oldtonew.h"

using namespace std;

void oldtonew(double ***c_n, double ***f_n, double ***c, double ***f, int nx, int ny, int nz)
{
#pragma omp parallel for simd collapse(3)
	for (int i = 0; i < nx; i++)
	{
		for (int j = 0; j < ny; j++)
		{
			for (int k = 0; k < nz; k++)
			{
				c_n[i][j][k] = c[i][j][k];
				f_n[i][j][k] = f[i][j][k];
			}
		}
	}
}