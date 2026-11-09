#!/usr/bin/env python3
"""Compare two directories of simulator results row by row.

usage: python3 tests/compare.py <reference_dir> <new_dir>
exit code 0 = every row matches, 1 = a difference or a row missing on one side

Every row is reduced to
    key    = (method, tuple, reta, symmetry, weighting, hash, key_id, num_channels, window)
    values = (overload, fairness, chi)   -- compared as TEXT, so even the last
                                            printed digit has to match
Both formats are understood:
  - old format (header starts with "algorithm,"): tuple and RETA come from the
    file name, because the old CSV did not record them:
        <retaon|retaoff>_<full|ip>_<sym>_<weighting>.csv   golden results
        jsq_<weighting>.csv                                 old JSQ run (symmetry none)
        run_sym<0|1|2>_<weighting>.csv                      old run.sh output (full tuple, RETA on)
  - new format (header has a "method" column): everything is in the row.

Big result sets (the full dataset has tens of millions of rows per side) do not
fit into memory, so each side is written out as sorted text lines (the system
`sort` works on disk) and the two sorted files are walked side by side.
"""

import csv
import os
import re
import subprocess
import sys
import tempfile
from pathlib import Path

SYMMETRY_BY_NUMBER = {"0": "none", "1": "xorfold", "2": "sortfold"}
GOLDEN_NAME = re.compile(r"^(retaon|retaoff)_(full|ip)_(none|xorfold|sortfold)_(flow|packet|byte)\.csv$")
JSQ_NAME = re.compile(r"^jsq_(flow|packet|byte)\.csv$")
RUN_NAME = re.compile(r"^run_sym([012])_(flow|packet|byte)\.csv$")
SEP = "\t"


def old_file_context(name: str):
    """(method, tuple, reta) for an old-format file, or None if the name is unknown."""
    m = GOLDEN_NAME.match(name)
    if m:
        return "hash", m.group(2), "on" if m.group(1) == "retaon" else "off"
    if JSQ_NAME.match(name):
        return "jsq", "-", "-"
    if RUN_NAME.match(name):
        return "hash", "full", "on"
    return None


def rows_of(path: Path):
    """Yield (key, values) for every row of one result file."""
    with path.open(newline="") as f:
        reader = csv.DictReader(f)
        header = reader.fieldnames or []
        if "method" in header:                                  # new format
            for r in reader:
                key = (r["method"], r["tuple"], r["reta"], r["symmetry"], r["weighting"],
                       r["hash"], r["key_id"], r["num_channels"], r["window"])
                yield key, (r["overload"], r["fairness"], r["chi"])
        elif header[:1] == ["algorithm"]:                       # old format
            context = old_file_context(path.name)
            if context is None:
                print(f"  skipping {path} (unknown old-format file name)")
                return
            method, tup, reta = context
            for r in reader:
                hash_name = "-" if method == "jsq" else r["algorithm"]
                symmetry = "none" if method == "jsq" else r["symmetry"]
                key = (method, tup, reta, symmetry, r["weighting"], hash_name,
                       r["key_id"], r["num_channels"], r["tuple_run_index"])
                yield key, (r["thresshold_sum"], r["fairness"], r["chi"])
        else:
            print(f"  skipping {path} (not a results file)")


def write_sorted(directory: Path, tmp: Path, label: str) -> Path:
    """All rows of a directory as 'key<TAB>values' lines, sorted with the system sort."""
    raw = tmp / f"{label}.raw"
    count = 0
    with raw.open("w") as out:
        for path in sorted(directory.glob("*.csv")):
            for key, values in rows_of(path):
                out.write("|".join(key) + SEP + "|".join(values) + "\n")
                count += 1
    print(f"  {label}: {count} rows from {directory}")
    sorted_path = tmp / f"{label}.sorted"
    env = dict(os.environ, LC_ALL="C")
    subprocess.run(["sort", "-o", str(sorted_path), str(raw)], check=True, env=env)
    raw.unlink()
    return sorted_path


def split(line: str):
    key, values = line.rstrip("\n").split(SEP)
    return key, values


def main() -> int:
    if len(sys.argv) != 3:
        sys.stderr.write("usage: compare.py <reference_dir> <new_dir>\n")
        return 2
    ref_dir, new_dir = Path(sys.argv[1]), Path(sys.argv[2])
    for d in (ref_dir, new_dir):
        if not d.is_dir():
            sys.stderr.write(f"not a directory: {d}\n")
            return 2

    with tempfile.TemporaryDirectory() as t:
        tmp = Path(t)
        ref_sorted = write_sorted(ref_dir, tmp, "reference")
        new_sorted = write_sorted(new_dir, tmp, "new")

        compared = 0
        diffs, only_ref, only_new, duplicates = [], [], [], []
        n_diff = n_only_ref = n_only_new = n_dup = 0

        with ref_sorted.open() as a, new_sorted.open() as b:
            la, lb = a.readline(), b.readline()
            prev_a = prev_b = None
            while la or lb:
                ka = split(la) if la else None
                kb = split(lb) if lb else None
                # the same key twice on one side means two files describe the same thing
                for k, prev, side in ((ka, prev_a, "reference"), (kb, prev_b, "new")):
                    if k and prev == k[0]:
                        n_dup += 1
                        if len(duplicates) < 10:
                            duplicates.append(f"{side}: {k[0]}")
                if ka and (not kb or ka[0] < kb[0]):
                    n_only_ref += 1
                    if len(only_ref) < 10:
                        only_ref.append(ka[0])
                    prev_a, la = ka[0], a.readline()
                elif kb and (not ka or kb[0] < ka[0]):
                    n_only_new += 1
                    if len(only_new) < 10:
                        only_new.append(kb[0])
                    prev_b, lb = kb[0], b.readline()
                else:
                    compared += 1
                    if ka[1] != kb[1]:
                        n_diff += 1
                        if len(diffs) < 10:
                            diffs.append(f"{ka[0]}\n      reference: {ka[1]}\n      new:       {kb[1]}")
                    prev_a, la = ka[0], a.readline()
                    prev_b, lb = kb[0], b.readline()

    print("key = method|tuple|reta|symmetry|weighting|hash|key_id|num_channels|window ; values = overload|fairness|chi")
    for title, n, items in (("rozdiely v hodnotach", n_diff, diffs),
                            ("chyba v novom", n_only_ref, only_ref),
                            ("navyse v novom", n_only_new, only_new),
                            ("duplicitne kluce", n_dup, duplicates)):
        if n:
            print(f"{title}: {n} (prvych {len(items)}):")
            for item in items:
                print(f"    {item}")

    if compared and not (n_diff or n_only_ref or n_only_new or n_dup):
        print(f"OK: {compared} riadkov, 0 rozdielov")
        return 0
    print(f"CHYBA: porovnanych {compared}, rozdielov {n_diff}, chyba v novom {n_only_ref}, "
          f"navyse v novom {n_only_new}, duplicit {n_dup}")
    return 1


if __name__ == "__main__":
    sys.exit(main())
