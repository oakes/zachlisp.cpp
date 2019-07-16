#!/bin/bash
g++ repl.cpp -o ${1:-step1_read_print} -std=c++17 && ./tests/runtest.py tests/${1:-step1_read_print}.mal -- ./run.sh
