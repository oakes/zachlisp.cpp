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

enum ValueName {BoolValue, CharValue, LongValue, DoubleValue, StringValue};
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

std::optional<std::pair<Form, std::list<Token>::const_iterator> > readForm(const std::list<Token> *tokens, std::list<Token>::const_iterator it);

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
                        return std::make_pair(ReaderError{"Unmatched delimiter: " + std::string(1, c), token}, tokens->end());
                }
            }
        }
        if (auto retOpt = readForm(tokens, it)) {
            auto ret = retOpt.value();
            forms.push_back(FormWrapper{ret.first});
            it = ret.second;
        } else {
            ++it;
        }
    }
    return std::make_pair(ReaderError{"EOF: no " + std::string(1, endDelimiter) + " found", std::nullopt}, tokens->end());
}

std::optional<std::pair<Form, std::list<Token>::const_iterator> > readForm(const std::list<Token> *tokens, std::list<Token>::const_iterator it) {
    if (it == tokens->end()) {
        return std::nullopt;
    }
    auto token = *it;
    switch (token.type) {
        case Whitespace:
            return std::nullopt;
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
                        return readList(tokens, ++it, ']');
                    case '{':
                        return readList(tokens, ++it, '}');
                    case ')':
                    case ']':
                    case '}':
                        return std::make_pair(ReaderError{"Unmatched delimiter: " + std::string(1, c), token}, tokens->end());
                }
                break;
            }
        case String:
            {
                std::string s = std::get<std::string>(token.value);
                if (s.size() < 2 || s.back() != '"') {
                    return std::make_pair(ReaderError{"EOF: unbalanced quote", token}, tokens->end());
                } else {
                    token.value = s.substr(1, s.size() - 2);
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
        if (auto retOpt = readForm(tokens, it)) {
            auto ret = retOpt.value();
            forms.push_back(ret.first);
            it = ret.second;
        } else {
            ++it;
        }
    }
    return forms;
}

std::string prStr(Token token) {
    switch (token.value.index()) {
        case BoolValue:
            return std::get<bool>(token.value) ? "true" : "false";
        case CharValue:
            return std::string(1, std::get<char>(token.value));
        case LongValue:
            return std::to_string(std::get<long>(token.value));
        case DoubleValue:
            return std::to_string(std::get<double>(token.value));
        case StringValue:
            {
                std::string s = std::get<std::string>(token.value);
                if (token.type == String) {
                    return "\"" + s + "\"";
                } else {
                    return s;
                }
            }
    }
    return "";
}

std::string prStr(Form form);

template <template <class> class T>
std::string prStr(T<FormWrapper> list) {
    std::string s;
    for (auto item : list) {
        if (s.size() > 0) {
            s += " ";
        }
        s += prStr(item.form);
    }
    return s;
}

std::string prStr(Form form) {
    switch (form.index()) {
        case 0: // ReaderError
            {
                auto error = std::get<ReaderError>(form);
                return "#ReaderError \"" + error.message + "\"";
            }
        case 1: // Token
            return prStr(std::get<Token>(form));
        case 2: // list
            return "(" + prStr<std::list>(std::get<std::list<FormWrapper> >(form)) + ")";
        case 3: // vector
            return "[" + prStr<std::vector>(std::get<std::vector<FormWrapper> >(form)) + "]";
        case 4: // map
            return "{" + prStr<std::list>(std::get<std::list<FormWrapper> >(form)) + "}";
        case 5: // set
            return "#{" + prStr<std::list>(std::get<std::list<FormWrapper> >(form)) + "}";
    }
    return "";
}

std::list<Form> READ(const std::string input) {
    auto tokens = tokenize(input);
    auto forms = readForms(&tokens);
    return forms;
}

std::list<Form> EVAL(const std::list<Form> forms) {
    return forms;
}

std::string PRINT(const std::list<Form> forms) {
    std::string s;
    for (auto form : forms) {
        s += prStr(form) + "\n";
    }
    return s;
}

std::string rep(const std::string input) {
    return PRINT(EVAL(READ(input)));
}

int main(int argc, char* argv[]) {
    std::string input;
    do {
        std::cout << "user> ";
        std::getline(std::cin, input);
        std::cout << rep(input);
    } while (!std::cin.fail());
    return 0;
}
