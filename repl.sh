#!/bin/bash
g++ repl.cpp -O3 -ldl -lpthread -o ${1:-zachlisp} -std=c++17 && ./${1:-zachlisp}
