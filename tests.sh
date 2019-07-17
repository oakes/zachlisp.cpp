#!/bin/bash
EXEC=zachlisp
STEP=${1:-step1_read_print}
g++ repl.cpp -o $EXEC -std=c++17 && ./tests/runtest.py tests/$STEP.mal -- ./$EXEC
