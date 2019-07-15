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

enum FormName {ReaderErrorForm, TokenForm, ListForm, VectorForm, MapForm, SetForm};

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

std::map<std::variant<char, std::string>, std::string> EXPANDED_NAMES = {
    {'\'', "quote"},
    {'`', "quasiquote"},
    {'~', "unquote"},
    {'@', "deref"},
    {"~@", "splice-unquote"}
};

std::map<std::variant<char, std::string>, FormName> COLL_NAMES = {
    {'(', ListForm},
    {'[', VectorForm},
    {'{', MapForm},
    {"#{", SetForm},
};

std::map<FormName, char> END_DELIMITERS = {
    {ListForm, ')'},
    {VectorForm, ']'},
    {MapForm, '}'},
    {SetForm, '}'}
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

template <class T>
std::vector<T> listToVector(const std::list<T> list) {
    std::vector<T> v {
        std::make_move_iterator(std::begin(list)),
        std::make_move_iterator(std::end(list))
    };
    return v;
}

std::pair<Form, std::list<Token>::const_iterator> readColl(const std::list<Token> *tokens, std::list<Token>::const_iterator it, FormName formName) {
    char endDelimiter = END_DELIMITERS[formName];
    std::list<FormWrapper> forms;
    while (it != tokens->end()) {
        auto token = *it;
        if (token.type == SpecialChar) {
            char c = std::get<char>(token.value);
            if (c == endDelimiter) {
                switch (formName) {
                    case VectorForm:
                        return std::make_pair(listToVector<FormWrapper>(forms), ++it);
                    default:
                        return std::make_pair(forms, ++it);
                }
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

std::pair<Form, std::list<Token>::const_iterator> expandQuotedForm(const std::list<Token> *tokens, std::list<Token>::const_iterator it, Token token) {
    if (auto retOpt = readForm(tokens, it)) {
        auto ret = retOpt.value();
        std::list<FormWrapper> list {
            FormWrapper{token},
            ret.first
        };
        return std::make_pair(list, ret.second);
    } else {
        return std::make_pair(ReaderError{"EOF: Nothing found after quote", token}, tokens->end());
    }
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
                    return readColl(tokens, ++it, COLL_NAMES[s]);
                } else if (s == "~@") {
                    return expandQuotedForm(tokens, ++it, Token{EXPANDED_NAMES[s], Symbol, token.line, token.column});
                }
                break;
            }
        case SpecialChar:
            {
                char c = std::get<char>(token.value);
                switch (c) {
                    case '(':
                    case '[':
                    case '{':
                        return readColl(tokens, ++it, COLL_NAMES[c]);
                    case ')':
                    case ']':
                    case '}':
                        return std::make_pair(ReaderError{"Unmatched delimiter: " + std::string(1, c), token}, tokens->end());
                    case '\'':
                    case '`':
                    case '~':
                    case '@':
                        return expandQuotedForm(tokens, ++it, Token{EXPANDED_NAMES[c], Symbol, token.line, token.column});
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
        case Comment:
            return std::nullopt;
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
        case ReaderErrorForm:
            {
                auto error = std::get<ReaderError>(form);
                return "#ReaderError \"" + error.message + "\"";
            }
        case TokenForm:
            return prStr(std::get<Token>(form));
        case ListForm:
            return "(" + prStr<std::list>(std::get<std::list<FormWrapper> >(form)) + ")";
        case VectorForm:
            return "[" + prStr<std::vector>(std::get<std::vector<FormWrapper> >(form)) + "]";
        case MapForm:
            return "{" + prStr<std::list>(std::get<std::list<FormWrapper> >(form)) + "}";
        case SetForm:
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
