#ifndef __INTERPRETER_FUNCTION_HPP__
#define __INTERPRETER_FUNCTION_HPP__

// IWYU pragma: private; include "Interpreter.hpp"

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <variant>

#include "Reference.hpp"

#include "../parser/Expressions.hpp"


namespace Interpreter {

    class FunctionContext;

    using CustomFunction = std::shared_ptr<Parser::FunctionDefinition>;

    struct SystemFunction {
        std::shared_ptr<Parser::Expression> parameters;
        std::function<Reference(FunctionContext&)> pointer;

        [[nodiscard]] friend bool operator==(SystemFunction const& a, SystemFunction const& b) {
            return a.parameters == b.parameters && a.pointer.target<Reference(*)(FunctionContext&)>() == b.pointer.target<Reference(*)(FunctionContext&)>();
        }
    };

    struct Function : public std::variant<CustomFunction, SystemFunction> {
        std::map<std::string, IndirectReference> extern_symbols;

        using std::variant<CustomFunction, SystemFunction>::variant;
    };

}


#endif
