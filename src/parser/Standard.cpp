#include <algorithm>
#include <iostream>

#include "expression/Expression.hpp"
#include "expression/FunctionCall.hpp"
#include "expression/FunctionDefinition.hpp"
#include "expression/Property.hpp"
#include "expression/Symbol.hpp"
#include "expression/Tuple.hpp"

#include "Standard.hpp"


namespace StandardParser {

    TextPosition::TextPosition(std::string const& path, int line, int column): line(line), column(column) {
        this->path = path;
    }

    void TextPosition::notify() {
        std::cerr << "\tin file \"" << path << "\" at line " << line << ", column " << column << std::endl;
    }

    Word::Word(std::string const& word, TextPosition position): word(word), position(position) {}

    ParserError::ParserError(std::string const& message, TextPosition position): message(message), position(position) {}


    std::string operators = "!$%&*+-/:;<=>?@^~";
    bool is_operator(char c) {
        return operators.find(c) < operators.size();
    }

    bool is_number(char c) {
        return std::isdigit(c) || c == '.';
    }

    bool is_alphanum(char c) {
        return std::isalnum(c) || c == '_' || c == '`';
    }

    bool is_operator(std::string const& str) {
        for (char c : str)
            if (!is_operator(c))
                return false;
        return true;
    }

    bool is_number(std::string const& str) {
        for (char c : str)
            if (!is_number(c))
                return false;
        return true;
    }

    bool is_alnum(std::string const& str) {
        for (char c : str)
            if (!is_alphanum(c))
                return false;
        return true;
    }

    std::vector<std::string> system_chars = {"->", ",", "\\", "|->"};
    bool is_system(std::string const& str) {
        return std::find(system_chars.begin(), system_chars.end(), str) != system_chars.end();
    }


    std::vector<Word> get_words(std::string const& path, std::string const& code) {
        std::vector<Word> words;

        int size = code.size();
        int b = 0;
        auto last = '\n';
        auto is_comment = false;
        auto is_str = false;
        auto escape = false;

        int line = 1;
        int column = 1;
        TextPosition position(path, line, column);

        int i;
        for (i = 0; i < size; i++) {
            char c = code[i];

            if (!is_comment) {
                if (!is_str) {
                    if (c == '#') {
                        if (b < i) words.push_back(Word(code.substr(b, i-b), position));
                        b = i+1;
                        is_comment = true;
                    } else if (std::isspace(c)) {
                        if (b < i) words.push_back(Word(code.substr(b, i-b), position));
                        b = i+1;
                    } else if (c == ',' || c == '\\' || c == '(' || c == ')' || c == '[' || c == ']' || c == '{' || c == '}') {
                        if (b < i) words.push_back(Word(code.substr(b, i-b), position));
                        words.push_back(Word(std::string(1,c), position));
                        b = i+1;
                    } else if (c == '\"') {
                        if (b < i) words.push_back(Word(code.substr(b, i-b), position));
                        b = i;
                        is_str = true;
                    } else if (is_operator(last) && !is_operator(c) && b < i) {
                        words.push_back(Word(code.substr(b, i-b), position));
                        b = i;
                    } else if (is_number(last) && !is_number(c) && b < i) {
                        words.push_back(Word(code.substr(b, i-b), position));
                        b = i;
                    } else if (is_alphanum(last) && !is_alphanum(c) && b < i) {
                        words.push_back(Word(code.substr(b, i-b), position));
                        b = i;
                    }
                } else {
                    if (!escape) {
                        if (c == '\"') {
                            words.push_back(Word(code.substr(b, i-b+1), position));
                            b = i+1;
                            is_str = false;
                        } else if (c == '\\') {
                            escape = true;
                        }
                    } else escape = false;
                }
            } else {
                if (c == '\n') is_comment = false;
                b = i+1;
            }

            last = c;
            if (c == '\n') {
                line++;
                column = 1;
            } else column++;
            position = TextPosition(path, line, column);
        }
        if (b < i && !is_comment) words.push_back(Word(code.substr(b, i-b), position));

        return words;
    }

