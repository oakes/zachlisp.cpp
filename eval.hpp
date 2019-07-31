#pragma once

#include <functional>

#include "read.hpp"
#include "print.hpp"
#include "chaiscript/chaiscript.hpp"

namespace zachlisp {

    namespace evaled {

        namespace fn {

        enum Type {ZERO, ONE, TWO, THREE, FOUR, FIVE, SIX};

        using Zero = std::function<chaiscript::Boxed_Value()>;
        using One = std::function<chaiscript::Boxed_Value(chaiscript::Boxed_Value)>;
        using Two = std::function<chaiscript::Boxed_Value(chaiscript::Boxed_Value, chaiscript::Boxed_Value)>;
        using Three = std::function<chaiscript::Boxed_Value(chaiscript::Boxed_Value, chaiscript::Boxed_Value, chaiscript::Boxed_Value)>;
        using Four = std::function<chaiscript::Boxed_Value(chaiscript::Boxed_Value, chaiscript::Boxed_Value, chaiscript::Boxed_Value, chaiscript::Boxed_Value)>;
        using Five = std::function<chaiscript::Boxed_Value(chaiscript::Boxed_Value, chaiscript::Boxed_Value, chaiscript::Boxed_Value, chaiscript::Boxed_Value, chaiscript::Boxed_Value)>;
        using Six = std::function<chaiscript::Boxed_Value(chaiscript::Boxed_Value, chaiscript::Boxed_Value, chaiscript::Boxed_Value, chaiscript::Boxed_Value, chaiscript::Boxed_Value, chaiscript::Boxed_Value)>;

        }

    enum Type {SPECIAL, CHAI};
    using Maybe = std::variant<form::Special, chaiscript::Boxed_Value>;

    }

const std::unordered_set<char> OPERATORS = {'+', '-', '*', '/'};

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
        default: //case token::value::STRING:
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

form::Form chai_to_form(chaiscript::Boxed_Value bv, chaiscript::ChaiScript* chai);

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
                    auto first_form = list.front().form;
                    list.pop_front();

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

                    std::string fn_name = "";
                    if (first_form.index() == form::TOKEN) {
                        auto token = std::get<token::Token>(first_form);
                        if (token.type == token::type::SYMBOL) {
                            fn_name = std::get<std::string>(token.value);
                        }
                    }

                    if (fn_name.size() == 1 && OPERATORS.find(fn_name.at(0)) != OPERATORS.end()) {
                        if (args.size() >= 2) {
                            auto fn = chai->eval<evaled::fn::Two>("`" + fn_name + "`");
                            auto ret = fn(args[0], args[1]);
                            for (int i = 2; i < args.size(); i++) {
                                ret = fn(ret, args[i]);
                            }
                            return ret;
                        }
                    } else {
                        auto ret = form_to_chai(first_form, chai);
                        switch (ret.index()) {
                            case evaled::SPECIAL:
                                return ret;
                            case evaled::CHAI:
                                {
                                    auto chai_fn = std::get<chaiscript::Boxed_Value>(ret);

                                    switch (args.size()) {
                                        case evaled::fn::ZERO:
                                            {
                                                auto fn = chai->boxed_cast<evaled::fn::Zero>(chai_fn);
                                                return fn();
                                            }
                                        case evaled::fn::ONE:
                                            {
                                                auto fn = chai->boxed_cast<evaled::fn::One>(chai_fn);
                                                return fn(args[0]);
                                            }
                                        case evaled::fn::TWO:
                                            {
                                                auto fn = chai->boxed_cast<evaled::fn::Two>(chai_fn);
                                                return fn(args[0], args[1]);
                                            }
                                        case evaled::fn::THREE:
                                            {
                                                auto fn = chai->boxed_cast<evaled::fn::Three>(chai_fn);
                                                return fn(args[0], args[1], args[2]);
                                            }
                                        case evaled::fn::FOUR:
                                            {
                                                auto fn = chai->boxed_cast<evaled::fn::Four>(chai_fn);
                                                return fn(args[0], args[1], args[2], args[3]);
                                            }
                                        case evaled::fn::FIVE:
                                            {
                                                auto fn = chai->boxed_cast<evaled::fn::Five>(chai_fn);
                                                return fn(args[0], args[1], args[2], args[3], args[4]);
                                            }
                                        case evaled::fn::SIX:
                                            {
                                                auto fn = chai->boxed_cast<evaled::fn::Six>(chai_fn);
                                                return fn(args[0], args[1], args[2], args[3], args[4], args[5]);
                                            }
                                    }
                                }
                        }
                    }

