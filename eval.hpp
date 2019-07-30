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

        }

    enum Type {SPECIAL, CHAI};
    using Maybe = std::variant<form::Special, chaiscript::Boxed_Value>;

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

evaled::Maybe form_to_chai(form::Form form, chaiscript::ChaiScript* chai) {
    switch (form.index()) {
        case form::SPECIAL:
            return std::get<form::Special>(form);
        case form::TOKEN:
            {
                auto token = std::get<token::Token>(form);
                return eval_token(token, chai);
            }
        case form::LIST:
            {
                auto list = std::get<std::list<form::FormWrapper>>(form);
                if (list.size() == 0) {
                    return form::Special{"RuntimeError", "Empty list", std::nullopt};
                } else {
                    auto ret = form_to_chai(list.front().form, chai);
                    list.pop_front();

                    switch (ret.index()) {
                        case evaled::SPECIAL:
                            return ret;
                        case evaled::CHAI:
                            {
                                auto fn = std::get<chaiscript::Boxed_Value>(ret);

                                std::vector<chaiscript::Boxed_Value> args;
                                for (auto it = list.begin(); it != list.end(); ++it) {
                                    auto ret = form_to_chai((*it).form, chai);
                                    switch (ret.index()) {
                                        case evaled::SPECIAL:
                                            return ret;
                                        case evaled::CHAI:
                                            {
                                                args.push_back(std::get<chaiscript::Boxed_Value>(ret));
                                                break;
                                            }
                                    }
                                }

                                switch (args.size()) {
                                    case evaled::fn::ZERO:
                                        {
                                            auto fn_zero = chai->boxed_cast<evaled::fn::Zero>(fn);
                                            return fn_zero();
                                        }
                                    case evaled::fn::ONE:
                                        {
                                            auto fn_one = chai->boxed_cast<evaled::fn::One>(fn);
                                            return fn_one(args[0]);
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
    return form::Special{"RuntimeError", "Form not recognized", std::nullopt};
}

form::Form chai_to_form(chaiscript::Boxed_Value bv, chaiscript::ChaiScript* chai) {
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
        chai->boxed_cast<evaled::fn::Zero>(bv);
        return form::Special{"Object", "function", std::nullopt};
    } catch (const chaiscript::exception::bad_boxed_cast &) {}

    try {
        chai->boxed_cast<evaled::fn::One>(bv);
        return form::Special{"Object", "function", std::nullopt};
    } catch (const chaiscript::exception::bad_boxed_cast &) {}

    return form::Special{"RuntimeError", "Value not recognized", std::nullopt};
}

std::list<form::Form> eval(std::list<form::Form> forms, chaiscript::ChaiScript* chai) {
    std::list<form::Form> new_forms;
    for (auto form : forms) {
        auto evaled_form = form_to_chai(form, chai);
        switch (evaled_form.index()) {
            case evaled::SPECIAL:
                {
                    new_forms.push_back(std::get<form::Special>(evaled_form));
                    break;
                }
            case evaled::CHAI:
                {
                    auto ret = chai_to_form(std::get<chaiscript::Boxed_Value>(evaled_form), chai);
                    new_forms.push_back(ret);
                    break;
                }
        }
    }
    return new_forms;
}

}
