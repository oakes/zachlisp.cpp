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
    if (bv.is_null()) {
        return token::Token{std::string("nil"), token::type::SYMBOL, 0, 0};
    }

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

    return form::Special{"RuntimeError", "Value not recognized", token};
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
        case form::LIST:
            {
                auto list = std::get<std::list<form::FormWrapper>>(form);
                if (list.size() == 0) {
                    return form;
                } else {
                    auto evaled_front = eval_form(list.front().form, chai);
                    list.pop_front();

                    switch (evaled_front.index()) {
                        case evaled::FORM:
                            return form::Special{"RuntimeError", "Invalid function", std::nullopt};
                        case evaled::FN_PAIR:
                            {
                                auto pair = std::get<evaled::fn::Pair>(evaled_front);
                                auto fn = pair.first;
                                auto token = pair.second;

                                std::vector<chaiscript::Boxed_Value> bvs;
                                for (auto it = list.begin(); it != list.end(); ++it) {
                                    auto token = std::get<token::Token>((*it).form);
                                    bvs.push_back(eval_token(token, chai));
                                }

                                switch (fn.index()) {
                                    case evaled::fn::ZERO:
                                        {
                                            auto fn_zero = std::get<evaled::fn::Zero>(fn);
                                            return chai_to_form(fn_zero(), token, chai);
                                        }
                                    case evaled::fn::ONE:
                                        {
                                            auto fn_one = std::get<evaled::fn::One>(fn);
                                            return chai_to_form(fn_one(bvs[0]), token, chai);
                                        }
                                }
                            }
                    }
                }
            }
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
                    new_forms.push_back(form::Special{"Function", name, token});
                    break;
                }
        }
    }
    return new_forms;
}

}
