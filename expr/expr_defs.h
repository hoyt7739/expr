#ifndef EXPR_DEFS_H
#define EXPR_DEFS_H

#include <map>
#include <memory>
#include "expr_variant.h"

namespace expr {

using node_list = std::vector<struct node*>;
using define_map_ptr = std::shared_ptr<std::map<string_t, std::pair<string_t, const struct node*>>>;

struct operater {
    enum operater_type {
        LOGIC,
        COMPARE,
        ARITHMETIC,
        STATISTIC,
        INVOCATION,
        FUNCTION
    };

    enum operater_mode {
        UNARY = 1,
        BINARY
    };

    // extradefs(expr::operater::logic_operater) // mode // priority // text // postpose // usage
    enum logic_operater {
        AND,                // 2 // 8 // &&
        OR,                 // 2 // 9 // ||
        NOT                 // 1 // 1 // !
    };

    // extradefs(expr::operater::compare_operater)
    enum compare_operater {
        LESS,               // 2 // 7 // <
        LESS_EQUAL,         // 2 // 7 // <=
        EQUAL,              // 2 // 7 // =
        APPROACH,           // 2 // 7 // ~=
        NOT_EQUAL,          // 2 // 7 // !=
        GREATER_EQUAL,      // 2 // 7 // >=
        GREATER             // 2 // 7 // >
    };

    // extradefs(expr::operater::arithmetic_operater)
    enum arithmetic_operater {
        PLUS,               // 2 // 5 // +
        MINUS,              // 2 // 5 // -
        MULTIPLY,           // 2 // 4 // *
        DIVIDE,             // 2 // 4 // /
        MOD,                // 2 // 4 // %
        NEGATIVE,           // 1 // 3 // -
        ABS,                // 1 // 1 // abs
        CEIL,               // 1 // 1 // ceil
        FLOOR,              // 1 // 1 // floor
        TRUNC,              // 1 // 1 // trunc
        ROUND,              // 1 // 1 // round
        RINT,               // 1 // 1 // rint
        FACTORIAL,          // 1 // 2 // ~!    // 1
        POW,                // 2 // 2 // ^
        EXP,                // 1 // 1 // exp
        LOG,                // 2 // 2 // log
        LG,                 // 1 // 1 // lg
        LN,                 // 1 // 1 // ln
        SQRT,               // 1 // 1 // √
        ROOT,               // 2 // 2 // √
        DEG,                // 1 // 1 // °     // 1
        TODEG,              // 1 // 1 // todeg
        TORAD,              // 1 // 1 // torad
        SIN,                // 1 // 1 // sin
        ARCSIN,             // 1 // 1 // asin
        COS,                // 1 // 1 // cos
        ARCCOS,             // 1 // 1 // acos
        TAN,                // 1 // 1 // tan
        ARCTAN,             // 1 // 1 // atan
        COT,                // 1 // 1 // cot
        ARCCOT,             // 1 // 1 // acot
        SEC,                // 1 // 1 // sec
        ARCSEC,             // 1 // 1 // asec
        CSC,                // 1 // 1 // csc
        ARCCSC,             // 1 // 1 // acsc
        VECTOR,             // 2 // 6 // ∠
        AMPLITUDE,          // 1 // 1 // amp
        ANGLE,              // 1 // 1 // ang
    };

    // extradefs(expr::operater::statistic_operater)
    enum statistic_operater {
        COUNT,              // 1 // 1 // cnt   // 0 // cnt(<sequence>)
        UNIQUE,             // 1 // 1 // uni
        SUM,                // 1 // 1 // sum
        AVERAGE,            // 1 // 1 // avg
        VARIANCE,           // 1 // 1 // var
        DEVIATION,          // 1 // 1 // dev
        MEDIAN,             // 1 // 1 // med
        MODE,               // 1 // 1 // mode
        MAX,                // 1 // 1 // max
        MIN,                // 1 // 1 // min
        RANGE               // 1 // 1 // range
    };

    // extradefs(expr::operater::invocation_operater)
    enum invocation_operater {
        HAS,                // 1 // 1 // has   // 0 // has(<sequence>,<item>|<function(<item>,<index>,<sequence>)>)
        PICK,               // 1 // 1 // pick  // 0 // pick(<sequence>,<index>|<function(<item>,<index>,<sequence>)>,[<default>])
        SELECT,             // 1 // 1 // sel   // 0 // sel(<sequence>,<item>|<function(<item>,<index>,<sequence>)>)
        TRANSFORM,          // 1 // 1 // trans // 0 // trans(<sequence>,<item>|<function(<item>,<index>,<sequence>)>)
        ACCUMULATE,         // 1 // 1 // acc   // 0 // acc(<sequence>,<function(<accumulation>,<item>)>,<initial>)
        GENERATE            // 1 // 1 // gen   // 0 // gen(<item>|<function(<sequence>)>,<size>|<function(<sequence>,<item>)>)
    };

    operater_type           type;
    operater_mode           mode;
    bool                    postpose;
    int                     priority;
    union {
        logic_operater      logic;
        compare_operater    compare;
        arithmetic_operater arithmetic;
        statistic_operater  statistic;
        invocation_operater invocation;
        string_t*           function;
    };
};

struct object {
    enum object_type {
        BOOLEAN,
        REAL,
        COMPLEX,
        STRING,
        PARAM,
        VARIABLE,
        LIST
    };

