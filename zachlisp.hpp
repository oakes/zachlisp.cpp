#pragma once

#include <string>
#include <list>
#include <vector>
#include <map>
#include <unordered_set>
#include <variant>
#include <regex>

namespace zachlisp {

    // zachlisp::token
    namespace token {

        // zachlisp::token::type
        namespace type {

        enum Type {WHITESPACE, SPECIAL_CHARS, SPECIAL_CHAR, STRING, COMMENT, NUMBER, SYMBOL};

        }

        // zachlisp::token::value
        namespace value {

        using Value = std::variant<bool, char, long, double, std::string>;

        enum ValueIndex {BOOL, CHAR, LONG, DOUBLE, STRING};

        }

    const std::regex REGEX(
        "([\\s,]*)|"                   // type::WHITESPACE
        "(~@|#\\{)|"                   // type::SPECIAL_CHARS
        "([\\[\\]{}()\'`~^@])|"        // type::SPECIAL_CHAR
        "(\"(?:\\\\.|[^\\\\\"])*\"?)|" // type::STRING
        "(;.*)|"                       // type::COMMENT
        "(\\d+\\.?\\d*)|"              // type::NUMBER
        "([^\\s\\[\\]{}(\'\"`,;)]*)"   // type::SYMBOL
    );

    struct Token {
        value::Value value;
        type::Type type;
        int line;
        int column;

        Token(value::Value v, type::Type t, int l, int c) : value(v), type(t), line(l), column(c) {}
    };

    }

    // zachlisp::form
    namespace form {

    struct ReaderError {
        std::string message;
        std::optional<token::Token> token;

        ReaderError(std::string m, std::optional<token::Token> t) : message(m), token(t) {}
    };

    struct FormWrapper;

    using Form = std::variant<
        ReaderError,
        token::Token,
        std::list<FormWrapper>,
        std::vector<FormWrapper>,
        std::map<std::string, FormWrapper>,
        std::unordered_set<std::string>
    >;

    enum FormIndex {READER_ERROR, TOKEN, LIST, VECTOR, MAP, SET};

    struct FormWrapper {
         Form form;

