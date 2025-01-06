#ifndef EXPR_MAKER_H
#define EXPR_MAKER_H

#include "expr_defs.h"

#define EXTRA_EXPR_DEFS
#include "extradefs.h"

namespace expr {

inline operater* make_arithmetic(operater::arithmetic_operater arithmetic) {
    operater* oper = new operater;
    oper->type = operater::ARITHMETIC;
    oper->mode = static_cast<operater::operater_mode>(std::stoi(EXTRA_ARITHMETIC_OPERATER.at(arithmetic).at(0)));
    oper->priority = std::stoi(EXTRA_ARITHMETIC_OPERATER.at(arithmetic).at(1));
    oper->arithmetic = arithmetic;
    return oper;
}

inline operater* make_compare(operater::compare_operater compare) {
    operater* oper = new operater;
    oper->type = operater::COMPARE;
    oper->mode = static_cast<operater::operater_mode>(std::stoi(EXTRA_COMPARE_OPERATER.at(compare).at(0)));
    oper->priority = std::stoi(EXTRA_COMPARE_OPERATER.at(compare).at(1));
    oper->compare = compare;
    return oper;
}

inline operater* make_logic(operater::logic_operater logic) {
    operater* oper = new operater;
    oper->type = operater::LOGIC;
    oper->mode = static_cast<operater::operater_mode>(std::stoi(EXTRA_LOGIC_OPERATER.at(logic).at(0)));
    oper->priority = std::stoi(EXTRA_LOGIC_OPERATER.at(logic).at(1));
    oper->logic = logic;
    return oper;
}

inline operater* make_evaluation(operater::evaluation_operater evaluation) {
    operater* oper = new operater;
    oper->type = operater::EVALUATION;
    oper->mode = static_cast<operater::operater_mode>(std::stoi(EXTRA_EVALUATION_OPERATER.at(evaluation).at(0)));
    oper->priority = std::stoi(EXTRA_EVALUATION_OPERATER.at(evaluation).at(1));
    oper->evaluation = evaluation;
    return oper;
}

inline object* make_boolean(bool boolean) {
    object* obj = new object;
    obj->type = object::BOOLEAN;
    obj->boolean = boolean;
    return obj;
}

inline object* make_real(real_t real) {
    object* obj = new object;
    obj->type = object::REAL;
    obj->real = real;
    return obj;
}

inline object* make_complex(real_t real, real_t imag) {
    object* obj = new object;
    obj->type = object::COMPLEX;
    obj->complex = new complex_t(real, imag);
    return obj;
}

inline object* make_string(const string_t& string) {
    object* obj = new object;
    obj->type = object::STRING;
    obj->string = new string_t(string);
    return obj;
}

inline object* make_param(const string_t& param) {
    object* obj = new object;
    obj->type = object::PARAM;
    obj->param = new string_t(param);
    return obj;
}

inline object* make_list(const node_list& list) {
    object* obj = new object;
    obj->type = object::LIST;
    obj->list = new node_list(list);
    return obj;
}

inline node* make_node(object* obj) {
    node* nd = new node;
    nd->type = node::OBJECT;
    nd->obj = obj;
    return nd;
}

inline node* make_node(operater* oper) {
    node* nd = new node;
    nd->type = node::EXPR;
    nd->expr.oper = oper;
    return nd;
}

}

#endif
