## Introduction

ZachLisp is a little lisp reader and printer written in C++17. The entire reader is in [read.hpp](read.hpp) and has no dependencies, so it's easy to throw into a project.

See [repl.cpp](repl.cpp) for an example of how to use it. You could probably use it as a dead simple [edn](https://github.com/edn-format/edn) reader. It supports the four Clojure data structures literals: `()` becomes `std::list`, `[]` becomes `std::vector`, `{}` becomes `std::unordered_map`, and `#{}` becomes `std::unordered_set`.

## Build Instructions

On Linux, run `./repl.sh`. On Windows, install [Scoop](https://scoop.sh), and then in PowerShell run `scoop install gcc` and `.\repl.ps1`.

## Licensing

All files that originate from this project are dedicated to the public domain. I would love pull requests, and will assume that they are also dedicated to the public domain.
