#!/bin/bash

echo "running rss_analyzer.cpp"
make 

symmetry=0

echo $symmetry

./benchmark data/output.csv results/run_sym0.csv 0
./benchmark data/output.csv results/run_sym1.csv 1
./benchmark data/output.csv results/run_sym2.csv 2
make clean

echo "running python program"
source venv/bin/activate

DMA=128

echo $DMA

python analyze.py results/run_sym0.csv results/run_sym1.csv results/run_sym2.csv results/ $DMA


