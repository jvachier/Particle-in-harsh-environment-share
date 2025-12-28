# Mathematical Formulation

This document describes the mathematical equations implemented in the particle simulation code.

## Overview

The simulation models the interaction between **ice crystals** (particle density) and **water vapor** (concentration field) in harsh environmental conditions using a coupled system of partial differential equations (PDEs).

## System of PDEs

The system consists of two coupled equations:

1. **Concentration Field Equation** - Diffusion of water vapor
2. **Density Field Equation** - Fokker-Planck equation with chemotaxis, advection, reaction, and diffusion

---

## 1. Concentration Field Equation

The concentration field `c(x,y,z,t)` represents the water vapor density and evolves according to a **pure diffusion equation**:

```
∂c/∂t = D_c ∇²c
```

### Expanded Form

```
∂c/∂t = D_c (∂²c/∂x² + ∂²c/∂y² + ∂²c/∂z²)
```

### Discrete Form

Using finite differences on a 3D grid with spacing `(dx, dy, dz)`:

```
c[i,j,k]^(n+1) = c[i,j,k]^n + D_c Δt [
    (c[i-1,j,k]^n - 2c[i,j,k]^n + c[i+1,j,k]^n) / dx²  +
    (c[i,j-1,k]^n - 2c[i,j,k]^n + c[i,j+1,k]^n) / dy²  +
    (c[i,j,k-1]^n - 2c[i,j,k]^n + c[i,j,k+1]^n) / dz²
]
```

### Parameters

- `D_c = 1×10⁻¹⁰` - Diffusion coefficient for concentration field
- `dx = X_LENGTH/NX` - Grid spacing in x-direction
- `dy = Y_LENGTH/NY` - Grid spacing in y-direction
- `dz = Z_LENGTH/NZ` - Grid spacing in z-direction

### Implementation

See [concentration_field.cpp](../implementation_metal/concentration_field.cpp) and [concentration_field.metal](../implementation_metal/shaders/concentration_field.metal)

---

## 2. Density Field Equation (Fokker-Planck with Chemotaxis)

The density field `f(x,y,z,t)` represents the ice crystal population density and evolves according to a **Fokker-Planck equation** with four physical processes:

```
∂f/∂t = Chemotaxis + Advection + Reaction + Diffusion
```

### Full Equation

```
∂f/∂t = -∇·(β f ∇c) + ∇·(v f) - λ(z) f + ∇·(D(z) ∇f)
```

where:
- `β` - Chemotaxis coefficient (attraction/repulsion to water vapor)
- `v` - Advection velocity field
- `λ(z)` - Depth-dependent reaction rate
- `D(z)` - Depth-dependent diffusion coefficient

### Term-by-Term Breakdown

#### 2.1 Chemotaxis Term

Models the movement of ice crystals towards (β < 0) or away from (β > 0) regions of high water vapor concentration:

```
Chemotaxis = -∇·(β f ∇c)
           = -β [∇f · ∇c + f ∇²c]
```

Expanded in 3D:

```
Chemotaxis = -β [
    (∂f/∂x)(∂c/∂x) + (∂f/∂y)(∂c/∂y) + (∂f/∂z)(∂c/∂z) +
    f(∂²c/∂x² + ∂²c/∂y² + ∂²c/∂z²)
]
```

#### 2.2 Advection Term

Models transport by environmental flow (e.g., settling under gravity):

```
Advection = ∇·(v f) = v_z ∂f/∂z
```

(assuming flow only in z-direction with velocity `v_z = advection[k]`)

#### 2.3 Reaction Term

Models particle creation/destruction (sublimation, aggregation):

```
Reaction = -λ(z) f = -reaction[k] f
```

Note: Implemented as `-Δt · reaction[k] · f` in discrete form

#### 2.4 Diffusion Term

Models random motion of particles:

```
Diffusion = ∇·(D(z) ∇f) = D(z) ∇²f
           = D(z) (∂²f/∂x² + ∂²f/∂y² + ∂²f/∂z²)
```