    object_type             type;
    union {
        bool                boolean;
        real_t              real;
        complex_t*          complex;
        string_t*           string;
        string_t*           param;
        char_t              variable;
        node_list*          list;
    };
};

struct node {
    enum node_type {
        OBJECT,
        EXPR
    };

    enum node_pos {
        HEAD,
        MIDDLE,
        TAIL
    };

    enum node_side {
        LEFT,
        RIGHT
    };

    node_type               type;
    node*                   super;
    node*                   parent;
    node*                   defines;
    union {
        object              obj;
        struct {
            operater        oper;
            node*           left;
            node*           right;
        }                   expr;
    };

    node() {
        memset(this, 0, sizeof(node));
    }

    ~node() {
        delete defines;
        switch (type) {
        case OBJECT:
            switch (obj.type) {
            case object::COMPLEX:
                delete obj.complex;
                break;
            case object::STRING:
                delete obj.string;
                break;
            case object::PARAM:
                delete obj.param;
                break;
            case object::LIST:
                if (obj.list) {
                    for (node* item : *(obj.list)) {
                        delete item;
                    }
                    delete obj.list;
                }
                break;
            }
            break;
        case EXPR:
            if (is_function()) {
                delete expr.oper.function;
            }
            delete expr.left;
            delete expr.right;
            break;
        }
    }

    bool is_object() const {
        return OBJECT == type;
    }

    bool is_boolean() const {
        return is_object() && object::BOOLEAN == obj.type;
    }

    bool is_real() const {
        return is_object() && object::REAL == obj.type;
    }

    bool is_complex() const {
        return is_object() && object::COMPLEX == obj.type;
    }

    bool is_string() const {
        return is_object() && object::STRING == obj.type;
    }

    bool is_param() const {
        return is_object() && object::PARAM == obj.type;
    }

    bool is_variable() const {
        return is_object() && object::VARIABLE == obj.type;
    }

    bool is_list() const {
        return is_object() && object::LIST == obj.type;
    }

    bool is_numeric() const {
        return is_real() || is_complex();
    }

    bool is_expr() const {
        return EXPR == type;
    }

    bool is_logic() const {
        return is_expr() && operater::LOGIC == expr.oper.type;
    }

    bool is_compare() const {
        return is_expr() && operater::COMPARE == expr.oper.type;
    }

    bool is_arithmetic() const {
        return is_expr() && operater::ARITHMETIC == expr.oper.type;
    }

    bool is_statistic() const {
        return is_expr() && operater::STATISTIC == expr.oper.type;
    }

    bool is_invocation() const {
        return is_expr() && operater::INVOCATION == expr.oper.type;
    }

    bool is_function() const {
        return is_expr() && operater::FUNCTION == expr.oper.type;
    }

    bool is_unary() const {
        return is_expr() && operater::UNARY == expr.oper.mode;
    }

    bool is_binary() const {
        return is_expr() && operater::BINARY == expr.oper.mode;
    }

    bool is_boolean_expr() const {
        return is_logic() || is_compare();
    }

    bool is_eval_expr() const {
        return is_statistic() || is_invocation() || is_function();
    }

    bool is_value_expr() const {
        return is_arithmetic() || is_eval_expr();
    }

    bool is_boolean_result() const {
        return is_boolean() || is_boolean_expr();
    }

    bool is_value_result() const {
        return is_numeric() || is_string() || is_param() || is_variable() || is_value_expr();
    }

    node* upper() const {
        return super ? super : parent;
    }

    node_pos pos() const {
        if (super && !super->obj.list->empty()) {
            if (super->obj.list->back() == this) {
                return TAIL;
            }

            if (super->obj.list->front() == this) {
                return HEAD;
            }

            return MIDDLE;
        }

        return TAIL;
    }

    node_side side() const {
        return parent && parent->expr.left == this ? LEFT : RIGHT;
    }

    bool higher_than(const node* other) const {
        return other &&
               ((is_object() && other->is_expr()) ||
                (is_expr() && other->is_expr() && expr.oper.priority < other->expr.oper.priority));
    }

    bool lower_than(const node* other) const {
        return other &&
               ((is_expr() && other->is_object()) ||
                (is_expr() && other->is_expr() && other->expr.oper.priority < expr.oper.priority));
    }

    string_t function_variables() const {
        if (!is_function() || !expr.right || !expr.right->is_list()) {
            return string_t();
        }

        string_t variables;
        for (const node* nd : *expr.right->obj.list) {
            if (nd && nd->is_variable()) {
                variables += nd->obj.variable;
            }
        }

        return variables;
    }

    define_map_ptr define_map() const {
        node* def = nullptr;
        for (const node* nd = this; nd; nd = nd->upper()) {
            if (nd->defines && nd->defines->is_list()) {
                def = nd->defines;
                break;
            }
        }

        if (!def) {
            return nullptr;
        }

        define_map_ptr dm = std::make_shared<define_map_ptr::element_type>();
        for (const node* item : *def->obj.list) {
            if (!item || !item->is_compare() || operater::EQUAL != item->expr.oper.compare) {
                continue;
            }

            const node* rule = item->expr.right;
            if (!rule) {
                continue;
            }

            const node* function = item->expr.left;
            if (!function) {
                continue;
            }

            string_t variables = function->function_variables();
            dm->emplace(*function->expr.oper.function, std::make_pair(variables, rule));
        }

        if (dm->empty())
            return nullptr;

        return dm;
    }
};

}

#endif
