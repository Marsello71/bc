#!/bin/bash
# Build the simulator, run one experiment and draw its plot.
#
#   ./run.sh                 uses config.conf
#   ./run.sh my_test.conf    uses another config file
set -e

CONFIG="${1:-config.conf}"

echo "building"
make

echo "running simulation: $CONFIG"
./rss_sim "$CONFIG"

# use the project's Python environment if it exists
PYTHON=python3
if [ -x venv/bin/python ]; then
    PYTHON=venv/bin/python
fi

echo "drawing the plot"
"$PYTHON" analyze.py "$CONFIG"
