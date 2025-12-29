#!/bin/bash
# Performance comparison: Pragma SIMD vs Metal GPU
# Usage: ./scripts/compare_performance.sh

set -e  # Exit on error

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"
cd "$ROOT_DIR"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo ""
echo "════════════════════════════════════════════════════════"
echo "  Performance Comparison: Pragma SIMD vs Metal GPU"
echo "════════════════════════════════════════════════════════"
echo ""

# Check for macOS
if [[ "$OSTYPE" != "darwin"* ]]; then
    echo -e "${RED}❌ Error: Metal GPU requires macOS${NC}"
    echo "This script can only run on macOS with Metal support"
    exit 1
fi

# Check if binaries exist
if [ ! -f "implementation_pragma/main_pragma.out" ]; then
    echo -e "${YELLOW}⚠ Pragma binary not found. Building...${NC}"
    make pragma
fi

if [ ! -f "implementation_metal/main_metal.out" ]; then
    echo -e "${YELLOW}⚠ Metal binary not found. Building...${NC}"
    make metal
fi

# Create data directories for each implementation
mkdir -p data/pragma data/metal

# Clean old data files
echo -e "${BLUE}Cleaning old data files...${NC}"
rm -f data/pragma/*.bin data/pragma/*.dat
rm -f data/metal/*.bin data/metal/*.dat
rm -f pragma_timing.log metal_timing.log

echo ""
echo "════════════════════════════════════════════════════════"
echo "  Test Configuration"
echo "════════════════════════════════════════════════════════"
echo ""

# Extract configuration from header (if available)
if [ -f "config/simulation_parameters.h" ]; then
    NX=$(grep "int nx =" config/simulation_parameters.h | awk '{print $4}' | tr -d ';')
    NY=$(grep "int ny =" config/simulation_parameters.h | awk '{print $4}' | tr -d ';')
    NZ=$(grep "int nz =" config/simulation_parameters.h | awk '{print $4}' | tr -d ';')
    NT=$(grep "int nt =" config/simulation_parameters.h | awk '{print $4}' | tr -d ';')
    NUM_YEARS=$(grep "int num_years =" config/simulation_parameters.h | awk '{print $4}' | tr -d ';')
    DT=$(grep "double dt =" config/simulation_parameters.h | awk '{print $4}' | tr -d ';')

    # Calculate years per iteration
    YEARS_PER_ITER=$(echo "scale=1; ($NT * $DT) / (365.25 * 24 * 3600)" | bc)
    TOTAL_YEARS=$(echo "scale=1; $YEARS_PER_ITER * $NUM_YEARS" | bc)

    echo "Grid size: ${NX}×${NY}×${NZ}"
    echo "Time steps per iteration: ${NT}"
    echo "Number of iterations: ${NUM_YEARS}"
    echo "Years per iteration: ${YEARS_PER_ITER}"
    echo "Total years simulated: ${TOTAL_YEARS}"
    echo ""
fi

# System information
echo "System Information:"
sysctl -n machdep.cpu.brand_string
echo "CPU cores: $(sysctl -n hw.ncpu)"
echo "Physical memory: $(( $(sysctl -n hw.memsize) / 1024 / 1024 / 1024 )) GB"
echo ""

# Test 1: Pragma SIMD
echo "════════════════════════════════════════════════════════"
echo "  [1/2] Running Pragma SIMD Implementation"
echo "════════════════════════════════════════════════════════"
echo ""
echo -e "${BLUE}Starting pragma SIMD test...${NC}"
echo "Output: pragma_timing.log"
echo ""

cd implementation_pragma
/usr/bin/time -l ./main_pragma.out 2>&1 | tee ../pragma_timing.log
PRAGMA_EXIT=${PIPESTATUS[0]}
cd ..

if [ $PRAGMA_EXIT -ne 0 ]; then
    echo -e "${RED}❌ Pragma SIMD test failed!${NC}"
    exit 1
fi

echo ""
echo -e "${GREEN}✓ Pragma SIMD test complete${NC}"
echo ""

# Clean data for next test
rm -f data/*.bin data/*.dat

# Test 2: Metal GPU
echo "════════════════════════════════════════════════════════"
echo "  [2/2] Running Metal GPU Implementation"
echo "════════════════════════════════════════════════════════"
echo ""
echo -e "${BLUE}Starting Metal GPU test...${NC}"
echo "Output: metal_timing.log"
echo ""

cd implementation_metal
/usr/bin/time -l ./main_metal.out 2>&1 | tee ../metal_timing.log
METAL_EXIT=${PIPESTATUS[0]}
cd ..

if [ $METAL_EXIT -ne 0 ]; then
    echo -e "${RED}❌ Metal GPU test failed!${NC}"
    exit 1
fi

echo ""
echo -e "${GREEN}✓ Metal GPU test complete${NC}"
echo ""

# Parse timing results
echo "════════════════════════════════════════════════════════"
echo "  Performance Results"
echo "════════════════════════════════════════════════════════"
echo ""

# Extract timing data (format: "Time taken is X.XX      X.XX real  X.XX user  X.XX sys")
PRAGMA_REAL=$(grep "real.*user.*sys" pragma_timing.log | awk '{for(i=1;i<=NF;i++) if($(i+1)=="real") print $i}')
PRAGMA_USER=$(grep "real.*user.*sys" pragma_timing.log | awk '{for(i=1;i<=NF;i++) if($(i+1)=="user") print $i}')
PRAGMA_SYS=$(grep "real.*user.*sys" pragma_timing.log | awk '{for(i=1;i<=NF;i++) if($(i+1)=="sys") print $i}')

METAL_REAL=$(grep "real.*user.*sys" metal_timing.log | awk '{for(i=1;i<=NF;i++) if($(i+1)=="real") print $i}')
METAL_USER=$(grep "real.*user.*sys" metal_timing.log | awk '{for(i=1;i<=NF;i++) if($(i+1)=="user") print $i}')
METAL_SYS=$(grep "real.*user.*sys" metal_timing.log | awk '{for(i=1;i<=NF;i++) if($(i+1)=="sys") print $i}')

# Memory usage (in MB)
PRAGMA_MEM=$(grep "maximum resident set size" pragma_timing.log | awk '{printf "%.1f", $1/1024/1024}')
METAL_MEM=$(grep "maximum resident set size" metal_timing.log | awk '{printf "%.1f", $1/1024/1024}')

echo "┌─────────────────────┬──────────────┬──────────────┐"
echo "│ Metric              │ Pragma SIMD  │ Metal GPU    │"
echo "├─────────────────────┼──────────────┼──────────────┤"
printf "│ %-19s │ %11.2fs │ %11.2fs │\n" "Real time" $PRAGMA_REAL $METAL_REAL
printf "│ %-19s │ %11.2fs │ %11.2fs │\n" "User time" $PRAGMA_USER $METAL_USER
printf "│ %-19s │ %11.2fs │ %11.2fs │\n" "System time" $PRAGMA_SYS $METAL_SYS
printf "│ %-19s │ %9.1f MB │ %9.1f MB │\n" "Peak memory" $PRAGMA_MEM $METAL_MEM
echo "└─────────────────────┴──────────────┴──────────────┘"
echo ""

# Calculate speedup
SPEEDUP=$(echo "scale=2; $PRAGMA_REAL / $METAL_REAL" | bc)
TIME_SAVED=$(echo "scale=1; $PRAGMA_REAL - $METAL_REAL" | bc)

echo "Performance Gain:"
echo -e "  ${GREEN}✓ Speedup: ${SPEEDUP}x${NC}"
echo "  ✓ Time saved: ${TIME_SAVED}s ($(echo "scale=1; $TIME_SAVED / 60" | bc) minutes)"
echo ""

# Check if Metal GPU was actually used
if grep -q "Metal GPU initialized successfully" metal_timing.log; then
    echo -e "${GREEN}✓ Metal GPU was successfully used${NC}"
else
    echo -e "${YELLOW}⚠ Warning: Metal GPU may have fallen back to CPU${NC}"
    echo "  Check metal_timing.log for details"
fi

echo ""
echo "════════════════════════════════════════════════════════"
echo "  Detailed Logs"
echo "════════════════════════════════════════════════════════"
echo ""
echo "Full timing logs saved to:"
echo "  • pragma_timing.log  (Pragma SIMD implementation)"
echo "  • metal_timing.log   (Metal GPU implementation)"
echo ""
echo "To view details:"
echo "  cat pragma_timing.log"
echo "  cat metal_timing.log"
echo ""
echo "To visualize output data:"
echo "  cd scripts"
echo "  # Pragma SIMD results:"
echo "  uv run python visualize_simulation.py ../data/pragma/*.bin"
echo "  # Metal GPU results:"
echo "  uv run python visualize_simulation.py ../data/metal/*.bin"
echo "  # Or just use default (metal):"
echo "  uv run python visualize_simulation.py"
echo ""

# Create summary file
cat > performance_summary.txt <<EOF
Performance Comparison Summary
==============================
Date: $(date)
Platform: macOS $(sw_vers -productVersion)
CPU: $(sysctl -n machdep.cpu.brand_string)

Configuration:
- Grid size: ${NX:-50}×${NY:-50}×${NZ:-1600}
- Time steps per iteration: ${NT:-15768}
- Number of iterations: ${NUM_YEARS:-12}
- Years per iteration: ${YEARS_PER_ITER:-50.0}
- Total years simulated: ${TOTAL_YEARS:-600.0}

Results:
- Pragma SIMD: ${PRAGMA_REAL}s (${PRAGMA_MEM} MB peak memory)
- Metal GPU:   ${METAL_REAL}s (${METAL_MEM} MB peak memory)
- Speedup:     ${SPEEDUP}x
- Time saved:  ${TIME_SAVED}s

Metal GPU Status: $(grep -q "Metal GPU initialized successfully" metal_timing.log && echo "Working" || echo "Check logs")
EOF

echo -e "${GREEN}✓ Summary saved to: performance_summary.txt${NC}"
echo ""
