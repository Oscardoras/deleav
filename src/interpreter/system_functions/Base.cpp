#include <algorithm>
#include <exception>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <sstream>

#include "Base.hpp"

#include "../../parser/Standard.hpp"


namespace Interpreter {

    namespace Base {

        auto getter_args = std::make_shared<Parser::Symbol>("var");
        Reference getter(FunctionContext & context) {
            return std::visit([](auto const& arg) -> Data & {
                return arg;
            }, context["var"]) = context.new_object();
        }

        Reference assignation(Context & context, Reference const& var, Data const& d) {
            if (std::get_if<Data>(&var)) return d;
            else if (auto symbol_reference = std::get_if<SymbolReference>(&var)) symbol_reference->get() = d;
            else if (auto property_reference = std::get_if<PropertyReference>(&var)) static_cast<Data &>(*property_reference) = d;
            else if (auto array_reference = std::get_if<ArrayReference>(&var)) static_cast<Data &>(*array_reference) = d;
            else if (auto tuple_reference = std::get_if<TupleReference>(&var)) {
                try {
                    auto object = d.get<Object*>();
                    if (tuple_reference->size() == object->array.size()) {
                        for (size_t i = 0; i < tuple_reference->size(); i++)
                            assignation(context, (*tuple_reference)[i], object->array[i]);
                    } else throw Interpreter::FunctionArgumentsError();
                } catch (Data::BadAccess const& e) {
                    throw Interpreter::FunctionArgumentsError();
                }
            }
            return var;
        }

        auto setter_args = std::make_shared<Parser::Tuple>(Parser::Tuple({
            std::make_shared<Parser::FunctionCall>(
                std::make_shared<Parser::Symbol>("var"),
                std::make_shared<Parser::Tuple>()
            ),
            std::make_shared<Parser::Symbol>("data")
        }));
        Reference setter(FunctionContext & context) {
            auto var = Interpreter::call_function(context.get_parent(), context.expression, context["var"].to_data(context).get<Object*>()->functions, std::make_shared<Parser::Tuple>());
            auto data = context["data"].to_data(context);

            return assignation(context, var, data);
        }


        auto separator_args = std::make_shared<Parser::Tuple>(Parser::Tuple({
            std::make_shared<Parser::Symbol>("a"),
            std::make_shared<Parser::Symbol>("b")
        }));
        Reference separator(FunctionContext & context) {
            return Reference(context["b"]);
        }

        auto if_statement_args = std::make_shared<Parser::FunctionCall>(
            std::make_shared<Parser::Symbol>("function"),
            std::make_shared<Parser::Tuple>()
        );
        Reference if_statement(FunctionContext & context) {
            try {
                auto function = std::get<CustomFunction>(context["function"].to_data(context).get<Object*>()->functions.front())->body;

                if (auto tuple = std::dynamic_pointer_cast<Parser::Tuple>(function)) {
                    if (tuple->objects.size() >= 2) {
                        auto & parent = context.get_parent();

                        if (Interpreter::execute(parent, tuple->objects[0]).to_data(context).get<bool>())
                            return Interpreter::execute(parent, tuple->objects[1]);
                        else {
                            unsigned long i = 2;
                            while (i < tuple->objects.size()) {
                                auto else_s = Interpreter::execute(parent, tuple->objects[i]).to_data(context);
                                if (else_s == context["else"].to_data(context) && i+1 < tuple->objects.size()) {
                                    auto s = Interpreter::execute(parent, tuple->objects[i+1]).to_data(context);
                                    if (s == context["if"].to_data(context) && i+3 < tuple->objects.size()) {
                                        if (Interpreter::execute(parent, tuple->objects[i+2]).to_data(context).get<bool>())
                                            return Interpreter::execute(parent, tuple->objects[i+3]);
                                        else i += 4;
                                    } else return s;
                                } else throw Interpreter::FunctionArgumentsError();
                            }
                            return Reference();
                        }
                    } else throw Interpreter::FunctionArgumentsError();
                } else throw Interpreter::FunctionArgumentsError();
            } catch (Data::BadAccess & e) {
                throw FunctionArgumentsError();
            }
        }

