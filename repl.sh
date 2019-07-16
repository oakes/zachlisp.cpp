#!/bin/bash
g++ repl.cpp -o ${1:-zachlisp} -std=c++17 && ./${1:-zachlisp}
