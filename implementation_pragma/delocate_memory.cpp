#include "headers/delocate_memory.h"

using namespace std;

void delocate_memory(
  double ***f, double ***f_n, double ***c, double ***c_n,
  int nx, int ny, int nz) {
    for (int i = 0; i < nx; i++) {
      for (int j = 0; j < ny; j++) {
        free(f[i][j]);
      }
      free(f[i]);
    }
    free(f);
    for (int i = 0; i < nx; i++) {
      for (int j = 0; j < ny; j++) {
        free(f_n[i][j]);
      }
      free(f_n[i]);
    }
    free(f_n);
    // CONCENTRATION
    for (int i = 0; i < nx; i++) {
      for (int j = 0; j < ny; j++) {
        free(c[i][j]);
      }
      free(c[i]);
    }
    free(c);
    for (int i = 0; i < nx; i++) {
      for (int j = 0; j < ny; j++) {
        free(c_n[i][j]);
      }
      free(c_n[i]);
    }
    free(c_n);
}