### Discrete Form

Combining all terms with finite differences:

```
f[i,j,k]^(n+1) = f[i,j,k]^n + Δt [Chemotaxis + Advection + Reaction + Diffusion]
```

#### Discrete Chemotaxis

```
Chemotaxis = β [
    F_x · (∂f/∂x) · C_x · (∂c/∂x) +
    F_y · (∂f/∂y) · C_y · (∂c/∂y) +
    F_z · (∂f/∂z) · C_z · (∂c/∂z) +
    f · (C_xx · ∂²c/∂x² + C_yy · ∂²c/∂y² + C_zz · ∂²c/∂z²)
]
```

where:
- `∂f/∂x = f[i+1,j,k] - f[i,j,k]` (forward difference)
- `∂c/∂x = c[i+1,j,k] - c[i,j,k]` (forward difference)
- `∂²c/∂x² = c[i-1,j,k] - 2c[i,j,k] + c[i+1,j,k]` (central difference)

Coefficients:
- `C_x, C_y, C_z = 1/dx, 1/dy, 1/dz` (first derivative coefficients)
- `C_xx, C_yy, C_zz = 1/dx², 1/dy², 1/dz²` (second derivative coefficients)
- `F_x, F_y, F_z = 1/dx, 1/dy, 1/dz` (density gradient coefficients)
- `F_xx, F_yy, F_zz = 1/dx², 1/dy², 1/dz²` (density Laplacian coefficients)

#### Discrete Advection

```
Advection = F_z · advection[k] · (f[i,j,k+1] - f[i,j,k])
```

#### Discrete Reaction

```
Reaction = -Δt · reaction[k] · f[i,j,k]
```

#### Discrete Diffusion

```
Diffusion = diffusion[k] · [
    F_xx · (f[i-1,j,k] - 2f[i,j,k] + f[i+1,j,k]) +
    F_yy · (f[i,j-1,k] - 2f[i,j,k] + f[i,j+1,k]) +
    F_zz · (f[i,j,k-1] - 2f[i,j,k] + f[i,j,k+1])
]
```

### Parameters

- `β = -1×10⁻¹⁰` - Chemotaxis coefficient (negative = attraction to water vapor)
- `advection[k]` - Depth-dependent advection coefficient (1D array, size `nz`)
- `reaction[k]` - Depth-dependent reaction rate (1D array, size `nz`)
- `diffusion[k]` - Depth-dependent diffusion coefficient (1D array, size `nz`)
- `Δt = 1×10⁵` seconds - Time step size (~27.8 hours)

### Implementation

See [concentration_field_density.cpp](../implementation_metal/concentration_field_density.cpp) and [concentration_field_density_metal.mm](../implementation_metal/concentration_field_density_metal.mm)

---

## 3. Physical Constants

From `config/simulation_config.h`:

| Symbol | Value | Units | Description |
|--------|-------|-------|-------------|
| `ρ_l` | 920 | kg/m³ | Density of liquid water |
| `q_m` | 3.3×10⁵ | J/kg | Latent heat of melting |
| `T_m` | 273.15 | K | Melting temperature |
| `ΔT` | 0.1 | K | Temperature difference |
| `ν` | 1×10⁻³ | Pa·s | Dynamic viscosity |
| `N_i` | 1×10⁻⁴ | mol/m³ | Initial molar concentration |
| `ρ²_m` | 3.34×10⁸ | kg²/m⁶ | Density squared parameter |
| `R_g` | 8.31 | J/(mol·K) | Universal gas constant |
| `R` | 9×10⁻⁶ | m | Particle radius |
| `k_b` | 1.38×10⁻²³ | J/K | Boltzmann constant |

### Derived Parameters

```
A_3 = (ρ²_m · ΔT · (R_g · T_m · N_i)²) / (6 · ν · R · T_m)

A_2 = (ρ_l · q_m · ΔT) / T_m

AA = A_3 / A_2³

BB = ((R_g · T_m · N_i)³ / (8π · ν · R⁴ · A_2³)) · k_b · T_m

D_a = 100 · BB
```

