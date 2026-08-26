#!/bin/bash

echo "running rss_analyzer.cpp"
make 

symmetry=1

echo $symmetry
./benchmark data/output.csv results/FULL/FULL_RUN_results.csv $symmetry
./benchmark data/sweep_dstip_50k.csv results/PER_FIELD/dstIP/sweep_dstip_results.csv $symmetry
./benchmark data/sweep_dstport_50k.csv results/PER_FIELD/dstPort/sweep_dstport_results.csv $symmetry
./benchmark data/sweep_srcip_50k.csv results/PER_FIELD/srcIP/sweep_srcip_results.csv $symmetry
./benchmark data/sweep_srcport_50k.csv results/PER_FIELD/srcPort/sweep_srcport_results.csv $symmetry
make clean

echo "running python program"
source venv/bin/activate

python analyze.py results/FULL/FULL_RUN_results.csv results/FULL/
python analyze.py results/PER_FIELD/dstIP/sweep_dstip_results.csv results/PER_FIELD/dstIP/
python analyze.py results/PER_FIELD/dstPort/sweep_dstport_results.csv results/PER_FIELD/dstPort/
python analyze.py results/PER_FIELD/srcIP/sweep_srcip_results.csv results/PER_FIELD/srcIP/
python analyze.py results/PER_FIELD/srcPort/sweep_srcport_results.csv results/PER_FIELD/srcPort/
