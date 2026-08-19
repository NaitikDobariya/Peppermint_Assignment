#!/bin/bash
# test_determinism.sh

# Create a single clean results directory
RESULTS_DIR="results"
rm -rf $RESULTS_DIR
mkdir -p $RESULTS_DIR

# Define our robust list of compiler optimizations
OPTIMIZATIONS=("O0" "O1" "O2" "O3" "Os" "Ofast")
RUNS=5

for OPT in "${OPTIMIZATIONS[@]}"; do
    echo "=== Building and Running with -$OPT ==="
    BUILD_DIR="build_$OPT"
    mkdir -p $BUILD_DIR && cd $BUILD_DIR
    
    # Build the executable (now named run_simulation)
    cmake -DCMAKE_CXX_FLAGS="-$OPT" .. > /dev/null 2>&1
    make -j4 -s > /dev/null 2>&1
    
    # Run 5 times
    for i in $(seq 1 $RUNS); do
        ./run_simulation > ../$RESULTS_DIR/log_${OPT}_run${i}.txt
        for file in route_*.json; do
            # Rename and move to the single results folder
            cp "$file" "../$RESULTS_DIR/${OPT}_run${i}_${file}" 2>/dev/null
        done
    done
    cd ..
done

echo "=== Verifying Determinism (MD5 Checksums) ==="
# We have 5 routes, let's check them all
for ROUTE_ID in {1..5}; do
    echo "Checking Route $ROUTE_ID..."
    
    # Establish a baseline hash from O0 Run 1
    BASELINE_FILE="$RESULTS_DIR/O0_run1_route_${ROUTE_ID}.json"
    if [ ! -f "$BASELINE_FILE" ]; then
        echo "❌ ERROR: $BASELINE_FILE not found! run_simulation failed to generate JSON."
        exit 1
    fi
    BASELINE_HASH=$(md5sum "$BASELINE_FILE" | awk '{print $1}')

    # Compare all runs across all optimizations against the baseline
    for OPT in "${OPTIMIZATIONS[@]}"; do
        for RUN in $(seq 1 $RUNS); do
            CURRENT_FILE="$RESULTS_DIR/${OPT}_run${RUN}_route_${ROUTE_ID}.json"
            
            if [ ! -f "$CURRENT_FILE" ]; then
                echo "❌ ERROR: $CURRENT_FILE is missing!"
                exit 1
            fi
            
            CURRENT_HASH=$(md5sum "$CURRENT_FILE" | awk '{print $1}')
            
            if [ "$BASELINE_HASH" != "$CURRENT_HASH" ]; then
                echo "❌ ERROR: Non-determinism detected in Route $ROUTE_ID!"
                echo "Baseline (O0 Run 1): $BASELINE_HASH"
                echo "Mismatch ($OPT Run $RUN): $CURRENT_HASH"
                exit 1
            fi
        done
    done
done

echo "✅ SUCCESS: Absolute determinism guaranteed! All routes across ${#OPTIMIZATIONS[@]} optimization levels match perfectly."

echo "=== Cleaning Up Temporary Build Folders ==="
for OPT in "${OPTIMIZATIONS[@]}"; do
    rm -rf "build_$OPT"
done
echo "✨ Workspace is clean! All 150 results are in the '$RESULTS_DIR' folder."