#include <iostream>
#include <vector>
#include <variant>

struct TokenWrapper;

using TokenVector = std::vector<TokenWrapper>;
using Token = std::variant<long, double, std::string, TokenVector>;

struct TokenWrapper {
    Token _data;

    template <typename... Ts, typename = 
        std::enable_if_t // https://vittorioromeo.info/index/blog/variants_lambdas_part_2.html#libcpp_constraint
        <
            !std::disjunction_v
            <
                std::is_same<std::decay_t<Ts>, TokenWrapper>...
            >
        >
    >
    TokenWrapper(Ts&&... xs)
        : _data{std::forward<Ts>(xs)...}
    {
    }
};

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
