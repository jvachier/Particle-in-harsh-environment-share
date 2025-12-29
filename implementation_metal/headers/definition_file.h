#define PI 3.14159265
#define rho_l 920
#define q_m 3.3e5
#define T_m 273.15
#define delaT 0.1
#define nu 1e-3
#define N_i 100.0e-6
#define rhosq_m 334.0e6
#define R_g 8.31
#define R 9.0e-6
#define kb 1.38e-23

#define N_thread 6

#define alpha 1 // number of years to loop over

// #define z_0 60.0


// #define A_3 rhosq_m * delaT * pow(R_g * T_m * N_i, 2) / (6.0 * nu * R * T_m)
// #define A_2 (rho_l * q_m * delaT) / T_m
// #define AA A_3 / pow(A_2, 3)
// #define BB (pow(R_g * T_m * N_i, 3) / (8. * PI * nu * pow(R, 4) * pow(A_2, 3))) * kb * T_m
// #define D_a 100.0 * BB
// // chemotaxis
// #define beta -1e-10
// #define D_c 1e-10
// #define nz 1600
// #define dz 80.0 / nz
// #define ny 50	   // need to be equal to nx
// #define dy 10.0 / ny // need to be equal to nx
// #define nx 50
// #define dx 10.0 / nx
// #define nt 31536 / 2// 50years with dt 1E5
// #define dt 1e5
// #define norm1 (PI * 7.926)
// #define norm2 (pow(PI, 3 / 2))



