#!/usr/bin/env python3
"""Build the small test dataset tests/data/flows_small.csv from data/flow_big.csv.

flow_big.csv is synthetic (not a CESNET capture). Its first 1500 flows are
taken and their packet counts divided by 100, so the reference runs stay small
(tens of thousands of packets instead of millions). Byte counts are scaled with
the packets, so the mean packet size of every direction stays the same.

At the end a few edge-case rows are appended, each starting after the last
regular flow so the file stays sorted by flow start time:
  - reverse direction without packets
  - flowend earlier than flowstart (the parser turns it into an instant flow)
  - malformed row with 8 fields (the reader must skip it)
  - flow with a single packet
  - 20 packets with zero duration (all at the same instant)
  - forward direction without packets, only the reverse one has traffic
"""

import csv
import sys
from datetime import datetime, timedelta
from pathlib import Path

SOURCE = Path("data/flow_big.csv")
TARGET = Path("tests/data/flows_small.csv")
ROWS = 1500
SCALE = 100
TS_FORMAT = "%Y-%m-%d %H:%M:%S"


def scale(packets: int, nbytes: int) -> tuple[int, int]:
    """Divide the packet count by SCALE (at least 1), keep the mean packet size."""
    if packets == 0:
        return 0, 0
    new_packets = max(1, packets // SCALE)
    return new_packets, nbytes * new_packets // packets


def parse_ts(ts: str) -> datetime:
    # "2026-07-11 00:00:00.000288582" - Python keeps microseconds only, which is
    # enough here: it is used only to place the edge rows after the last flow
    main, frac = ts.split(".")
    return datetime.strptime(main, TS_FORMAT) + timedelta(microseconds=int(frac[:6]))


def format_ts(t: datetime) -> str:
    return t.strftime(TS_FORMAT) + ".%09d" % (t.microsecond * 1000)


def row_line(start, end, src, sport, dst, dport, proto, b, p, brev, prev) -> str:
    """One row in the same quoting style as the source: strings quoted, numbers not."""
    return f'"{start}","{end}","{src}",{sport},"{dst}",{dport},{proto},{b},{p},{brev},{prev}'


def main() -> int:
    if not SOURCE.exists():
        sys.stderr.write(f"missing {SOURCE}\n")
        return 1

    with SOURCE.open(newline="") as f:
        header = f.readline().rstrip("\n")
        reader = csv.reader(f)
        rows = [next(reader) for _ in range(ROWS)]

    lines = [header]
    for r in rows:
        start, end, src, sport, dst, dport, proto = r[:7]
        p, b = scale(int(r[8]), int(r[7]))
        prev, brev = scale(int(r[10]), int(r[9]))
        lines.append(row_line(start, end, src, sport, dst, dport, proto, b, p, brev, prev))

    # edge rows start after the last regular flow, one second apart
    t = parse_ts(rows[-1][0]) + timedelta(seconds=1)

    def at(seconds: float) -> str:
        return format_ts(t + timedelta(seconds=seconds))

    a, z = "::ffff:192.0.2.1", "::ffff:198.51.100.7"
    lines.append(row_line(at(0), at(2), a, 40000, z, 443, 6, 3000, 5, 0, 0))      # rev without packets
    lines.append(row_line(at(1), at(0.5), a, 40001, z, 443, 6, 800, 4, 400, 2))   # end before start
    lines.append(f'"{at(2)}","{at(3)}","{a}",40002,"{z}",443,6,100')              # malformed, 8 fields
    lines.append(row_line(at(3), at(3), a, 40003, z, 53, 17, 70, 1, 0, 0))        # single packet
    lines.append(row_line(at(4), at(4), a, 40004, z, 80, 6, 20000, 20, 0, 0))     # 20 packets, zero duration
    lines.append(row_line(at(5), at(6), a, 40005, z, 22, 6, 0, 0, 1500, 3))       # only reverse traffic

    TARGET.parent.mkdir(parents=True, exist_ok=True)
    TARGET.write_text("\n".join(lines) + "\n")
    print(f"wrote {TARGET}: {len(lines)} lines")
    return 0


if __name__ == "__main__":
    sys.exit(main())