---

## 4. Computational Domain

### Grid Specification

- **Spatial Domain**: `[0, X_LENGTH] × [0, Y_LENGTH] × [0, Z_LENGTH]`
  - X: 10.0 units (50 grid points)
  - Y: 10.0 units (50 grid points)
  - Z: 80.0 units (1600 grid points)

- **Grid Spacing**:
  - `dx = 0.2` units
  - `dy = 0.2` units
  - `dz = 0.05` units

### Time Integration

- **Time step**: `Δt = 1×10⁵` seconds (~1.157 days)
- **Steps per year**: `nt = 15,768` (simulates 50 years per output cycle)
- **Total simulation**: `α = 1` year

### Boundary Conditions

Both fields use **Neumann boundary conditions** (zero flux) at all boundaries:

```
∂c/∂n = 0  on ∂Ω
∂f/∂n = 0  on ∂Ω
```

Implemented by not updating boundary cells (loop from `i=1` to `nx-2`, etc.)

---

## 5. Initial Conditions

### Concentration Field

```
c(x,y,z,0) = exp(-((z - z₀)²) / 20) / (√(2π) · norm)
```

where:
- `z₀ = 60.0` - Initial peak position
- `norm = √(π) · 7.926` - Normalization constant

### Density Field

```
f(x,y,z,0) = exp(-((z - z₀)²) / 20) · exp(-(x² + y²) / 10) / norm
```

where:
- `norm = π^(3/2)` - 3D normalization constant

---

## 6. Numerical Method

### Scheme
- **Explicit finite differences** for time integration
- **Central differences** for spatial derivatives (second-order accurate)
- **Forward differences** for chemotaxis gradients

### Stability
The explicit scheme requires time step constraints:

```
Δt ≤ min(dx², dy², dz²) / (2 · D_max)
```

where `D_max = max(D_c, max(diffusion[k]))`.

Current parameters satisfy this constraint:
```
dz² / (2 · D_c) = (0.05)² / (2 · 1×10⁻¹⁰) = 1.25×10⁷ seconds >> Δt = 1×10⁵ seconds
```

---

## 7. GPU Implementation Details

The Metal GPU implementation uses **single-precision (float32)** instead of double-precision due to GPU hardware constraints. Conversions occur at CPU↔GPU boundaries:

```cpp
// CPU→GPU: double→float conversion
for (size_t i = 0; i < totalSize; i++) {
    c_gpu[i] = (float)c[i];
}

// GPU→CPU: float→double conversion
for (size_t i = 0; i < totalSize; i++) {
    f[i] = (double)f_result_gpu[i];
}
```

The linear array refactoring enables **zero-copy transfers** - no intermediate array conversions needed, just precision changes.

---

## 8. Performance Optimizations

### CPU (Pragma SIMD)
- OpenMP parallel loops with `#pragma omp parallel for collapse(2)`
- SIMD vectorization on innermost k-loop: `#pragma omp simd`
- Optimal loop ordering: `i→j→k` for memory locality

### GPU (Metal)
- 3D thread dispatch: `(nx-2) × (ny-2) × (nz-2)` threads
- Thread groups: `8 × 8 × 4` for optimal occupancy
- Buffer caching: GPU buffers allocated once and reused across all time steps
- Zero-copy transfers: Linear arrays enable direct CPU↔GPU data sharing

---

## References

This simulation implements a **chemotaxis model** for particle dynamics in environmental flows, based on Fokker-Planck equations with spatially-varying coefficients.

For implementation details, see:
- [IMPLEMENTATION_SUMMARY.md](IMPLEMENTATION_SUMMARY.md)
- [LINEAR_ARRAY_REFACTOR_SUCCESS.md](LINEAR_ARRAY_REFACTOR_SUCCESS.md)
- [GPU_OPTIMIZATION_ANALYSIS.md](GPU_OPTIMIZATION_ANALYSIS.md)
