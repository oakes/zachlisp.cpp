#include <iostream>
#include <string>

#include "read.hpp"
#include "eval.hpp"
#include "print.hpp"

int main(int argc, char* argv[]) {
    chaiscript::ChaiScript chai;
    std::string input;
    do {
        std::cout << "user> ";
        std::getline(std::cin, input);
        std::cout << zachlisp::print(zachlisp::eval(zachlisp::read(input), &chai));
    } while (!std::cin.fail());
    return 0;
}
