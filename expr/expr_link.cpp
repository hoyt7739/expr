#include "expr_link.h"

#define EXTRA_EXPR_DEFS
#include "extradefs.h"

namespace expr {

operater make_logic(operater::logic_operater logic) {
    operater oper;
    oper.type = operater::LOGIC;
    oper.mode = static_cast<operater::operater_mode>(EXTRA_LOGIC_OPERATER.number(logic, 0));
    oper.postpose = false;
    oper.priority = EXTRA_LOGIC_OPERATER.number(logic, 1);
    oper.logic = logic;
    return oper;
}

operater make_compare(operater::compare_operater compare) {
    operater oper;
    oper.type = operater::COMPARE;
    oper.mode = static_cast<operater::operater_mode>(EXTRA_COMPARE_OPERATER.number(compare, 0));
    oper.postpose = false;
    oper.priority = EXTRA_COMPARE_OPERATER.number(compare, 1);
    oper.compare = compare;
    return oper;
}

operater make_arithmetic(operater::arithmetic_operater arithmetic) {
    operater oper;
    oper.type = operater::ARITHMETIC;
    oper.mode = static_cast<operater::operater_mode>(EXTRA_ARITHMETIC_OPERATER.number(arithmetic, 0));
    oper.postpose = EXTRA_ARITHMETIC_OPERATER.number(arithmetic, 3);
    oper.priority = EXTRA_ARITHMETIC_OPERATER.number(arithmetic, 1);
    oper.arithmetic = arithmetic;
    return oper;
}

operater make_statistic(operater::statistic_operater statistic) {
    operater oper;
    oper.type = operater::STATISTIC;
    oper.mode = static_cast<operater::operater_mode>(EXTRA_STATISTIC_OPERATER.number(statistic, 0));
    oper.postpose = false;
    oper.priority = EXTRA_STATISTIC_OPERATER.number(statistic, 1);
    oper.statistic = statistic;
    return oper;
}

operater make_invocation(operater::invocation_operater invocation) {
    operater oper;
    oper.type = operater::INVOCATION;
    oper.mode = static_cast<operater::operater_mode>(EXTRA_INVOCATION_OPERATER.number(invocation, 0));
    oper.postpose = false;
    oper.priority = EXTRA_INVOCATION_OPERATER.number(invocation, 1);
    oper.invocation = invocation;
    return oper;
}

operater make_function(const string_t& function) {
    operater oper;
    oper.type = operater::FUNCTION;
    oper.mode = operater::UNARY;
    oper.postpose = false;
    oper.priority = 1;
    oper.function = new string_t(function);
    return oper;
}

object make_boolean(bool boolean) {
    object obj;
    obj.type = object::BOOLEAN;
    obj.boolean = boolean;
    return obj;
}

object make_real(real_t real) {
    object obj;
    obj.type = object::REAL;
    obj.real = real;
    return obj;
}

object make_complex(real_t real, real_t imag) {
    object obj;
    obj.type = object::COMPLEX;
    obj.complex = new complex_t(real, imag);
    return obj;
}

object make_string(const string_t& string) {
    object obj;
    obj.type = object::STRING;
    obj.string = new string_t(string);
    return obj;
}

object make_param(const string_t& param) {
    object obj;
    obj.type = object::PARAM;
    obj.param = new string_t(param);
    return obj;
}

object make_variable(char_t variable) {
    object obj;
    obj.type = object::VARIABLE;
    obj.variable = variable;
    return obj;
}

object make_list(const node_list& list) {
    object obj;
    obj.type = object::LIST;
    obj.list = new node_list(list);
    return obj;
}

node* make_node(const object& obj) {
    node* nd = new node;
    nd->type = node::OBJECT;
    nd->obj = obj;

    if (object::LIST == obj.type) {
        for (node* item : *obj.list) {
            item->super = nd;
        }
    }

    return nd;
}

node* make_node(const operater& oper) {
    node* nd = new node;
    nd->type = node::EXPR;
    nd->expr.oper = oper;
    return nd;
}

bool link_node(node* parent, node::node_side side, node* child) {
    if (!parent || !parent->is_expr()) {
        return false;
    }

    if ((parent->is_unary()) && (parent->expr.oper.postpose ? node::RIGHT == side : node::LEFT == side)) {
        return !child;
    }

    if (!child) {
        return false;
    }

    (node::LEFT == side ? parent->expr.left : parent->expr.right) = child;
    child->parent = parent;

    return true;
}

bool insert_node(node*& root, node*& semi, node*& pending, node*& current) {
    if (!semi) {
        if (!current) {
            root = pending;
            pending = nullptr;
            return true;
        }

        if (!link_node(current, node::LEFT, pending)) {
            return false;
        }

        root = current;
        semi = current;
        pending = nullptr;
        current = nullptr;
        return true;
    }

    if (!current) {
        if (!link_node(semi, node::RIGHT, pending)) {
            return false;
        }

        pending = nullptr;
        return true;
    }

    if (!semi->is_expr() && current->is_expr()) {
        return false;
    }

    if (current->higher_than(semi) || current->is_unary()) {
        if (!link_node(current, node::LEFT, pending)) {
            return false;
        }

        pending = nullptr;

        if (!link_node(semi, node::RIGHT, current)) {
            return false;
        }

        semi = current;
        current = nullptr;
        return true;
    } else {
        if (!link_node(semi, node::RIGHT, pending)) {
            return false;
        }

        pending = nullptr;

        node* ancestor = semi->parent;
        while (ancestor && !ancestor->lower_than(current)) {
            ancestor = ancestor->parent;
        }

        if (ancestor) {
            if (!link_node(current, node::LEFT, ancestor->expr.right)) {
                return false;
            }

            if (!link_node(ancestor, node::RIGHT, current)) {
                return false;
            }

            semi = current;
            current = nullptr;
            return true;
        } else {
            if (!link_node(current, node::LEFT, root)) {
                return false;
            }

            root = current;
            semi = current;
            current = nullptr;
            return true;
        }
    }

    return false;
}

bool detach_node(node* nd) {
    if (!nd) {
        return false;
    }

    node*& super = nd->super;
    if (super && super->is_list()) {
        for (auto iter = super->obj.list->begin(); super->obj.list->end() != iter; ++iter) {
            if (*iter == nd) {
                super->obj.list->erase(iter);
                break;
            }
        }
        super = nullptr;
    }

    node*& parent = nd->parent;
    if (parent && parent->is_expr()) {
        if (parent->expr.left == nd) {
            parent->expr.left = nullptr;
        } else if (parent->expr.right == nd) {
            parent->expr.right = nullptr;
        }
        parent = nullptr;
    }

    return true;
}

bool test_link(const node* parent, node::node_side side, const node* child, define_map_ptr dm) {
    if (!parent || !parent->is_expr()) {
        return false;
    }

    if (!child) {
        return (parent->is_unary()) && (parent->expr.oper.postpose ? node::RIGHT == side : node::LEFT == side);
    }

    switch (parent->expr.oper.type) {
    case operater::LOGIC:
        return child->is_boolean_result() || child->is_function();
    case operater::COMPARE:
    case operater::ARITHMETIC:
        return child->is_value_result();
    case operater::STATISTIC:
    case operater::INVOCATION:
        return child->is_list();
    case operater::FUNCTION:
        return child->is_list() && dm && dm->end() != dm->find(*parent->expr.oper.function);
    }

    return false;
}

bool test_node(const node* nd, define_map_ptr dm) {
    if (!nd) {
        return true;
    }

    if (!dm) {
        dm = nd->define_map();
    }

    switch (nd->type) {
    case node::OBJECT:
        if (nd->is_list()) {
            for (node* item : *nd->obj.list) {
                if (!test_node(item, dm)) {
                    return false;
                }
            }
        }
        return true;
    case node::EXPR:
        return test_link(nd, node::LEFT, nd->expr.left, dm) &&
               test_link(nd, node::RIGHT, nd->expr.right, dm) &&
               test_node(nd->expr.left, dm) &&
               test_node(nd->expr.right, dm);
    }

    return false;
}

}
