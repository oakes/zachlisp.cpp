#include <iostream>
#include <string>

#include "zachlisp.hpp"

int main(int argc, char* argv[]) {
    std::string input;
    do {
        std::cout << "user> ";
        std::getline(std::cin, input);
        std::cout << zachlisp::rep(input);
    } while (!std::cin.fail());
    return 0;
}