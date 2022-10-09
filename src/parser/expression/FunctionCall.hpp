#ifndef __PARSER_EXPRESSION_FUNCTIONCALL_HPP__
#define __PARSER_EXPRESSION_FUNCTIONCALL_HPP__

#include "Expression.hpp"


struct FunctionCall: public Expression {

    FunctionCall() {
        type = Type::FunctionCall;
    }

    std::unique_ptr<Expression> function;
    std::unique_ptr<Expression> object;

};


#endif
