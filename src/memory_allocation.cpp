#include "headers/memory_allocation.h"

using namespace std;

void memory_allocation_cn(
  double ***c_n, double ***c, double ***f_n, double ***f,
  int nx, int ny, int nz) {
  // f
  if (f == NULL) {
    fprintf(stderr, "Out of memory");
    exit(0);
  }
  for (int i = 0; i < nx; i++) {
    f[i] = reinterpret_cast<double**>(malloc(nx * sizeof(double *)));
    if (f[i] == NULL) {
      fprintf(stderr, "Out of memory");
      exit(0);
    }
    for (int j = 0; j < ny; j++) {
      f[i][j] = reinterpret_cast<double*>(malloc(nz * sizeof(double)));
      if (f[i][j] == NULL) {
        fprintf(stderr, "Out of memory");
        exit(0);
      }
    }
  }

  // f_n
  if (f_n == NULL) {
    fprintf(stderr, "Out of memory");
    exit(0);
  }
  for (int i = 0; i < nx; i++) {
    f_n[i] = reinterpret_cast<double**>(malloc(nx * sizeof(double *)));
    if (f_n[i] == NULL) {
      fprintf(stderr, "Out of memory");
      exit(0);
    }
    for (int j = 0; j < ny; j++) {
      f_n[i][j] = reinterpret_cast<double*>(malloc(nz * sizeof(double)));
      if (f_n[i][j] == NULL) {
        fprintf(stderr, "Out of memory");
        exit(0);
      }
    }
  }

  // c
  if (c == NULL) {
    fprintf(stderr, "Out of memory");
    exit(0);
  }
  for (int i = 0; i < nx; i++) {
    c[i] = reinterpret_cast<double**>(malloc(nx * sizeof(double *)));
    if (c[i] == NULL) {
      fprintf(stderr, "Out of memory");
      exit(0);
    }
    for (int j = 0; j < ny; j++) {
      c[i][j] = reinterpret_cast<double*>(malloc(nz * sizeof(double)));
      if (c[i][j] == NULL) {
        fprintf(stderr, "Out of memory");
        exit(0);
      }
    }
  }

  // c_n
  if (c_n == NULL) {
    fprintf(stderr, "Out of memory");
    exit(0);
  }
  for (int i = 0; i < nx; i++) {
    c_n[i] = reinterpret_cast<double**>(malloc(nx * sizeof(double *)));
    if (c_n[i] == NULL) {
      fprintf(stderr, "Out of memory");
      exit(0);
    }
    for (int j = 0; j < ny; j++) {
       c_n[i][j] = reinterpret_cast<double*>(malloc(nz * sizeof(double)));
       if (c_n[i][j] == NULL) {
         fprintf(stderr, "Out of memory");
         exit(0);
       }
    }
  }
}
