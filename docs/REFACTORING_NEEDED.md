# Critical Refactoring Needed

## Issues Identified

### 1. Configuration Requires Recompilation ❌

**Problem**: Currently using `#define` macros in `config/simulation_config.h`:
```cpp
#define ALPHA 6  // Changing this requires recompilation!
```

**Solution**: Use runtime configuration file (JSON/INI) or command-line arguments

**Files to change**:
- Delete or deprecate `config/simulation_config.h`
- Use `config/simulation_parameters.h` (already exists but not used)
- Update `main.cpp` in both implementations to load config at runtime

**Benefit**: Change `num_years` from 1 to 6 without rebuilding!

---

### 2. Too Many Makefiles ❌

**Problem**: Currently have 4+ Makefiles:
- `./Makefile` (centralized - GOOD!)
- `implementation_pragma/Makefile` (duplicate)
- `implementation_metal/Makefile` (duplicate)
- `tests/Makefile`

**Solution**: Keep ONLY the root `./Makefile`, remove the others

**Implementation**:
```makefile
# Root Makefile contains ALL build rules
# Implementations are just source directories, not independent projects
```

**Benefit**: Single source of truth for build configuration

---

### 3. Data Directory Organization ✅ FIXED

Organized into:
```
data/
├── pragma/  # Pragma SIMD output
└── metal/   # Metal GPU output
```

---

## Recommended Next Steps

### Option A: Quick Fix (5 min)
Just update `ALPHA` in `simulation_config.h` and rebuild once

### Option B: Proper Fix (30-60 min)
1. Create `config.json` file with runtime parameters
2. Add JSON parsing to `main.cpp`
3. Remove duplicate Makefiles
4. Update all build rules in root Makefile
5. Test everything works

### Option C: Defer for Later
Document the issues and fix in next session

---

## Current Workaround

**To change simulation length RIGHT NOW**:

```bash
# Edit config/simulation_config.h
# Change: #define ALPHA 6
# To: #define ALPHA <YOUR_VALUE>

# Rebuild
make rebuild

# Run
make benchmark
```

Yes, this requires recompilation, but it works until we implement runtime config.

---

## What's Already Done ✅

1. ✅ Centralized Makefile created with all common targets
2. ✅ Data directories organized (pragma/metal)
3. ✅ Comprehensive visualization tools
4. ✅ Full mathematical documentation
5. ✅ Clean repository structure

---

## Priority

**HIGH**: Fix config system (enables parameter sweeps without rebuilding)
**MEDIUM**: Remove duplicate Makefiles (cleaner, less confusing)
**LOW**: Other improvements can wait