    int get_char_priority(char const& c) {
        if (c == '^') return 1;
        if (c == '*' || c == '/' || c == '%') return 2;
        if (c == '+' || c == '-') return 3;
        //if (c == '!' || c == '=' || c == '<' || c == '>') return 4;
        if (c == '&' || c == '|') return 5;
        if (c == ':') return 6;
        if (c == ';') return 7;
        return 4;
    }

    int compare_operators(std::string const& a, std::string const& b) {
        for (int i = 0; i < (int) a.size() && i < (int) b.size(); i++) {
            if (get_char_priority(a[i]) < get_char_priority(b[i])) return 1;
            if (get_char_priority(a[i]) > get_char_priority(b[i])) return -1;
        }
        if (a.size() < b.size()) return 1;
        else if (a.size() > b.size()) return -1;
        else return 0;
    }

    bool comparator(std::vector<std::string> const& a, std::vector<std::string> const& b) {
        return compare_operators(a[0], b[0]) >= 0;
    }

    std::shared_ptr<Expression> expressions_to_expression(std::vector<ParserError> & errors, std::vector<std::shared_ptr<Expression>> expressions, std::shared_ptr<Expression> expression, bool isFunction) {
        if (expressions.empty()) {
            return expression;
        } else {
            expressions.push_back(expression);
            if (isFunction) {
                auto functioncall = std::make_shared<FunctionCall>();
                functioncall->position = expressions[0]->position;
                functioncall->function = expressions[0];

                if (expressions.size() != 2) {
                    auto tuple = std::make_shared<Tuple>();
                    tuple->position = functioncall->position;
                    for (unsigned long i = 1; i < expressions.size(); i++)
                        tuple->objects.push_back(expressions[i]);
                    functioncall->object = tuple;
                } else functioncall->object = expressions[1];

                return functioncall;
            } else {
                std::vector<std::vector<std::string>> operators;
                for (auto expr : expressions) {
                    if (expr->type == Expression::Symbol) {
                        auto symbol = std::static_pointer_cast<Symbol>(expr);
                        if (!symbol->escaped && is_operator(symbol->name)) {
                            bool put = false;
                            for (auto & op : operators) {
                                if (compare_operators(symbol->name, op[0]) == 0) {
                                    op.push_back(symbol->name);
                                    put = true;
                                    break;
                                }
                            }
                            if (!put) {
                                std::vector<std::string> op;
                                op.push_back(symbol->name);
                                operators.push_back(op);
                            }
                        }
                    }
                }

                std::sort(operators.begin(), operators.end(), comparator);

                for (auto op : operators) {
                    for (auto it = expressions.begin(); it != expressions.end();) {
                        if ((*it)->type == Expression::Symbol) {
                            auto symbol = std::static_pointer_cast<Symbol>(*it);
                            if (!symbol->escaped && std::find(op.begin(), op.end(), symbol->name) != op.end()) {
                                if (expressions.begin() <= it-1 && it+1 < expressions.end()) {
                                    auto functioncall = std::make_shared<FunctionCall>();
                                    functioncall->position = symbol->position;
                                    functioncall->function = symbol;
                                    auto tuple = std::make_shared<Tuple>();
                                    tuple->position = functioncall->position;
                                    tuple->objects.push_back(*(it-1));
                                    tuple->objects.push_back(*(it+1));
                                    functioncall->object = tuple;
                                    
                                    *(it-1) = functioncall;
                                    it = expressions.erase(it);
                                    it = expressions.erase(it);
                                } else errors.push_back(ParserError("operator " + symbol->name + " must be placed between two expressions", *std::static_pointer_cast<TextPosition>(symbol->position)));
                            } else it++;
                        } else it++;
                    }
                }

                return expressions[0];
            }
        }
    }