        auto while_statement_args = std::make_shared<Parser::Tuple>(Parser::Tuple({
            std::make_shared<Parser::FunctionCall>(
                std::make_shared<Parser::Symbol>("condition"),
                std::make_shared<Parser::Tuple>()
            ),
            std::make_shared<Parser::FunctionCall>(
                std::make_shared<Parser::Symbol>("block"),
                std::make_shared<Parser::Tuple>()
            )
        }));
        Reference while_statement(FunctionContext & context) {
            try {
                auto & parent = context.get_parent();
                Reference result;
                bool resulted = false;

                auto condition = context["condition"].to_data(context).get<Object*>();
                auto block = context["block"].to_data(context).get<Object*>();
                while (true) {
                    auto c = Interpreter::call_function(parent, parent.expression, condition->functions, std::make_shared<Parser::Tuple>()).to_data(context).get<bool>();
                    if (c) {
                        result = Interpreter::call_function(parent, parent.expression, block->functions, std::make_shared<Parser::Tuple>());
                        resulted = true;
                    } else break;
                }

                if (resulted)
                    return result;
                else
                    return Reference(Data(context.new_object()));
            } catch (Data::BadAccess & e) {
                throw FunctionArgumentsError();
            }
        }

        auto for_statement_args = std::make_shared<Parser::Tuple>(Parser::Tuple({
            std::make_shared<Parser::Symbol>("variable"),
            std::make_shared<Parser::Symbol>("from"),
            std::make_shared<Parser::Symbol>("begin"),
            std::make_shared<Parser::Symbol>("to"),
            std::make_shared<Parser::Symbol>("end"),
            std::make_shared<Parser::FunctionCall>(
                std::make_shared<Parser::Symbol>("block"),
                std::make_shared<Parser::Tuple>()
            )
        }));
        Reference for_statement(FunctionContext & context) {
            try {
                auto variable = context["variable"].to_data(context);
                auto begin = context["begin"].to_data(context).get<long>();
                auto end = context["end"].to_data(context).get<long>();
                auto block = context["block"].to_data(context).get<Object*>();

                for (long i = begin; i < end; i++) {
                    Interpreter::set(context, variable, i);
                    Interpreter::call_function(context.get_parent(), context.expression, block->functions, std::make_shared<Parser::Tuple>());
                }
                return Reference();
            } catch (Data::BadAccess & e) {
                throw FunctionArgumentsError();
            }
        }

        auto for_step_statement_args = std::make_shared<Parser::Tuple>(Parser::Tuple({
            std::make_shared<Parser::Symbol>("variable"),
            std::make_shared<Parser::Symbol>("from"),
            std::make_shared<Parser::Symbol>("begin"),
            std::make_shared<Parser::Symbol>("to"),
            std::make_shared<Parser::Symbol>("end"),
            std::make_shared<Parser::Symbol>("step"),
            std::make_shared<Parser::Symbol>("s"),
            std::make_shared<Parser::FunctionCall>(
                std::make_shared<Parser::Symbol>("block"),
                std::make_shared<Parser::Tuple>()
            )
        }));
        Reference for_step_statement(FunctionContext & context) {
            try {
                auto & parent = context.get_parent();

                auto variable = context["variable"];
                auto begin = context["begin"].to_data(context).get<long>();
                auto end = context["end"].to_data(context).get<long>();
                auto s = context["s"].to_data(context).get<long>();
                auto block = context["block"].to_data(context).get<Object*>();

                if (s > 0) {
                    for (long i = begin; i < end; i += s) {
                        Interpreter::set(context, variable, i);
                        Interpreter::call_function(parent, parent.expression, block->functions, std::make_shared<Parser::Tuple>());
                    }
                } else if (s < 0) {
                    for (long i = begin; i > end; i += s) {
                        Interpreter::set(context, variable, i);
                        Interpreter::call_function(parent, parent.expression, block->functions, std::make_shared<Parser::Tuple>());
                    }
                } else throw Interpreter::FunctionArgumentsError();
                return Reference(context.new_object());
            } catch (Data::BadAccess & e) {
                throw FunctionArgumentsError();
            }
        }

