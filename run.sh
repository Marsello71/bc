#!/bin/bash
set -e

DATA="${1:-data/odid1.csv}"   # vstupny dataset (11 stlpcov = flow CSV, 3/5 = simple tuple)
OUT="${2:-results}"           # kam sa pisu run_sym*.csv, jsq_*.csv a grafy (RETA beh daj inam, nech neprepise stary)
LOGS="$OUT/logs"              # vystup kazdeho behu (riadok "Streamed N packets, M windows ...")

# vsetky behy su na sebe nezavisle, tak ich pustam naraz. na 10 jadrach je 9 uloh
# akurat - kazda si cita dataset zvlast, ale citanie je pod 1 % casu (95 % su hashe).
# s JSQ je uloh 12 - JSQ sa spusti, ked sa uvolni miesto po niektorom hashovom behu.
JOBS="${JOBS:-9}"

echo "building"
make

# format zistim z poctu stlpcov hlavicky
NCOLS=$(awk -F, 'NR==1{print NF; exit}' "$DATA")
echo "dataset $DATA ma $NCOLS stlpcov"

mkdir -p "$OUT" "$LOGS"
rm -f "$LOGS"/*.failed        # znacky zlyhanych behov z minuleho spustenia

# jeden beh benchmarku na pozadi: vlastny log, vlastny cas.
#   launch <meno> <vystup.csv> <argumenty benchmarku za datasetom a vystupom...>
# ak beh zlyha, zanecha znacku $LOGS/<meno>.failed - podla nej sa na konci
# zisti, ci vsetko preslo (samotne `wait` chybu behu na pozadi nevrati)
launch() {
    local name="$1" out_csv="$2"
    shift 2
    (
        start=$SECONDS
        if ./benchmark "$DATA" "$out_csv" "$@" > "$LOGS/$name.log" 2>&1; then
            echo "hotovo $name: $((SECONDS - start)) s"
        else
            echo "CHYBA $name po $((SECONDS - start)) s - pozri $LOGS/$name.log"
            touch "$LOGS/$name.failed"
        fi
    ) &
    # nepusti viac ako JOBS uloh naraz
    while [ "$(jobs -rp | wc -l)" -ge "$JOBS" ]; do sleep 1; done
}

total_start=$SECONDS

for sym in 0 1 2; do
    if [ "$NCOLS" -eq 11 ]; then
        for w in flow packet byte; do
            echo "spustam sym=$sym weighting=$w"
            launch "sym${sym}_${w}" "$OUT/run_sym${sym}_${w}.csv" "$sym" "$w"
        done
    else
        echo "spustam sym=$sym (simple tuple)"
        launch "sym${sym}" "$OUT/run_sym${sym}_flow.csv" "$sym"
    fi
done

# JSQ oracle reference: symmetry has no effect on it, so sym 0 only, one run per weighting.
# named jsq_*.csv on purpose - analyze.py would otherwise mix it into the hash plots
if [ "$NCOLS" -eq 11 ]; then
    for w in flow packet byte; do
        echo "spustam jsq weighting=$w"
        launch "jsq_${w}" "$OUT/jsq_${w}.csv" 0 jsq "$w"
    done
fi

wait
echo "vsetky behy hotove za $((SECONDS - total_start)) s"

make clean || true

# ak niektory beh zlyhal, jeho CSV je neuplne - grafy z neho by klamali
failed=$(ls "$LOGS"/*.failed 2>/dev/null | wc -l | tr -d ' ')
if [ "$failed" -gt 0 ]; then
    echo "zlyhalo $failed behov, analyze.py nespustam:"
    ls "$LOGS"/*.failed
    exit 1
fi

echo "running python program"
source venv/bin/activate

python analyze.py "$OUT" "$OUT"
