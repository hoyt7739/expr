#ifndef EXPR_DEFS_H
#define EXPR_DEFS_H

#include "expr_variant.h"

namespace expr {

const real_t PI         = 3.141592653589793;
const real_t NATURAL    = 2.718281828459045;

using node_list = std::vector<struct node*>;

struct operater {
    enum operater_type {
        ARITHMETIC,
        COMPARE,
        LOGIC,
        EVALUATION
    };

    enum operater_mode {
        UNARY  = 1,
        BINARY = 2
    };

    // extradefs(expr::operater::arithmetic_operater) // mode // priority // text
    enum arithmetic_operater {
        PLUS,               // 2 // 5  // +
        MINUS,              // 2 // 5  // -
        MULTIPLY,           // 2 // 4  // *
        DIVIDE,             // 2 // 4  // /
        MOD,                // 2 // 4  // %
        NEGATIVE,           // 1 // 3  // -
        ABS,                // 1 // 1  // abs
        CEIL,               // 1 // 1  // ceil
        FLOOR,              // 1 // 1  // floor
        TRUNC,              // 1 // 1  // trunc
        ROUND,              // 1 // 1  // round
        RINT,               // 1 // 1  // rint
        FACTORIAL,          // 1 // 1  // fact
        POW,                // 2 // 2  // ^
        EXP,                // 1 // 1  // exp
        LOG,                // 2 // 2  // log
        LG,                 // 1 // 1  // lg
        LN,                 // 1 // 1  // ln
        SQRT,               // 1 // 1  // √
        ROOT,               // 2 // 2  // √
        HYPOT,              // 2 // 2  // ⊿
        DEG,                // 1 // 1  // deg
        RAD,                // 1 // 1  // rad
        SIN,                // 1 // 1  // sin
        ARCSIN,             // 1 // 1  // asin
        COS,                // 1 // 1  // cos
        ARCCOS,             // 1 // 1  // acos
        TAN,                // 1 // 1  // tan
        ARCTAN,             // 1 // 1  // atan
        COT,                // 1 // 1  // cot
        ARCCOT,             // 1 // 1  // acot
        SEC,                // 1 // 1  // sec
        ARCSEC,             // 1 // 1  // asec
        CSC,                // 1 // 1  // csc
        ARCCSC,             // 1 // 1  // acsc
        VECTOR,             // 2 // 6  // ∠
        AMPLITUDE,          // 1 // 1  // amp
        ANGLE,              // 1 // 1  // ang
        EXPAND,             // 2 // 7  // ±
        EXPAND_PERCENT      // 2 // 7  // ±%
    };

    // extradefs(expr::operater::compare_operater)
    enum compare_operater {
        LESS,               // 2 // 8  // <
        LESS_EQUAL,         // 2 // 8  // <=
        EQUAL,              // 2 // 8  // =
        APPROACH,           // 2 // 8  // ~=
        REGULAR_MATCH,      // 2 // 8  // ?=
        NOT_EQUAL,          // 2 // 8  // !=
        GREATER_EQUAL,      // 2 // 8  // >=
        GREATER             // 2 // 8  // >
    };

    // extradefs(expr::operater::logic_operater)
    enum logic_operater {
        AND,                // 2 // 9  // &&
        OR,                 // 2 // 10 // ||
        NOT                 // 1 // 1  // !
    };

    // extradefs(expr::operater::evaluation_operater)
    enum evaluation_operater {
        SUM,                // 1 // 1  // sum
        AVERAGE,            // 1 // 1  // avg
        VARIANCE,           // 1 // 1  // var
        DEVIATION,          // 1 // 1  // dev
        MEDIAN,             // 1 // 1  // med
        MODE,               // 1 // 1  // mode
        MAX,                // 1 // 1  // max
        MIN,                // 1 // 1  // min
        RANGE,              // 1 // 1  // range
        ARRANGEMENT,        // 1 // 1  // arra
        COMBINATION,        // 1 // 1  // comb
        LERP                // 1 // 1  // lerp
    };

    operater_type           type;
    operater_mode           mode;
    int                     priority;
    union {
        arithmetic_operater arithmetic;
        compare_operater    compare;
        logic_operater      logic;
        evaluation_operater evaluation;
    };
};

struct object {
    enum object_type {
        BOOLEAN,
        INTEGER,
        REAL,
        COMPLEX,
        STRING,
        PARAM,
        LIST
    };

    object_type             type;
    union {
        bool                boolean;
        integer_t           integer;
        real_t              real;
        complex_t*          complex;
        string_t*           string;
        string_t*           param;
        node_list*          list;
    };
};

struct node {
    enum node_type {
        OBJECT,
        EXPR
    };

    enum node_side {
        LEFT,
        RIGHT
    };

    node_type               type;
    node*                   parent;
    union {
        object*             obj;
        struct {
            operater*       oper;
            node*           left;
            node*           right;
        }                   expr;
    };

    node() {
        memset(this, 0, sizeof(node));
    }

    ~node() {
        switch (type) {
            case OBJECT: {
                switch (obj->type) {
                    case object::COMPLEX: {
                        delete obj->complex;
                        break;
                    }
                    case object::STRING: {
                        delete obj->string;
                        break;
                    }
                    case object::PARAM: {
                        delete obj->param;
                        break;
                    }
                    case object::LIST: {
                        if (obj->list) {
                            for (node* nd : *(obj->list)) {
                                delete nd;
                            }
                            delete obj->list;
                        }
                        break;
                    }
                }
                delete obj;
                break;
            }
            case EXPR: {
                delete expr.oper;
                delete expr.left;
                delete expr.right;
                break;
            }
        }
    }

    bool is_boolean() const {
        return OBJECT == type && object::BOOLEAN == obj->type;
    }

    bool is_numeric() const {
        return OBJECT == type &&
               (object::INTEGER == obj->type || object::REAL == obj->type || object::COMPLEX == obj->type);
    }

    bool is_string() const {
        return OBJECT == type && object::STRING == obj->type;
    }

    bool is_param() const {
        return OBJECT == type && object::PARAM == obj->type;
    }

    bool is_list() const {
        return OBJECT == type && object::LIST == obj->type;
    }

    bool is_value_expr() const {
        return EXPR == type && (operater::ARITHMETIC == expr.oper->type || operater::EVALUATION == expr.oper->type);
    }

    bool is_boolean_expr() const {
        return EXPR == type && (operater::COMPARE == expr.oper->type || operater::LOGIC == expr.oper->type);
    }

    bool is_value_result() const {
        return is_numeric() || is_string() || is_param() || is_value_expr();
    }

    bool is_boolean_result() const {
        return is_boolean() || is_boolean_expr();
    }
};

}

#endif
