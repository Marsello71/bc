#!/bin/bash
# Reference ("golden") results from the old code at commit 0aaa540, on the small
# test dataset. Every refactor step must reproduce these numbers exactly.
#
# The old code is configured by #defines, so it is built once per combination
# (RETA on/off x full tuple / IP only) with a small window, in a temporary git
# worktree - the working copy is never touched.
#
# usage: bash tests/make_golden.sh        (run from the repository root)
set -e

REF_COMMIT=0aaa540
WINDOW=500
REPO=$(pwd)
DATA="$REPO/tests/data/flows_small.csv"
OUT="$REPO/tests/golden"

TMP=$(mktemp -d)
WT="$TMP/wt"
cleanup() {
    git -C "$REPO" worktree remove --force "$WT" 2>/dev/null || true
    rm -rf "$TMP"
}
trap cleanup EXIT

git -C "$REPO" worktree add --detach "$WT" "$REF_COMMIT" >/dev/null
rm -rf "$OUT"
mkdir -p "$OUT"

# build the old benchmark with the given RETA_ON / IP_ONLY_ON and the test window
build() {
    local reta_on="$1" ip_only="$2"
    local cfg="$WT/src/config.hpp"
    git -C "$WT" checkout -q -- src/config.hpp
    sed -i.bak \
        -e "s/^#define RETA_ON .*/#define RETA_ON $reta_on/" \
        -e "s/^#define IP_ONLY_ON .*/#define IP_ONLY_ON $ip_only/" \
        -e "s/constexpr uint32_t WINDOW_SIZE = [0-9]*;/constexpr uint32_t WINDOW_SIZE = $WINDOW;/" \
        "$cfg"
    rm -f "$cfg.bak"
    # make sure all three replacements really happened
    grep -q "^#define RETA_ON $reta_on\$" "$cfg"
    grep -q "^#define IP_ONLY_ON $ip_only\$" "$cfg"
    grep -q "WINDOW_SIZE = $WINDOW;" "$cfg"
    # the old Makefile does not track header changes, so always rebuild from scratch
    make -C "$WT" clean >/dev/null 2>&1 || true
    make -C "$WT" -s >/dev/null 2>&1
}

SYMS=(none xorfold sortfold)

for reta_on in 1 0; do
    for ip_only in 0 1; do
        reta_name=$([ "$reta_on" = 1 ] && echo retaon || echo retaoff)
        tuple_name=$([ "$ip_only" = 1 ] && echo ip || echo full)
        echo "building RETA_ON=$reta_on IP_ONLY_ON=$ip_only"
        build "$reta_on" "$ip_only"
        for sym in 0 1 2; do
            for w in flow packet byte; do
                "$WT/benchmark" "$DATA" "$OUT/${reta_name}_${tuple_name}_${SYMS[$sym]}_${w}.csv" "$sym" "$w" >/dev/null
            done
        done
    done
done

# JSQ ignores the tuple and RETA, one run per weighting is enough
echo "building for JSQ"
build 1 0
for w in flow packet byte; do
    "$WT/benchmark" "$DATA" "$OUT/jsq_${w}.csv" 0 jsq "$w" >/dev/null
done

echo "golden results: $(ls "$OUT" | wc -l | tr -d ' ') files in $OUT"
