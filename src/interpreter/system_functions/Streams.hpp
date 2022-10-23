#ifndef __INTERPRETER_SYSTEMFUNCTIONS_STREAMS_HPP__
#define __INTERPRETER_SYSTEMFUNCTIONS_STREAMS_HPP__

#include "../Interpreter.hpp"


namespace Streams {

    std::shared_ptr<Expression> print();
    Reference print(FunctionContext & context);

    std::shared_ptr<Expression> scan();
    Reference scan(FunctionContext & context);

    Reference read(FunctionContext & context);

    Reference has(FunctionContext & context);

    void setInputStream(Context & context, Object & object);

    Reference write(FunctionContext & context);

    Reference flush(FunctionContext & context);

    void setOutputStream(Context & context, Object & object);

    std::shared_ptr<Expression> path();
    Reference input_file(FunctionContext & context);
    Reference output_file(FunctionContext & context);

    void init(Context & context);

}


#endif