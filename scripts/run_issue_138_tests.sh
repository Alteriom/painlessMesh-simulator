#!/bin/bash
# Run all Issue #138 validation scenarios
# Tests network partition recovery and bridge re-election behavior

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
SIMULATOR="${PROJECT_ROOT}/build/bin/painlessmesh-simulator"
SCENARIOS_DIR="${PROJECT_ROOT}/examples/scenarios"
RESULTS_DIR="${PROJECT_ROOT}/results/issue_138"
LOG_LEVEL="${LOG_LEVEL:-INFO}"

# Scenario list
SCENARIOS=(
  "issue_138_rapid_partition_cycles.yaml"
  "issue_138_message_routing.yaml"
  "issue_138_uneven_partitions.yaml"
  "issue_138_stress_partition.yaml"
  "issue_138_cascade_healing.yaml"
)

# Check if simulator exists
if [ ! -f "$SIMULATOR" ]; then
  echo -e "${RED}Error: Simulator not found at $SIMULATOR${NC}"
  echo "Please build the project first:"
  echo "  mkdir -p build && cd build"
  echo "  cmake -G Ninja .."
  echo "  ninja"
  exit 1
fi

# Create results directory
mkdir -p "$RESULTS_DIR"

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}Issue #138 Validation Test Suite${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""
echo "Simulator: $SIMULATOR"
echo "Scenarios: ${#SCENARIOS[@]}"
echo "Results: $RESULTS_DIR"
echo "Log Level: $LOG_LEVEL"
echo ""

# Track results
PASSED=0
FAILED=0
TOTAL=${#SCENARIOS[@]}

# Run each scenario
for scenario in "${SCENARIOS[@]}"; do
  scenario_path="${SCENARIOS_DIR}/${scenario}"
  scenario_name="${scenario%.yaml}"
  
  echo -e "${BLUE}========================================${NC}"
  echo -e "${BLUE}Running: ${scenario}${NC}"
  echo -e "${BLUE}========================================${NC}"
  
  if [ ! -f "$scenario_path" ]; then
    echo -e "${RED}✗ SKIP: Scenario file not found${NC}"
    ((FAILED++))
    echo ""
    continue
  fi
  
  # Run the simulation
  start_time=$(date +%s)
  
  if "$SIMULATOR" \
    --config "$scenario_path" \
    --log-level "$LOG_LEVEL" \
    --output "$RESULTS_DIR/${scenario_name}/" \
    > "$RESULTS_DIR/${scenario_name}.log" 2>&1; then
    
    end_time=$(date +%s)
    duration=$((end_time - start_time))
    
    echo -e "${GREEN}✓ PASSED${NC} (${duration}s)"
    ((PASSED++))
    
    # Check if results CSV exists
    if [ -f "$RESULTS_DIR/${scenario_name}/metrics.csv" ]; then
      lines=$(wc -l < "$RESULTS_DIR/${scenario_name}/metrics.csv")
      echo "  Metrics: ${lines} samples collected"
    fi
  else
    end_time=$(date +%s)
    duration=$((end_time - start_time))
    
    echo -e "${RED}✗ FAILED${NC} (${duration}s)"
    echo "  Check log: $RESULTS_DIR/${scenario_name}.log"
    ((FAILED++))
    
    # Show last 10 lines of error
    echo -e "${YELLOW}Last 10 lines of output:${NC}"
    tail -n 10 "$RESULTS_DIR/${scenario_name}.log" | sed 's/^/  /'
  fi
  
  echo ""
done

# Summary
echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}Test Summary${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""
echo "Total Scenarios: $TOTAL"
echo -e "${GREEN}Passed: $PASSED${NC}"
echo -e "${RED}Failed: $FAILED${NC}"
echo ""

if [ $FAILED -eq 0 ]; then
  echo -e "${GREEN}✓ All issue #138 validation tests PASSED${NC}"
  echo ""
  echo "Results suggest issue #138 is NOT present (or is fixed)."
  echo ""
  exit 0
else
  echo -e "${RED}✗ Some tests FAILED${NC}"
  echo ""
  echo "Results suggest issue #138 MAY be present."
  echo "Review the logs in: $RESULTS_DIR"
  echo ""
  echo "Failed scenarios:"
  for scenario in "${SCENARIOS[@]}"; do
    scenario_name="${scenario%.yaml}"
    if [ ! -f "$RESULTS_DIR/${scenario_name}/metrics.csv" ]; then
      echo "  - ${scenario}"
    fi
  done
  echo ""
  exit 1
fi
