#!/bin/bash
set -e

DATA="${1:-data/odid1.csv}"   # vstupny dataset (11 stlpcov = flow CSV, 3/5 = simple tuple)

echo "building"
make

# format zistim z poctu stlpcov hlavicky - rovnako ako to robi benchmark
NCOLS=$(awk -F, 'NR==1{print NF; exit}' "$DATA")
echo "dataset $DATA ma $NCOLS stlpcov"

rm -f results/run_sym*.csv    # stare behy nech sa nepomiesaju do glob-u

for sym in 0 1 2; do
    if [ "$NCOLS" -eq 11 ]; then
        for w in flow packet byte; do
            echo "sym=$sym weighting=$w"
            ./benchmark "$DATA" "results/run_sym${sym}_${w}.csv" "$sym" 0 "$w"
        done
    else
        echo "sym=$sym (simple tuple)"
        ./benchmark "$DATA" "results/run_sym${sym}_flow.csv" "$sym" 0
    fi
done

make clean || true

echo "running python program"
source venv/bin/activate

python analyze.py results/ results/
