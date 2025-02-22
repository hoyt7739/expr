/*
  MIT License

  Copyright (c) 2025 Kong Pengsheng

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

#include "expr_link.h"

#define EXTRA_EXPR_NODE
#include "extradefs.h"

namespace expr {

operater make_operater(operater::operater_code code) {
    operater oper;
    oper.type = static_cast<decltype(oper.type)>(EXTRA_OPERATER_CODE.integer(code, operater::TYPE));
    oper.kind = static_cast<decltype(oper.kind)>(EXTRA_OPERATER_CODE.integer(code, operater::KIND));
    oper.priority = EXTRA_OPERATER_CODE.integer(code, operater::PRIORITY);
    oper.postpose = EXTRA_OPERATER_CODE.integer(code, operater::POSTPOSE);
    oper.code = code;
    return oper;
}

operater make_function(const string_t& function) {
    operater oper;
    oper.type = operater::FUNCTION;
    oper.kind = operater::UNARY;
    oper.priority = 1;
    oper.postpose = false;
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

object make_imaginary(real_t imaginary) {
    object obj;
    obj.type = object::IMAGINARY;
    obj.imaginary = imaginary;
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

object make_array(const node_array& array) {
    object obj;
    obj.type = object::ARRAY;
    obj.array = new node_array(array);
    return obj;
}

node* make_node(const object& obj) {
    node* nd = new node;
    nd->type = node::OBJECT;
    nd->obj = obj;

    if (object::ARRAY == obj.type) {
        for (node* item : *obj.array) {
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
    if (super && super->is_array()) {
        for (auto iter = super->obj.array->begin(); super->obj.array->end() != iter; ++iter) {
            if (*iter == nd) {
                super->obj.array->erase(iter);
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
    case operater::RELATION:
    case operater::ARITHMETIC:
        return child->is_value_result();
    case operater::EVALUATION:
    case operater::INVOCATION:
    case operater::LARGESCALE:
        return child->is_array();
    case operater::FUNCTION:
        return child->is_array() && dm && dm->end() != dm->find(*parent->expr.oper.function);
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
        if (nd->is_array()) {
            for (const node* item : *nd->obj.array) {
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
