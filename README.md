## Introduction

You know what the world needs? Another Lisp. I wrote this one in C++17 and haven't figured out why it exists yet. It consists of three files:

* [read.hpp](read.hpp) reads lisp data into C++ data structures. It supports the four Clojure data structure literals: `()` becomes `std::list`, `[]` becomes `std::vector`, `{}` becomes `std::unordered_map`, and `#{}` becomes `std::unordered_set`. It has no dependencies so it can be easily used on its own as a dead simple [edn](https://github.com/edn-format/edn) reader.
* [eval.hpp](eval.hpp) takes the result of `zachlisp::read` and evaluates it using [ChaiScript](http://chaiscript.com/).
* [print.hpp](print.hpp) takes the result of `zachlisp::eval` and prints it back into lisp syntax.

In [repl.cpp](repl.cpp) they are combined to create an interactive REPL.

## Build Instructions

On Linux, run `./repl.sh`. On Windows, install [Scoop](https://scoop.sh), and then in PowerShell run `scoop install gcc` and `.\repl.ps1`.

## Licensing

All files that originate from this project are dedicated to the public domain. I would love pull requests, and will assume that they are also dedicated to the public domain.
