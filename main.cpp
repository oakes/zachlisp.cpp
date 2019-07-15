#include <iostream>
#include <string>
#include <list>
#include <vector>
#include <map>
#include <set>
#include <variant>
#include <regex>

enum Type {
    Whitespace, SpecialChars, SpecialChar, String, Comment, Number, Symbol
};

const std::regex TYPE(
    "([\\s,]*)|" // Whitespace
    "(~@|#\\{)|" // SpecialChars
    "([\\[\\]{}()\'`~^@])|" // SpecialChar
    "(\"(?:\\\\.|[^\\\\\"])*\"?)|" // String
    "(;.*)|" // Comment
    "(\\d+\\.?\\d*)|" // Number
    "([^\\s\\[\\]{}(\'\"`,;)]*)" // Symbol
);

using Value = std::variant<bool, char, long, double, std::string>;

struct Token {
    Value value;
    Type type;
    int line;
    int column;

    Token(Value v, Type t, int l, int c) : value(v), type(t), line(l), column(c) {}
};

struct FormWrapper;

using Form = std::variant<
    Token,
    std::list<FormWrapper>,
    std::vector<FormWrapper>,
    std::map<FormWrapper, FormWrapper>,
    std::set<FormWrapper>
>;

struct FormWrapper {
     Form form;

     FormWrapper(Form f) : form(f) {}
};

Value parse(std::string value, Type type) {
    switch (type) {
        case SpecialChar:
            return value[0];
        case Number:
            if (value.find('.') == std::string::npos) {
                return std::stol(value);
            } else {
                return std::stod(value);
            }
        case Symbol:
            if (value == "true") {
                return true;
            } else if (value == "false") {
                return false;
            }
    }
    return value;
}

std::list<Form> tokenize(std::string input) {
    std::sregex_iterator begin(input.begin(), input.end(), TYPE);
    std::sregex_iterator end;

    std::list<Form> tokens;

    int line = 1;

    for (auto it = begin; it != end; ++it) {
        std::smatch match = *it;
        for(auto i = 1; i < match.size(); ++i){
           if (!match[i].str().empty()) {
               std::string valueStr = match.str();
               Type type = static_cast<Type>(i-1);
               Value value = parse(valueStr, type);
               int column = match.position() + 1;
               tokens.push_back(Token{value, type, line, column});
               line += std::count(valueStr.begin(), valueStr.end(), '\n');
               break;
           }
        }
    }

    return tokens;
}

std::list<Form> organize(std::list<Form> forms) {
    auto end = forms.end();
    for (auto it = forms.begin(); it != end; ++it) {
        auto token = std::get<Token>(*it);
        switch (token.type) {
            case Whitespace:
                forms.erase(it);
                --it;
        }
    }
    return forms;
}

std::string READ(const std::string input) {
    auto tokens = organize(tokenize(input));
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