         FormWrapper(Form f) : form(f) {}
    };

    }

std::map<std::variant<char, std::string>, std::string> EXPANDED_NAMES = {
    {'\'', "quote"},
    {'`', "quasiquote"},
    {'~', "unquote"},
    {'@', "deref"},
    {'^', "with-meta"},
    {"~@", "splice-unquote"}
};

std::map<std::variant<char, std::string>, form::FormIndex> COLL_NAMES = {
    {'(', form::LIST},
    {'[', form::VECTOR},
    {'{', form::MAP},
    {"#{", form::SET},
};

std::map<form::FormIndex, char> END_DELIMITERS = {
    {form::LIST, ')'},
    {form::VECTOR, ']'},
    {form::MAP, '}'},
    {form::SET, '}'}
};

token::value::Value parse(std::string value, token::type::Type type) {
    switch (type) {
        case token::type::SPECIAL_CHAR:
            return value[0];
        case token::type::NUMBER:
            if (value.find('.') == std::string::npos) {
                return std::stol(value);
            } else {
                return std::stod(value);
            }
        case token::type::SYMBOL:
            if (value == "true") {
                return true;
            } else if (value == "false") {
                return false;
            }
    }
    return value;
}

std::list<token::Token> tokenize(std::string input) {
    std::sregex_iterator begin(input.begin(), input.end(), token::REGEX);
    std::sregex_iterator end;

    std::list<token::Token> tokens;

    int line = 1;

    for (auto it = begin; it != end; ++it) {
        std::smatch match = *it;
        for(auto i = 1; i < match.size(); ++i){
           if (!match[i].str().empty()) {
               std::string valueStr = match.str();
               token::type::Type type = static_cast<token::type::Type>(i-1);
               token::value::Value value = parse(valueStr, type);
               int column = match.position() + 1;
               tokens.push_back(token::Token{value, type, line, column});
               line += std::count(valueStr.begin(), valueStr.end(), '\n');
               break;
           }
        }
    }

    return tokens;
}

std::pair<form::Form, std::list<token::Token>::const_iterator> readForm(const std::list<token::Token> *tokens, std::list<token::Token>::const_iterator it);
std::optional<std::pair<form::Form, std::list<token::Token>::const_iterator> > readUsefulForm(const std::list<token::Token> *tokens, std::list<token::Token>::const_iterator it);
std::optional<std::pair<token::Token, std::list<token::Token>::const_iterator> > readUsefulToken(const std::list<token::Token> *tokens, std::list<token::Token>::const_iterator it);
std::string prStr(form::Form form);

form::Form listToVector(const std::list<form::FormWrapper> list) {
    std::vector<form::FormWrapper> v {
        std::make_move_iterator(std::begin(list)),
        std::make_move_iterator(std::end(list))
    };
    return v;
}

form::Form listToMap(const std::list<form::FormWrapper> list) {
    std::map<std::string, form::FormWrapper> m;
    std::map<std::string, form::FormWrapper>::const_iterator mapIt = m.begin();
    std::list<form::FormWrapper>::const_iterator listIt = list.begin();
    while (listIt != list.end()) {
        auto key = *listIt;
        ++listIt;
        if (listIt == list.end()) {
            return form::ReaderError{"Map must contain even number of forms", std::nullopt};
        } else {
            auto val = *listIt;
            ++listIt;
            m.insert(mapIt, std::pair(prStr(key.form), val));
            ++mapIt;
        }
    }
    return m;
}

form::Form listToSet(const std::list<form::FormWrapper> list) {
    std::unordered_set<std::string> s;
    std::unordered_set<std::string>::const_iterator it = s.begin();
    for (auto item : list) {
        s.insert(it, prStr(item.form));
    }
    return s;
}

std::pair<form::Form, std::list<token::Token>::const_iterator> readColl(const std::list<token::Token> *tokens, std::list<token::Token>::const_iterator it, form::FormIndex formName) {
    char endDelimiter = END_DELIMITERS[formName];
    std::list<form::FormWrapper> forms;
    while (auto retOpt = readUsefulToken(tokens, it)) {
        auto ret = retOpt.value();
        auto token = ret.first;
        it = ret.second;
        if (token.type == token::type::SPECIAL_CHAR) {
            char c = std::get<char>(token.value);
            if (c == endDelimiter) {
                switch (formName) {
                    case form::VECTOR:
                        return std::make_pair(listToVector(forms), ++it);
                    case form::MAP:
                        return std::make_pair(listToMap(forms), ++it);
                    case form::SET:
                        return std::make_pair(listToSet(forms), ++it);
                    default:
                        return std::make_pair(forms, ++it);
                }
            } else {
                switch (c) {
                    case ')':
                    case ']':
                    case '}':
                        return std::make_pair(form::ReaderError{"Unmatched delimiter: " + std::string(1, c), token}, tokens->end());
                }
            }
        }
        auto ret2 = readForm(tokens, it);
        forms.push_back(form::FormWrapper{ret2.first});
        it = ret2.second;
    }
    return std::make_pair(form::ReaderError{"EOF: no " + std::string(1, endDelimiter) + " found", std::nullopt}, tokens->end());
}

std::pair<form::Form, std::list<token::Token>::const_iterator> expandQuotedForm(const std::list<token::Token> *tokens, std::list<token::Token>::const_iterator it, token::Token token) {
    if (auto retOpt = readUsefulForm(tokens, it)) {
        auto ret = retOpt.value();
        std::list<form::FormWrapper> list {
            form::FormWrapper{token},
            ret.first
        };
        return std::make_pair(list, ret.second);
    } else {
        return std::make_pair(form::ReaderError{"EOF: Nothing found after quote", token}, tokens->end());
    }
}

std::pair<form::Form, std::list<token::Token>::const_iterator> expandMetaQuotedForm(const std::list<token::Token> *tokens, std::list<token::Token>::const_iterator it, token::Token token) {
    if (auto retOpt = readUsefulForm(tokens, it)) {
        auto ret = retOpt.value();
        if (auto ret2Opt = readUsefulForm(tokens, ret.second)) {
            auto ret2 = ret2Opt.value();
            std::list<form::FormWrapper> list {
                form::FormWrapper{token},
                ret2.first,
                ret.first
            };
            return std::make_pair(list, ret2.second);
        } else {
            return std::make_pair(form::ReaderError{"EOF: Nothing found after metadata", token}, tokens->end());
        }
    } else {
        return std::make_pair(form::ReaderError{"EOF: Nothing found after ^", token}, tokens->end());
    }
}

std::pair<form::Form, std::list<token::Token>::const_iterator> readForm(const std::list<token::Token> *tokens, std::list<token::Token>::const_iterator it) {
    auto token = *it;
    switch (token.type) {
        case token::type::WHITESPACE:
            break;
        case token::type::SPECIAL_CHARS:
            {
                std::string s = std::get<std::string>(token.value);
                if (s == "#{") {
                    return readColl(tokens, ++it, COLL_NAMES[s]);
                } else if (s == "~@") {
                    return expandQuotedForm(tokens, ++it, token::Token{EXPANDED_NAMES[s], token::type::SYMBOL, token.line, token.column});
                }
                break;
            }
        case token::type::SPECIAL_CHAR:
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
                        return std::make_pair(form::ReaderError{"Unmatched delimiter: " + std::string(1, c), token}, tokens->end());
                    case '\'':
                    case '`':
                    case '~':
                    case '@':
                        return expandQuotedForm(tokens, ++it, token::Token{EXPANDED_NAMES[c], token::type::SYMBOL, token.line, token.column});
                    case '^':
                        return expandMetaQuotedForm(tokens, ++it, token::Token{EXPANDED_NAMES[c], token::type::SYMBOL, token.line, token.column});
                }
                break;
            }
        case token::type::STRING:
            {
                std::string s = std::get<std::string>(token.value);
                if (s.size() < 2 || s.back() != '"') {
                    return std::make_pair(form::ReaderError{"EOF: unbalanced quote", token}, tokens->end());
                } else {
                    token.value = s.substr(1, s.size() - 2);
                }
                break;
            }
        case token::type::COMMENT:
            break;
    }
    return std::make_pair(token, ++it);
}

