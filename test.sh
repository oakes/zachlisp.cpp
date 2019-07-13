#!/bin/bash
env STEP=$1 ./tests/runtest.py tests/$1.mal -- ./run.sh
