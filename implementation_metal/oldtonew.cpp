#include "headers/oldtonew.h"
#include "headers/array_utils.h"
#include <cstring>  // for memcpy

using namespace std;

/**
 * Copy new values to old arrays for next iteration
 *
 * With linear arrays, this is just two memcpy calls!
 * Much simpler and faster than nested loops.
 *
 * @param c_n Output: old concentration array
 * @param f_n Output: old density array
 * @param c   Input: new concentration array
 * @param f   Input: new density array
 * @param nx  Grid size in x direction
 * @param ny  Grid size in y direction
 * @param nz  Grid size in z direction
 */
void oldtonew(
    double *c_n, double *f_n,
    const double *c, const double *f,
    int nx, int ny, int nz) {

    size_t total_size = ARRAY3D_SIZE(nx, ny, nz);
    size_t bytes = total_size * sizeof(double);

    // Copy entire arrays at once - extremely fast!
    memcpy(c_n, c, bytes);
    memcpy(f_n, f, bytes);
}