        auto try_statement_args = std::make_shared<Parser::Tuple>(Parser::Tuple({
            std::make_shared<Parser::FunctionCall>(
                std::make_shared<Parser::Symbol>("try_block"),
                std::make_shared<Parser::Tuple>()
            ),
            std::make_shared<Parser::Symbol>("catch"),
            std::make_shared<Parser::Symbol>("catch_function")
        }));
        Reference try_statement(FunctionContext & context) {
            auto try_block = context["try_block"].to_data(context).get<Object*>();
            auto catch_function = context["catch_function"].to_data(context).get<Object*>();

            try {
                return Interpreter::call_function(context.get_parent(), context.expression, try_block->functions, std::make_shared<Parser::Tuple>());
            } catch (Exception & ex) {
                try {
                    return Interpreter::call_function(context.get_parent(), context.expression, catch_function->functions, ex.reference);
                } catch (Interpreter::Exception & e) {
                    throw ex;
                }
            }
        }

        auto throw_statement_args = std::make_shared<Parser::Symbol>("throw_expression");
        Reference throw_statement(FunctionContext & context) {
            throw Exception(
                context,
                context["throw_expression"],
                context.get_parent().expression
            );
        }

        std::string get_canonical_path(FunctionContext & context) {
            if (auto position = std::dynamic_pointer_cast<Parser::Standard::TextPosition>(context.expression->position)) {
                try {
                    auto path = context["path"].to_data(context).get<Object*>()->to_string();
                    auto system_position = position->path;

                    if (path[0] != '/')
                        path = system_position.substr(0, system_position.find_last_of("/")+1) + path;
                    std::filesystem::path p(path);
                    return std::filesystem::canonical(p).string();
                } catch (std::exception & e) {
                    throw Interpreter::FunctionArgumentsError();
                }
            } else throw Interpreter::FunctionArgumentsError();
        }

        auto path_args = std::make_shared<Parser::Symbol>("path");
        Reference import(FunctionContext & context) {
            auto & global = context.get_global();
            auto root = context.expression->get_root();
            std::string path = get_canonical_path(context);

            auto it = global.sources.find(path);
            if (it == global.sources.end()) {
                std::string code = (std::ostringstream{} << std::ifstream(path).rdbuf()).str();

                try {
                    auto expression = Parser::Standard(code, path).get_tree();
                    global.sources[path] = expression;

                    {
                        auto symbols = GlobalContext(nullptr).get_symbols();
                        expression->compute_symbols(symbols);
                    }

                    {
                        auto symbols = root->symbols;
                        symbols.insert(expression->symbols.begin(), expression->symbols.end());
                        root->compute_symbols(symbols);
                    }

                    return Interpreter::execute(global, expression);
                } catch (Parser::Standard::IncompleteCode & e) {
                    throw Exception(context, "incomplete code, you must finish the last expression in file \"" + path + "\"", context.get_global()["ParserException"].to_data(context), context.expression);
                } catch (Parser::Standard::Exception & e) {
                    throw Exception(context, e.what(), context.get_global()["ParserException"].to_data(context), context.expression);
                }
            } else {
                auto expression = it->second;

                auto symbols = root->symbols;
                symbols.insert(expression->symbols.begin(), expression->symbols.end());
                root->compute_symbols(symbols);

                return Reference();
            }
        }

        auto copy_args = std::make_shared<Parser::Symbol>("data");
        Reference copy(FunctionContext & context) {
            auto data = context["data"].to_data(context);

            try {
                data.get<Object*>();
                throw Interpreter::FunctionArgumentsError();
            } catch (Data::BadAccess const& e) {
                return Reference(data);
            }
        }

        Reference copy_pointer(FunctionContext & context) {
            return Reference(context["data"].to_data(context));
        }

        auto function_definition_args = std::make_shared<Parser::Tuple>(Parser::Tuple({
            std::make_shared<Parser::Symbol>("var"),
            std::make_shared<Parser::Symbol>("data")
        }));
        Reference function_definition(FunctionContext & context) {
            try {
                auto var = context["var"].to_data(context).get<Object*>();
                auto functions = context["data"].to_data(context).get<Object*>()->functions;

                for (auto it = functions.rbegin(); it != functions.rend(); it++)
                    var->functions.push_front(*it);

                return context["var"];
            } catch (Data::BadAccess & e) {
                throw FunctionArgumentsError();
            }
        }

