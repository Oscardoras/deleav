#ifndef __COMPILER_C_TRANSLATOR_HPP__
#define __COMPILER_C_TRANSLATOR_HPP__

#include <list>
#include <variant>

#include "Code.hpp"

#include "../Analyzer.hpp"


namespace Translator::CStandard {

    struct TypeTable: public std::map<std::shared_ptr<Analyzer::Type>, std::shared_ptr<Type>> {

        TypeTable() {
            operator[](Analyzer::Bool) = Bool;
            operator[](Analyzer::Char) = Char;
            operator[](Analyzer::Int) = Int;
            operator[](Analyzer::Float) = Float;
        }

        std::shared_ptr<Type> get(Analyzer::Types const& types) {
            if (types.size() == 1)
                return operator[](types.begin()->lock());
            else
                return Unknown;
        }

    };
    using FunctionTable = std::map<std::shared_ptr<Analyzer::FunctionDefinition>, std::shared_ptr<FunctionDefinition>>;

    using Instructions = std::vector<std::shared_ptr<Instruction>>;

    class Translator {

        TypeTable type_table;
        FunctionTable function_table;

    public:

        std::pair<std::set<std::shared_ptr<Component>>, std::set<std::shared_ptr<Class>>> create_structures(std::set<std::shared_ptr<Analyzer::Structure>> const& structures);
        std::set<std::shared_ptr<FunctionDefinition>> create_functions(std::set<std::shared_ptr<Analyzer::FunctionDefinition>> const& functions);

        std::shared_ptr<Expression> eval_system_function(Analyzer::SystemFunction const& function, std::shared_ptr<Analyzer::Expression> arguments, Instructions & instructions);

        std::shared_ptr<FunctionDefinition> get_function(std::shared_ptr<Analyzer::FunctionDefinition> function);
        void get_instructions(std::shared_ptr<Analyzer::Expression> expression, Instructions & instructions);
        std::shared_ptr<Expression> get_expression(std::shared_ptr<Analyzer::Expression> expression, Instructions & instructions);

    };

}


#endif