std::optional<std::pair<token::Token, std::list<token::Token>::const_iterator> > readUsefulToken(const std::list<token::Token> *tokens, std::list<token::Token>::const_iterator it) {
    while (it != tokens->end()) {
        auto token = *it;
        switch (token.type) {
            case token::type::WHITESPACE:
            case token::type::COMMENT:
                ++it;
                break;
            default:
                return std::make_pair(token, it);
        }
    }
    return std::nullopt;
}

std::optional<std::pair<form::Form, std::list<token::Token>::const_iterator> > readUsefulForm(const std::list<token::Token> *tokens, std::list<token::Token>::const_iterator it) {
    if (auto retOpt = readUsefulToken(tokens, it)) {
        auto ret = retOpt.value();
        return readForm(tokens, ret.second);
    } else {
        return std::nullopt;
    }
}

std::list<form::Form> readForms(const std::list<token::Token> *tokens) {
    std::list<form::Form> forms;
    std::list<token::Token>::const_iterator it = tokens->begin();
    while (auto retOpt = readUsefulForm(tokens, it)) {
        auto ret = retOpt.value();
        forms.push_back(ret.first);
        it = ret.second;
    }
    return forms;
}

std::string prStr(token::Token token) {
    switch (token.value.index()) {
        case token::value::BOOL:
            return std::get<bool>(token.value) ? "true" : "false";
        case token::value::CHAR:
            return std::string(1, std::get<char>(token.value));
        case token::value::LONG:
            return std::to_string(std::get<long>(token.value));
        case token::value::DOUBLE:
            return std::to_string(std::get<double>(token.value));
        case token::value::STRING:
            {
                std::string s = std::get<std::string>(token.value);
                if (token.type == token::type::STRING) {
                    return "\"" + s + "\"";
                } else {
                    return s;
                }
            }
    }
    return "";
}

std::string prStr(form::FormWrapper formWrapper) {
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

std::string prStr(std::map<std::string, form::FormWrapper> map) {
    std::string s;
    for (auto item : map) {
        if (s.size() > 0) {
            s += " ";
        }
        s += item.first + " " + prStr(item.second.form);
    }
    return s;
}

std::string prStr(form::Form form) {
    switch (form.index()) {
        case form::READER_ERROR:
            {
                auto error = std::get<form::ReaderError>(form);
                return "#ReaderError \"" + error.message + "\"";
            }
        case form::TOKEN:
            return prStr(std::get<token::Token>(form));
        case form::LIST:
            return "(" + prStr<std::list<form::FormWrapper> >(std::get<std::list<form::FormWrapper> >(form)) + ")";
        case form::VECTOR:
            return "[" + prStr<std::vector<form::FormWrapper> >(std::get<std::vector<form::FormWrapper> >(form)) + "]";
        case form::MAP:
            return "{" + prStr(std::get<std::map<std::string, form::FormWrapper> >(form)) + "}";
        case form::SET:
            return "#{" + prStr<std::unordered_set<std::string> >(std::get<std::unordered_set<std::string> >(form)) + "}";
    }
    return "";
}

std::list<form::Form> READ(const std::string input) {
    auto tokens = tokenize(input);
    auto forms = readForms(&tokens);
    return forms;
}

std::list<form::Form> EVAL(const std::list<form::Form> forms) {
    return forms;
}

std::string PRINT(const std::list<form::Form> forms) {
    std::string s;
    for (auto form : forms) {
        s += prStr(form) + "\n";
    }
    return s;
}

std::string rep(const std::string input) {
    return PRINT(EVAL(READ(input)));
}

}
