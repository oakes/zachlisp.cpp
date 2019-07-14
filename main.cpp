#include <iostream>
#include <string>
#include <vector>
#include <variant>
#include <regex>

struct DataWrapper;

using DataVector = std::vector<DataWrapper>;
using Data = std::variant<long, double, std::string, DataVector>;

struct DataWrapper {
    Data _data;

    template <typename... Ts, typename = 
        std::enable_if_t // https://vittorioromeo.info/index/blog/variants_lambdas_part_2.html#libcpp_constraint
        <
            !std::disjunction_v
            <
                std::is_same<std::decay_t<Ts>, DataWrapper>...
            >
        >
    >
    DataWrapper(Ts&&... xs)
        : _data{std::forward<Ts>(xs)...}
    {
    }
};

enum Type {Whitespace, SpliceUnquote, SpecialChar, String, Comment, Symbol};

const std::regex TYPE(
    "([\\s,]*)|" // Whitespace
    "(~@)|" // SpliceUnquote
    "([\\[\\]{}()\'`~^@])|" // SpecialChar
    "(\"(?:\\\\.|[^\\\\\"])*\"?)|" // String
    "(;.*)|" // Comment
    "([^\\s\\[\\]{}(\'\"`,;)]*)" // Symbol
);

struct Token {
    std::string value;
    int position;
    Type type;

    Token(std::string v, int p, Type t) : value(v), position(p), type(t) {}
};

std::vector<std::shared_ptr<Token> > tokenize(std::string input) {
    std::sregex_iterator begin(input.begin(), input.end(), TYPE);
    std::sregex_iterator end;

    std::vector<std::shared_ptr<Token> > tokens;

    for (std::sregex_iterator it = begin; it != end; ++it) {
        std::smatch match = *it;
        for(auto i = 1; i < match.size(); ++i){
           if (!match[i].str().empty()) {
               int position = match.position();
               int index = i-1;
               auto token = std::make_shared<Token>(Token{match.str(), position, static_cast<Type>(index)});
               tokens.push_back(token);
               break;
           }
        }
    }

    return tokens;
}

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
