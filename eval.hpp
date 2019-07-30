#pragma once

#include <functional>

#include "read.hpp"
#include "chaiscript/chaiscript.hpp"

namespace zachlisp {

    namespace evaled {

        namespace fn {

            enum Type {ZERO, ONE};

            using Zero = std::function<chaiscript::Boxed_Value()>;
            using One = std::function<chaiscript::Boxed_Value(chaiscript::Boxed_Value)>;

            using Fn = std::variant<Zero, One>;
            using Pair = std::pair<Fn, token::Token>;

        }

    enum Type {FORM, FN_PAIR};
    using Form = std::variant<form::Form, fn::Pair>;

    }

chaiscript::Boxed_Value eval_token(token::Token token, chaiscript::ChaiScript* chai) {
    switch (token.value.index()) {
        case token::value::BOOL:
            return chaiscript::Boxed_Value(std::get<bool>(token.value));
        case token::value::CHAR:
            return chaiscript::Boxed_Value(1, std::get<char>(token.value));
        case token::value::LONG:
            return chaiscript::Boxed_Value(std::get<long>(token.value));
        case token::value::DOUBLE:
            return chaiscript::Boxed_Value(std::get<double>(token.value));
        case token::value::STRING:
            {
                auto s = std::get<std::string>(token.value);
                if (token.type == token::type::SYMBOL) {
                    return chai->eval(s);
                } else {
                    return chaiscript::Boxed_Value(s);
                }
            }
    }
}

evaled::Form chai_to_form(chaiscript::Boxed_Value bv, token::Token token, chaiscript::ChaiScript* chai) {
    try {
        return token::Token{chai->boxed_cast<bool>(bv), token::type::SYMBOL, 0, 0};
    } catch (const chaiscript::exception::bad_boxed_cast &) {}

    try {
        return token::Token{chai->boxed_cast<char>(bv), token::type::STRING, 0, 0};
    } catch (const chaiscript::exception::bad_boxed_cast &) {}

    try {
        return token::Token{chai->boxed_cast<long>(bv), token::type::NUMBER, 0, 0};
    } catch (const chaiscript::exception::bad_boxed_cast &) {}

    try {
        return token::Token{chai->boxed_cast<double>(bv), token::type::NUMBER, 0, 0};
    } catch (const chaiscript::exception::bad_boxed_cast &) {}

    try {
        return token::Token{chai->boxed_cast<std::string>(bv), token::type::STRING, 0, 0};
    } catch (const chaiscript::exception::bad_boxed_cast &) {}

    try {
        return std::make_pair(chai->boxed_cast<evaled::fn::Zero>(bv), token);
    } catch (const chaiscript::exception::bad_boxed_cast &) {}

    try {
        return std::make_pair(chai->boxed_cast<evaled::fn::One>(bv), token);
    } catch (const chaiscript::exception::bad_boxed_cast &) {}
}

evaled::Form eval_form(form::Form form, chaiscript::ChaiScript* chai) {
    switch (form.index()) {
        case form::SPECIAL:
            return form;
        case form::TOKEN:
            {
                auto token = std::get<token::Token>(form);
                return chai_to_form(eval_token(token, chai), token, chai);
            }
        //case form::LIST:
        //case form::VECTOR:
        //case form::MAP:
        //case form::SET:
    }
    return form;
}

std::list<form::Form> eval(std::list<form::Form> forms, chaiscript::ChaiScript* chai) {
    std::list<form::Form> new_forms;
    for (auto form : forms) {
        auto evaled_form = eval_form(form, chai);
        switch (evaled_form.index()) {
            case evaled::Type::FORM:
                {
                    new_forms.push_back(std::get<form::Form>(evaled_form));
                    break;
                }
            case evaled::Type::FN_PAIR:
                {
                    auto token = std::get<evaled::fn::Pair>(evaled_form).second;
                    auto name = std::get<std::string>(token.value);
                    new_forms.push_back(form::Special{"Function", name, std::nullopt});
                    break;
                }
        }
    }
    return new_forms;
}

}
