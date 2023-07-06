#include <algorithm>
#include <iostream>
#include <fstream>

#include "Array.hpp"


namespace Interpreter {

    namespace Array {

        auto lenght_args = std::make_shared<Parser::Symbol>("array");
        Reference lenght(FunctionContext & context) {
            try {
                auto array = context["array"].to_data(context).get<Object*>();

                return Reference(Data((long) array->array.size()));
            } catch (Data::BadAccess & e) {
                throw FunctionArgumentsError();
            }
        }

        auto get_capacity_args = std::make_shared<Parser::Symbol>("array");
        Reference get_capacity(FunctionContext & context) {
            auto array = context["array"].to_data(context).get<Object*>();

            try {
                return Data((long) array->array.capacity());
            } catch (Data::BadAccess & e) {
                throw FunctionArgumentsError();
            }
        }

        auto set_capacity_args = std::make_shared<Parser::Tuple>(Parser::Tuple({
            std::make_shared<Parser::Symbol>("array"),
            std::make_shared<Parser::Symbol>("capacity")
        }));
        Reference set_capacity(FunctionContext & context) {
            try {
                auto array = context["array"].to_data(context).get<Object*>();
                auto capacity = context["capacity"].to_data(context).get<long>();

                array->array.reserve(capacity);
                return Data();
            } catch (Data::BadAccess & e) {
                throw FunctionArgumentsError();
            }
        }

        auto get_args = std::make_shared<Parser::Tuple>(Parser::Tuple({
            std::make_shared<Parser::Symbol>("array"),
            std::make_shared<Parser::Symbol>("i")
        }));
        Reference get(FunctionContext & context) {
            try {
                auto array = context["array"].to_data(context).get<Object*>();
                auto i = context["i"].to_data(context).get<long>();

                if (i >= 0 && i < (long) array->array.size())
                    return ArrayReference{*array, (size_t) i};
                else throw FunctionArgumentsError();
            } catch (Data::BadAccess & e) {
                throw FunctionArgumentsError();
            }
        }

        auto add_args = std::make_shared<Parser::Tuple>(Parser::Tuple({
            std::make_shared<Parser::Symbol>("array"),
            std::make_shared<Parser::Symbol>("element")
        }));
        Reference add(FunctionContext & context) {
            try {
                auto array = context["array"].to_data(context).get<Object*>();
                auto element = context["element"].to_data(context);

                array->array.push_back(element);
                return ArrayReference{*array, (size_t) array->array.size()-1};
            } catch (Data::BadAccess & e) {
                throw FunctionArgumentsError();
            }
        }

        auto remove_args = std::make_shared<Parser::Symbol>("array");
        Reference remove(FunctionContext & context) {
            try {
                auto array = context["array"].to_data(context).get<Object*>();

                Data d = array->array.back();
                array->array.pop_back();
                return Data(d);
            } catch (Data::BadAccess & e) {
                throw FunctionArgumentsError();
            }
        }

        void init(GlobalContext & context) {
            auto array = context["array"].to_data(context).get<Object*>();
            (*array)["lenght"].to_data(context).get<Object*>()->functions.push_front(SystemFunction{lenght_args, lenght});
            (*array)["get_capacity"].to_data(context).get<Object*>()->functions.push_front(SystemFunction{get_capacity_args, get_capacity});
            (*array)["set_capacity"].to_data(context).get<Object*>()->functions.push_front(SystemFunction{set_capacity_args, set_capacity});
            (*array)["get"].to_data(context).get<Object*>()->functions.push_front(SystemFunction{get_args, get});
            (*array)["add"].to_data(context).get<Object*>()->functions.push_front(SystemFunction{add_args, add});
            (*array)["remove"].to_data(context).get<Object*>()->functions.push_front(SystemFunction{remove_args, remove});
        }

    }

}