    std::shared_ptr<Expression> get_expression(std::vector<ParserError> & errors, std::vector<Word> const& words, unsigned long &i, bool inTuple, bool inFunction, bool inOperator, bool priority) {
        if (i >= words.size()) throw IncompleteError();
        
        std::shared_ptr<Expression> expression = nullptr;

        if (words[i].word == "(") {
            i++;
            expression = get_expression(errors, words, i, false, false, false, true);
            expression->escaped = true;
            if (words[i].word == ")") i++;
            else errors.push_back(ParserError(") expected", words[i].position));
        } else if (words[i].word == ")") {
            expression = std::make_shared<Tuple>();
            if (words[i-1].word != "(") {
                errors.push_back(ParserError(") unexpected", words[i].position));
                return expression;
            }
            expression->escaped = true;
            expression->position = std::make_shared<TextPosition>(words[i-1].position);
        } else if (words[i].word == "[") {
            i++;
            expression = get_expression(errors, words, i, false, false, false, true);
            expression->escaped = true;
            if (words[i].word == "]") i++;
            else errors.push_back(ParserError("] expected", words[i].position));
        } else if (words[i].word == "]") {
            expression = std::make_shared<Tuple>();
            if (words[i-1].word != "[") {
                errors.push_back(ParserError("] unexpected", words[i].position));
                return expression;
            }
            expression->escaped = true;
            expression->position = std::make_shared<TextPosition>(words[i-1].position);
        } else if (words[i].word == "{") {
            i++;
            expression = get_expression(errors, words, i, false, false, false, true);
            expression->escaped = true;
            if (words[i].word == "}") i++;
            else errors.push_back(ParserError("} expected", words[i].position));
        } else if (words[i].word == "}") {
            expression = std::make_shared<Tuple>();
            if (words[i-1].word != "{") {
                errors.push_back(ParserError("} unexpected", words[i].position));
                return expression;
            }
            expression->escaped = true;
            expression->position = std::make_shared<TextPosition>(words[i-1].position);
        } else {
            std::shared_ptr<Symbol> symbol = std::make_shared<Symbol>();
            symbol->position = std::make_shared<TextPosition>(words[i].position);
            symbol->name = words[i].word;
            expression = symbol;
            if (is_system(words[i].word))
                errors.push_back(ParserError(words[i].word + " is reserved", words[i].position));
            i++;
        }

        auto isFunction = false;
        std::vector<std::shared_ptr<Expression>> expressions;
        while (i < words.size()) {

            if (words[i].word == ")") break;
            if (words[i].word == "]") break;
            if (words[i].word == "}") break;

            if (words[i].word == "->") {
                while (words[i].word == "->") {
                    auto property = std::make_shared<Property>();
                    property->position = std::make_shared<TextPosition>(words[i].position);
                    property->object = expression;
                    i++;
                    property->name = words[i].word;
                    expression = property;
                    if (is_system(words[i].word))
                        errors.push_back(ParserError(words[i].word + " is reserved", words[i].position));
                    i++;
                }
                continue;
            }
            if (words[i].word == ",") {
                if (!inTuple && priority) {
                    auto tuple = std::make_shared<Tuple>();
                    tuple->position = std::make_shared<TextPosition>(words[i].position);
                    tuple->objects.push_back(expressions_to_expression(errors, expressions, expression, isFunction));
                    while (words[i].word == ",") {
                        i++;
                        tuple->objects.push_back(get_expression(errors, words, i, true, false, false, true));
                    }
                    return tuple;
                } else break;
            }
            if (words[i].word == "\\") {
                if (priority) {
                    i++;
                    auto filter = get_expression(errors, words, i, false, false, false, false);
                    if (words[i].word == "|->") {
                        i++;
                        auto functionDefinition = std::make_shared<FunctionDefinition>();
                        functionDefinition->position = std::make_shared<TextPosition>(words[i-1].position);
                        functionDefinition->parameters = expression;
                        functionDefinition->filter = filter;
                        functionDefinition->object = get_expression(errors, words, i, inTuple, inFunction, inOperator, false);
                        expression = functionDefinition;
                        continue;
                    } else {
                        errors.push_back(ParserError("|-> expected", words[i].position));
                        continue;
                    }
                } else break;
            }
            if (words[i].word == "|->") {
                if (priority) {
                    i++;
                    auto functionDefinition = std::make_shared<FunctionDefinition>();
                    functionDefinition->position = std::make_shared<TextPosition>(words[i-1].position);
                    functionDefinition->parameters = expression;
                    functionDefinition->filter = nullptr;
                    functionDefinition->object = get_expression(errors, words, i, inTuple, inFunction, inOperator, false);
                    expression = functionDefinition;
                    continue;
                } else break;
            }
            if (expression->type == Expression::Symbol) {
                auto symbol = std::static_pointer_cast<Symbol>(expression);

                if (!symbol->escaped && is_operator(symbol->name)) {
                    auto functioncall = std::make_shared<FunctionCall>();
                    functioncall->position = symbol->position;

                    functioncall->function = symbol;
                    functioncall->object = get_expression(errors, words, i, inTuple, inFunction, inOperator, false);
                    
                    expression = functioncall;
                    continue;
                }
            }
            if (is_operator(words[i].word)) {
                if (priority) {
                    if (inOperator) break;
                    else {
                        if (isFunction) {
                            expression = expressions_to_expression(errors, expressions, expression, isFunction);
                            expressions.clear();
                            isFunction = false;
                        }
                        expressions.push_back(expression);
                        auto symbol = std::make_shared<Symbol>();
                        symbol->position = std::make_shared<TextPosition>(words[i].position);
                        symbol->name = words[i].word;
                        expressions.push_back(symbol);
                        i++;
                        if (i >= words.size() || words[i].word == ")" || words[i].word == "]" || words[i].word == "}")
                            expression = std::make_shared<Tuple>();
                        else
                            expression = get_expression(errors, words, i, inTuple, inFunction, true, false);
                        continue;
                    }
                } else break;
            }
            if (!inFunction) {
                isFunction = true;
                expressions.push_back(expression);
                expression = get_expression(errors, words, i, inTuple, true, inOperator, false);
                continue;
            } else break;

        }

        return expressions_to_expression(errors, expressions, expression, isFunction);
    }

