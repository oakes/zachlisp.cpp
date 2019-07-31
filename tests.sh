#!/bin/bash
EXEC=zachlisp
STEP=${1:-step2_eval}
./tests/runtest.py tests/$STEP.mal -- ./$EXEC
