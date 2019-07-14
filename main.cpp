#include <iostream>

std::string READ(const std::string input) {
    return input;
}

std::string EVAL(const std::string input) {
    return input;
}

std::string PRINT(const std::string input) {
    return input;
}

std::string rep(const std::string input) {
    return PRINT(EVAL(READ(input)));
}

int main(int argc, char* argv[]) {
    std::string input;
    do {
        std::cout << "user> ";
        std::getline(std::cin, input);
        std::cout << rep(input) << std::endl;
    } while (!std::cin.fail());
    return 0;
}
