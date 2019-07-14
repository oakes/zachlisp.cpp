#include <iostream>
#include <string>
#include <vector>
#include <variant>
#include <regex>

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

void tokenize(std::string input) {
    std::regex e(
        "([\\s,]*)|" // whitespace
        "(~@)|" // special two-characters
        "([\\[\\]{}()\'`~^@])|" // special single characters
        "(\"(?:\\\\.|[^\\\\\"])*\"?)|" // strings
        "(;.*)|" // comments
        "([^\\s\\[\\]{}(\'\"`,;)]*)" // symbols, numbers, etc
    );
    std::sregex_iterator it(input.begin(), input.end(), e);
    std::sregex_iterator end;

    while (it != end) {
        std::smatch match = *it;
        std::string token = match.str();
        int index = -1;
        for(auto i = 1; i < match.size(); ++i){
           if (!match[i].str().empty()) {
               index = i-1;
               std::cout << token << " at position " << match.position() << " with group " << index << std::endl;
               break;
           }
        }
        it++;
    }
}

std::string READ(const std::string input) {
    tokenize(input);
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
