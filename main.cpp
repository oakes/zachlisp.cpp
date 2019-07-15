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

struct ReaderError {
    std::string message;
    std::optional<Token> token;

    ReaderError(std::string m, std::optional<Token> t) : message(m), token(t) {}
};

struct FormWrapper;

using Form = std::variant<
    ReaderError,
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

std::list<Token> tokenize(std::string input) {
    std::sregex_iterator begin(input.begin(), input.end(), TYPE);
    std::sregex_iterator end;

    std::list<Token> tokens;

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

std::pair<Form, std::list<Token>::const_iterator> readForm(const std::list<Token> *tokens, std::list<Token>::const_iterator it);

std::pair<Form, std::list<Token>::const_iterator> readList(const std::list<Token> *tokens, std::list<Token>::const_iterator it, char endDelimiter) {
    std::list<FormWrapper> forms;
    while (it != tokens->end()) {
        auto token = *it;
        if (token.type == SpecialChar) {
            char c = std::get<char>(token.value);
            if (c == endDelimiter) {
                return std::make_pair(forms, ++it);
            } else {
                switch (c) {
                    case ')':
                    case ']':
                    case '}':
                        return std::make_pair(ReaderError{"Unmatched delimiter: " + c, token}, tokens->end());
                }
            }
        }
        auto ret = readForm(tokens, it);
        auto form = ret.first;
        it = ret.second;
        forms.push_back(FormWrapper{form});
    }
    return std::make_pair(ReaderError{"No end delimiter found: " + endDelimiter, std::nullopt}, tokens->end());
}

template <class T>
std::vector<T> listToVector(const std::list<T> list) {
    std::vector<T> v {
        std::make_move_iterator(std::begin(list)),
        std::make_move_iterator(std::end(list))
    };
    return v;
}

std::pair<Form, std::list<Token>::const_iterator> readForm(const std::list<Token> *tokens, std::list<Token>::const_iterator it) {
    auto token = *it;
    switch (token.type) {
        case Whitespace:
            return readForm(tokens, ++it);
        case SpecialChars:
            {
                std::string s = std::get<std::string>(token.value);
                if (s == "#{") {
                    return readList(tokens, ++it, '}');
                }
                break;
            }
        case SpecialChar:
            {
                char c = std::get<char>(token.value);
                switch (c) {
                    case '(':
                        return readList(tokens, ++it, ')');
                    case '[':
                        {
                            auto ret = readList(tokens, ++it, ']');
                            auto list = std::get<std::list<FormWrapper> >(ret.first);
                            return std::make_pair(listToVector<FormWrapper>(list), ret.second);
                        }
                    case '{':
                        return readList(tokens, ++it, '}');
                    case ')':
                    case ']':
                    case '}':
                        return std::make_pair(ReaderError{"Unmatched delimiter: " + c, token}, tokens->end());
                }
                break;
            }
    }
    return std::make_pair(token, ++it);
}

std::list<Form> readForms(const std::list<Token> *tokens) {
    std::list<Form> forms;
    std::list<Token>::const_iterator it = tokens->begin();
    while (it != tokens->end()) {
        auto ret = readForm(tokens, it);
        forms.push_back(ret.first);
        it = ret.second;
    }
    return forms;
}

std::string READ(const std::string input) {
    auto tokens = tokenize(input);
    auto forms = readForms(&tokens);
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