    void find_symbols(std::shared_ptr<Expression> expression, std::shared_ptr<Expression> parent, bool new_scope) {
        if (expression == nullptr) return;

        if (parent != nullptr)
            expression->symbols = parent->symbols;

        auto type = expression->type;
        if (type == Expression::FunctionCall) {
            auto functionCall = std::static_pointer_cast<FunctionCall>(expression);

            find_symbols(functionCall->function, expression, false);
            find_symbols(functionCall->object, expression, false);
        } else if (type == Expression::FunctionDefinition) {
            auto functionDefinition = std::static_pointer_cast<FunctionDefinition>(expression);

            find_symbols(functionDefinition->parameters, expression, true);
            find_symbols(functionDefinition->filter, expression, true);
            find_symbols(functionDefinition->object, expression, true);
        } else if (type == Expression::Property) {
            auto property = std::static_pointer_cast<Property>(expression);

            find_symbols(property->object, expression, false);
        } else if (type == Expression::Symbol) {
            auto symbol = std::static_pointer_cast<Symbol>(expression);

            expression->symbols.push_back(symbol->name);
        } else if (type == Expression::Tuple) {
            auto tuple = std::static_pointer_cast<Tuple>(expression);

            for (auto ex : tuple->objects)
                find_symbols(ex, expression, false);
        }

        if (!new_scope)
            for (auto const& name : expression->symbols)
                if (std::find(parent->symbols.begin(), parent->symbols.end(), name) == parent->symbols.end())
                    parent->symbols.push_back(name);
    }

    std::shared_ptr<Expression> get_tree(std::vector<ParserError> & errors, std::string const& path, std::string const& code, std::vector<std::string> symbols) {
        auto words = get_words(path, code);
        unsigned long i = 0;
        auto expression = get_expression(errors, words, i, false, false, false, true);

        expression->symbols = symbols;
        find_symbols(expression, nullptr, true);
        
        return expression;
    }

}
