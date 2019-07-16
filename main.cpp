#include <iostream>
#include <string>
#include <list>
#include <vector>
#include <map>
#include <unordered_set>
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
    std::map<std::string, FormWrapper>,
    std::unordered_set<std::string>
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
    {'^', "with-meta"},
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

std::pair<Form, std::list<Token>::const_iterator> readForm(const std::list<Token> *tokens, std::list<Token>::const_iterator it);
std::optional<std::pair<Form, std::list<Token>::const_iterator> > readUsefulForm(const std::list<Token> *tokens, std::list<Token>::const_iterator it);
std::optional<std::pair<Token, std::list<Token>::const_iterator> > readUsefulToken(const std::list<Token> *tokens, std::list<Token>::const_iterator it);
std::string prStr(Form form);

Form listToVector(const std::list<FormWrapper> list) {
    std::vector<FormWrapper> v {
        std::make_move_iterator(std::begin(list)),
        std::make_move_iterator(std::end(list))
    };
    return v;
}

Form listToMap(const std::list<FormWrapper> list) {
    std::map<std::string, FormWrapper> m;
    std::map<std::string, FormWrapper>::const_iterator mapIt = m.begin();
    std::list<FormWrapper>::const_iterator listIt = list.begin();
    while (listIt != list.end()) {
        auto key = *listIt;
        ++listIt;
        if (listIt == list.end()) {
            return ReaderError{"Map must contain even number of forms", std::nullopt};
        } else {
            auto val = *listIt;
            ++listIt;
            m.insert(mapIt, std::pair(prStr(key.form), val));
            ++mapIt;
        }
    }
    return m;
}

Form listToSet(const std::list<FormWrapper> list) {
    std::unordered_set<std::string> s;
    std::unordered_set<std::string>::const_iterator it = s.begin();
    for (auto item : list) {
        s.insert(it, prStr(item.form));
    }
    return s;
}

std::pair<Form, std::list<Token>::const_iterator> readColl(const std::list<Token> *tokens, std::list<Token>::const_iterator it, FormName formName) {
    char endDelimiter = END_DELIMITERS[formName];
    std::list<FormWrapper> forms;
    while (auto retOpt = readUsefulToken(tokens, it)) {
        auto ret = retOpt.value();
        auto token = ret.first;
        it = ret.second;
        if (token.type == SpecialChar) {
            char c = std::get<char>(token.value);
            if (c == endDelimiter) {
                switch (formName) {
                    case VectorForm:
                        return std::make_pair(listToVector(forms), ++it);
                    case MapForm:
                        return std::make_pair(listToMap(forms), ++it);
                    case SetForm:
                        return std::make_pair(listToSet(forms), ++it);
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
        auto ret2 = readForm(tokens, it);
        forms.push_back(FormWrapper{ret2.first});
        it = ret2.second;
    }
    return std::make_pair(ReaderError{"EOF: no " + std::string(1, endDelimiter) + " found", std::nullopt}, tokens->end());
}

std::pair<Form, std::list<Token>::const_iterator> expandQuotedForm(const std::list<Token> *tokens, std::list<Token>::const_iterator it, Token token) {
    if (auto retOpt = readUsefulForm(tokens, it)) {
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

std::pair<Form, std::list<Token>::const_iterator> expandMetaQuotedForm(const std::list<Token> *tokens, std::list<Token>::const_iterator it, Token token) {
    if (auto retOpt = readUsefulForm(tokens, it)) {
        auto ret = retOpt.value();
        if (auto ret2Opt = readUsefulForm(tokens, ret.second)) {
            auto ret2 = ret2Opt.value();
            std::list<FormWrapper> list {
                FormWrapper{token},
                ret2.first,
                ret.first
            };
            return std::make_pair(list, ret2.second);
        } else {
            return std::make_pair(ReaderError{"EOF: Nothing found after metadata", token}, tokens->end());
        }
    } else {
        return std::make_pair(ReaderError{"EOF: Nothing found after ^", token}, tokens->end());
    }
}

std::pair<Form, std::list<Token>::const_iterator> readForm(const std::list<Token> *tokens, std::list<Token>::const_iterator it) {
    auto token = *it;
    switch (token.type) {
        case Whitespace:
            break;
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
                    case '^':
                        return expandMetaQuotedForm(tokens, ++it, Token{EXPANDED_NAMES[c], Symbol, token.line, token.column});
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
            break;
    }
    return std::make_pair(token, ++it);
}

std::optional<std::pair<Token, std::list<Token>::const_iterator> > readUsefulToken(const std::list<Token> *tokens, std::list<Token>::const_iterator it) {
    while (it != tokens->end()) {
        auto token = *it;
        switch (token.type) {
            case Whitespace:
            case Comment:
                ++it;
                break;
            default:
                return std::make_pair(token, it);
        }
    }
    return std::nullopt;
}

std::optional<std::pair<Form, std::list<Token>::const_iterator> > readUsefulForm(const std::list<Token> *tokens, std::list<Token>::const_iterator it) {
    if (auto retOpt = readUsefulToken(tokens, it)) {
        auto ret = retOpt.value();
        return readForm(tokens, ret.second);
    } else {
        return std::nullopt;
    }
}

std::list<Form> readForms(const std::list<Token> *tokens) {
    std::list<Form> forms;
    std::list<Token>::const_iterator it = tokens->begin();
    while (auto retOpt = readUsefulForm(tokens, it)) {
        auto ret = retOpt.value();
        forms.push_back(ret.first);
        it = ret.second;
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

std::string prStr(FormWrapper formWrapper) {
    return prStr(formWrapper.form);
}

std::string prStr(std::string s) {
    return s;
}

template <class T>
std::string prStr(T list) {
    std::string s;
    for (auto item : list) {
        if (s.size() > 0) {
            s += " ";
        }
        s += prStr(item);
    }
    return s;
}

std::string prStr(std::map<std::string, FormWrapper> map) {
    std::string s;
    for (auto item : map) {
        if (s.size() > 0) {
            s += " ";
        }
        s += item.first + " " + prStr(item.second.form);
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
            return "(" + prStr<std::list<FormWrapper> >(std::get<std::list<FormWrapper> >(form)) + ")";
        case VectorForm:
            return "[" + prStr<std::vector<FormWrapper> >(std::get<std::vector<FormWrapper> >(form)) + "]";
        case MapForm:
            return "{" + prStr(std::get<std::map<std::string, FormWrapper> >(form)) + "}";
        case SetForm:
            return "#{" + prStr<std::unordered_set<std::string> >(std::get<std::unordered_set<std::string> >(form)) + "}";
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
