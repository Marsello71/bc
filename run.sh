#!/bin/bash

echo "running rss_analyzer.cpp"
make run
make clean

echo "running python program"
source venv/bin/activate
python histogram.py