        bool equals(Data a, Data b) {
            if (auto a_object = std::get_if<Object*>(&a)) {
                if (auto b_object = std::get_if<Object*>(&b))
                    return (*a_object)->properties == (*b_object)->properties && (*a_object)->array == (*b_object)->array;
                else return false;
            } else if (auto a_char = std::get_if<char>(&a)) {
                if (auto b_char = std::get_if<char>(&b)) return *a_char == *b_char;
                else return false;
            } else if (auto a_float = std::get_if<double>(&a)) {
                if (auto b_float = std::get_if<double>(&b)) return *a_float == *b_float;
                else return false;
            } else if (auto a_int = std::get_if<long>(&a)) {
                if (auto b_int = std::get_if<long>(&b)) return *a_int == *b_int;
                else return false;
            } else if (auto a_bool = std::get_if<bool>(&a)) {
                if (auto b_bool = std::get_if<bool>(&b)) return *a_bool == *b_bool;
                else return false;
            } else throw FunctionArgumentsError();
        }

        auto equals_args = std::make_shared<Parser::Tuple>(Parser::Tuple({
            std::make_shared<Parser::Symbol>("a"),
            std::make_shared<Parser::Symbol>("b")
        }));
        Reference equals(FunctionContext & context) {
            auto a = context["a"].to_data(context);
            auto b = context["b"].to_data(context);

            return Reference(Data(equals(a, b)));
        }

        Reference not_equals(FunctionContext & context) {
            auto a = context["a"].to_data(context);
            auto b = context["b"].to_data(context);

            return Reference(Data(!equals(a, b)));
        }

        Reference check_pointers(FunctionContext & context) {
            auto a = context["a"].to_data(context);
            auto b = context["b"].to_data(context);

            return Reference(Data(a == b));
        }

        Reference not_check_pointers(FunctionContext & context) {
            auto a = context["a"].to_data(context);
            auto b = context["b"].to_data(context);

            return Reference(Data(a != b));
        }

        auto to_string_args = std::make_shared<Parser::Symbol>("data");
        Reference to_string(FunctionContext & context) {
            auto data = context["data"].to_data(context);

            return Data(context.new_object((std::ostringstream{} << data).str()));
        }

        void init(GlobalContext & context) {
            context.add_symbol("NotAFunction", context.new_reference());
            context.add_symbol("IncorrectFunctionArguments", context.new_reference());
            context.add_symbol("ParserException", context.new_reference());
            context.add_symbol("RecurionLimitExceeded", context.new_reference());


            auto object = context.new_object();
            object->functions.push_front(SystemFunction{getter_args, getter});
            context.add_symbol("getter", context.new_reference(object));

            context.get_function("setter").push_front(SystemFunction{setter_args, setter});
            set(context, context[":="], context["setter"]);


            context.get_function(";").push_front(SystemFunction{separator_args, separator});

            Function if_s = SystemFunction{if_statement_args, if_statement};
            if_s.extern_symbols.emplace("if", context["if"]);
            if_s.extern_symbols.emplace("else", context["else"]);
            context.get_function("if").push_front(if_s);

            context.get_function("while").push_front(SystemFunction{while_statement_args, while_statement});

            Function for_s = SystemFunction{for_statement_args, for_statement};
            for_s.extern_symbols.emplace("from", context["from"]);
            for_s.extern_symbols.emplace("to", context["to"]);
            context.get_function("for").push_front(for_s);

            Function for_step_s = SystemFunction{for_step_statement_args, for_step_statement};
            for_step_s.extern_symbols.emplace("from", context["from"]);
            for_step_s.extern_symbols.emplace("to", context["to"]);
            for_step_s.extern_symbols.emplace("step", context["step"]);
            context.get_function("for").push_front(for_step_s);

            Function try_s = SystemFunction{try_statement_args, try_statement};
            try_s.extern_symbols.emplace("catch", context["catch"]);
            context.get_function("try").push_front(try_s);
            context.get_function("throw").push_front(SystemFunction{throw_statement_args, throw_statement});

            context.get_function("import").push_front(SystemFunction{path_args, import});

            context.get_function("$").push_front(SystemFunction{copy_args, copy});
            context.get_function("$==").push_front(SystemFunction{copy_args, copy_pointer});

            context.get_function(":").push_front(SystemFunction{function_definition_args, function_definition});

            context.get_function("==").push_front(SystemFunction{equals_args, equals});
            context.get_function("!=").push_front(SystemFunction{equals_args, not_equals});
            context.get_function("===").push_front(SystemFunction{equals_args, check_pointers});
            context.get_function("!==").push_front(SystemFunction{equals_args, not_check_pointers});

            context.get_function("to_string").push_front(SystemFunction{to_string_args, to_string});
        }

    }

}
