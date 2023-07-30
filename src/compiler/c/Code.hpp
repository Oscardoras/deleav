#ifndef __COMPILER_C_CODE_HPP__
#define __COMPILER_C_CODE_HPP__

#include <memory>
#include <string>
#include <vector>

#include "../Analyzer.hpp"


namespace Translator::CStandard {

    struct Type {

        std::string name;

        virtual ~Type() = default;

    };
    using Declarations = std::map<std::string, std::weak_ptr<Type>>;
    struct Class: public Type {
        Declarations properties;
    };
    inline std::shared_ptr<Type> Unknown = nullptr;
    inline std::shared_ptr<Type> Bool = std::make_shared<Type>();
    inline std::shared_ptr<Type> Char = std::make_shared<Type>();
    inline std::shared_ptr<Type> Int = std::make_shared<Type>();
    inline std::shared_ptr<Type> Float = std::make_shared<Type>();


    struct Expression {
        std::weak_ptr<Type> type;
        enum class Reference {
            None,
            Owned,
            Shared,
        } reference;
    };
    struct LValue: public Expression {};
    struct Instruction {};
    struct Block: public Instruction, public std::vector<std::shared_ptr<Instruction>> {
        using std::vector<std::shared_ptr<Instruction>>::vector;
    };

    struct If: public Instruction {
        std::shared_ptr<Expression> condition;
        Block body;
        Block alternative;
    };

    struct While: public Instruction {
        std::shared_ptr<Expression> condition;
        Block body;
    };

    struct Switch: public Instruction {
        std::shared_ptr<Expression> value;
        std::vector<std::pair<std::vector<std::string>, Block>> cases;
    };

    struct Affectation: public Expression, public Instruction {
        std::shared_ptr<LValue> lvalue;
        std::shared_ptr<Expression> value;
    };

    struct VariableCall: public LValue {
        std::string name;
    };

    struct Value: public Expression {
        std::variant<char, bool, long, double, std::string> value;
    };

    struct FunctionCall: public Expression, public Instruction {
        std::shared_ptr<Expression> function;
        std::vector<std::shared_ptr<Expression>> parameters;
    };

    struct Property: public LValue {
        std::shared_ptr<Expression> object;
        std::string name;
        bool pointer;
    };

    struct List: public Expression {
        std::vector<std::shared_ptr<Expression>> objects;
    };

    struct Array: public Instruction {
        std::weak_ptr<Type> type;
        std::string name;
        std::shared_ptr<List> list;
    };


    struct FunctionDefinition {
        using Parameter = std::pair<std::string, std::weak_ptr<Type>>;
        using Parameters = std::vector<Parameter>;
        std::weak_ptr<Type> type;
        std::string name;
        Parameters parameters;
        Declarations local_variables;
        Block body;
    };

}


#endif
