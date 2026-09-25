#!/bin/bash
set -e

DATA="${1:-data/odid1.csv}"   # vstupny dataset (11 stlpcov = flow CSV, 3/5 = simple tuple)
OUT="${2:-results}"           # kam sa pisu run_sym*.csv a grafy (RETA beh daj inam, nech neprepise stary)

# vsetky behy su na sebe nezavisle, tak ich pustam naraz. na 10 jadrach je 9 uloh
# akurat - kazda si cita dataset zvlast, ale citanie je pod 1 % casu (95 % su hashe).
JOBS="${JOBS:-9}"

echo "building"
make

# format zistim z poctu stlpcov hlavicky
NCOLS=$(awk -F, 'NR==1{print NF; exit}' "$DATA")
echo "dataset $DATA ma $NCOLS stlpcov"

mkdir -p results

# jeden beh benchmarku na pozadi: vlastny log, vlastny cas
launch() {
    local sym="$1" w="$2" out_csv="$3"
    (
        start=$SECONDS
        ./benchmark "$DATA" "$out_csv" "$sym" ${w:+"$w"}
        echo "hotovo sym=$sym ${w:-flow}: $((SECONDS - start)) s" | tee -a
    ) &
    # nepusti viac ako JOBS uloh naraz
    while [ "$(jobs -rp | wc -l)" -ge "$JOBS" ]; do sleep 1; done
}

total_start=$SECONDS

for sym in 0 1 2; do
    if [ "$NCOLS" -eq 11 ]; then
        for w in flow packet byte; do
            echo "spustam sym=$sym weighting=$w"
            launch "$sym" "$w" "$OUT/run_sym${sym}_${w}.csv"
        done
    else
        echo "spustam sym=$sym (simple tuple)"
        launch "$sym" "" "$OUT/run_sym${sym}_flow.csv"
    fi
done

wait
echo "vsetky behy hotove za $((SECONDS - total_start)) s"

make clean || true

echo "running python program"
source venv/bin/activate

python analyze.py "$OUT" "$OUT"