                    return form::Special{"RuntimeError", "Invalid number of arguments function " + fn_name, std::nullopt};
                }
            }
        case form::VECTOR:
            {
                auto vec = std::get<std::vector<form::FormWrapper>>(form);
                auto new_vec = std::vector<chaiscript::Boxed_Value>();

                for (auto it = vec.begin(); it != vec.end(); ++it) {
                    auto ret = form_to_chai((*it).form, chai);
                    switch (ret.index()) {
                        case evaled::SPECIAL:
                            return ret;
                        case evaled::CHAI:
                            {
                                new_vec.push_back(std::get<chaiscript::Boxed_Value>(ret));
                                break;
                            }
                    }
                }

                return chaiscript::Boxed_Value(new_vec);
            }
        case form::MAP:
            {
                auto map = *std::get<std::shared_ptr<form::FormWrapperMap>>(form);
                auto new_map = std::map<std::string, chaiscript::Boxed_Value>();
                auto new_map_it = new_map.begin();

                for (auto it = map.begin(); it != map.end(); ++it) {
                    auto key = form_to_chai((*it).first.form, chai);
                    switch (key.index()) {
                        case evaled::SPECIAL:
                            return key;
                    }
                    auto val = form_to_chai((*it).second.form, chai);
                    switch (val.index()) {
                        case evaled::SPECIAL:
                            return key;
                    }
                    auto new_key = std::get<chaiscript::Boxed_Value>(key);
                    auto new_val = std::get<chaiscript::Boxed_Value>(val);
                    auto stringified_key = pr_str(chai_to_form(new_key, chai));
                    new_map.insert(new_map_it, std::pair(stringified_key, new_val));
                }

                return chaiscript::Boxed_Value(new_map);
            }
        case form::SET:
            {
                auto set = *std::get<std::shared_ptr<form::FormWrapperSet>>(form);
                auto new_set = std::map<std::string, chaiscript::Boxed_Value>();
                auto new_set_it = new_set.begin();

                for (auto it = set.begin(); it != set.end(); ++it) {
                    auto key = form_to_chai((*it).form, chai);
                    switch (key.index()) {
                        case evaled::SPECIAL:
                            return key;
                    }
                    auto new_key = std::get<chaiscript::Boxed_Value>(key);
                    auto stringified_key = pr_str(chai_to_form(new_key, chai));
                    new_set.insert(new_set_it, std::pair(stringified_key, new_key));
                }

                return chaiscript::Boxed_Value(new_set);
            }
    }
    return form::Special{"RuntimeError", "Form not recognized", std::nullopt};
}

form::Form chai_to_form(chaiscript::Boxed_Value bv, chaiscript::ChaiScript* chai) {
    if (bv.is_null()) {
        return token::Token{std::string("nil"), token::type::SYMBOL, 0, 0};
    }

    try {
        auto vec = chai->boxed_cast<std::vector<chaiscript::Boxed_Value>>(bv);
        auto new_vec = std::vector<form::FormWrapper>();

        for (auto it = vec.begin(); it != vec.end(); ++it) {
            new_vec.push_back(form::FormWrapper{chai_to_form(*it, chai)});
        }

        return new_vec;
    } catch (const chaiscript::exception::bad_boxed_cast &) {}

    try {
        auto map = chai->boxed_cast<std::map<std::string, chaiscript::Boxed_Value>>(bv);
        auto new_map = std::make_shared<form::FormWrapperMap>(form::FormWrapperMap{});
        auto new_set = std::make_shared<form::FormWrapperSet>(form::FormWrapperSet{});
        auto new_map_it = new_map->begin();
        auto new_set_it = new_set->begin();

        for (auto it = map.begin(); it != map.end(); ++it) {
            auto key_str = (*it).first;
            auto forms = read(key_str);
            if (forms.size() != 1) {
                return form::Special{"RuntimeError", "Failed to parse " + std::string(key_str), std::nullopt};
            }
            auto key = form::FormWrapper{forms.front()};
            auto val = form::FormWrapper{chai_to_form((*it).second, chai)};
            new_map->insert(new_map_it, std::pair(key, val));
            if (key == val) {
                new_set->insert(new_set_it, val);
            }
        }

        if (new_map->size() == new_set->size()) {
            return new_set;
        } else {
            return new_map;
        }
    } catch (const chaiscript::exception::bad_boxed_cast &) {}

    try {
        return token::Token{chai->boxed_cast<bool>(bv), token::type::SYMBOL, 0, 0};
    } catch (const chaiscript::exception::bad_boxed_cast &) {}

    try {
        return token::Token{chai->boxed_cast<char>(bv), token::type::STRING, 0, 0};
    } catch (const chaiscript::exception::bad_boxed_cast &) {}

    try {
        return token::Token{chaiscript::Boxed_Number(bv).get_as<long>(), token::type::NUMBER, 0, 0};
    } catch (const chaiscript::exception::bad_boxed_cast &) {}

    try {
        return token::Token{chaiscript::Boxed_Number(bv).get_as<double>(), token::type::NUMBER, 0, 0};
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
        try {
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
        } catch (const chaiscript::exception::eval_error &e) {
            new_forms.push_back(form::Special{"RuntimeError", e.what(), std::nullopt});
        } catch (const chaiscript::exception::bad_boxed_cast &e) {
            new_forms.push_back(form::Special{"RuntimeError", e.what(), std::nullopt});
        } catch (const chaiscript::detail::exception::bad_any_cast &e) {
            new_forms.push_back(form::Special{"RuntimeError", e.what(), std::nullopt});
        }
    }
    return new_forms;
}

}
