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

#ifndef EXPR_DEFS_H
#define EXPR_DEFS_H

#include <map>
#include <memory>
#include "expr_variant.h"

namespace expr {

using node_array = std::vector<struct node*>;
using define_map_ptr = std::shared_ptr<std::map<string_t, std::pair<string_t, const struct node*>>>;

struct operater {
    enum operater_type {
        LOGIC,
        COMPARE,
        ARITHMETIC,
        EVALUATION,
        INVOCATION,
        FUNCTION
    };

    enum operater_kind {
        UNARY = 1,
        BINARY
    };

    enum operater_attribute {
        KIND,
        PRIORITY,
        TEXT,
        COMMENT,
        POSTPOSE
    };

    // extradefs(expr::operater::logic_operater) // kind // priority // text // comment // postpose
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
        CEIL,               // 1 // 1 // ceil
        FLOOR,              // 1 // 1 // floor
        TRUNC,              // 1 // 1 // trunc
        ROUND,              // 1 // 1 // round
        RINT,               // 1 // 1 // rint
        ABS,                // 1 // 1 // abs
        PHASE,              // 1 // 1 // arg
        REAL,               // 1 // 1 // real
        IMAGINARY,          // 1 // 1 // imag
        CONJUGATE,          // 1 // 1 // conj
        FACTORIAL,          // 1 // 2 // ~!    // postpose // 1
        GAMMA,              // 1 // 1 // gamma
        PERMUTE,            // 2 // 6 // pm
        COMBINE,            // 2 // 6 // cb
        POW,                // 2 // 2 // ^
        EXP,                // 1 // 1 // exp
        LOG,                // 2 // 2 // log
        LG,                 // 1 // 1 // lg
        LN,                 // 1 // 1 // ln
        SQRT,               // 1 // 1 // √     // alias rt
        ROOT,               // 2 // 2 // √     // alias rt
        HYPOT,              // 2 // 6 // ⊿     // alias hp
        VECTOR,             // 2 // 6 // ∠     // alias vec
        DEG,                // 1 // 1 // °     // postpose // 1
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
        PRIME,              // 1 // 1 // pri
        COMPOSITE,          // 1 // 1 // com
        NTH_PRIME,          // 1 // 1 // npri
        NTH_COMPOSITE       // 1 // 1 // ncom
    };

    // extradefs(expr::operater::evaluation_operater)
    enum evaluation_operater {
        COUNT,              // 1 // 1 // cnt   // cnt(<sequence>)
        UNIQUE,             // 1 // 1 // uni
        TOTAL,              // 1 // 1 // tot
        MEAN,               // 1 // 1 // mean
        GEOMETRIC_MEAN,     // 1 // 1 // gmean
        QUADRATIC_MEAN,     // 1 // 1 // qmean
        HARMONIC_MEAN,      // 1 // 1 // hmean
        VARIANCE,           // 1 // 1 // var
        DEVIATION,          // 1 // 1 // dev
        MEDIAN,             // 1 // 1 // med
        MODE,               // 1 // 1 // mode
        MAX,                // 1 // 1 // max
        MIN,                // 1 // 1 // min
        RANGE,              // 1 // 1 // range
        GCD,                // 1 // 1 // gcd
        LCM,                // 1 // 1 // lcm
        DFT,                // 1 // 1 // dft
        IDFT,               // 1 // 1 // idft
        FFT,                // 1 // 1 // fft
        IFFT,               // 1 // 1 // ifft
        ZT                  // 1 // 1 // zt
    };

    // extradefs(expr::operater::invocation_operater)
    enum invocation_operater {
        GENERATE,           // 1 // 1 // gen   // gen(<value>|<function(<sequence>)>,<size>|<function(<sequence>,<item>)>)
        HAS,                // 1 // 1 // has   // has(<sequence>,<value>|<function(<item>,<index>,<sequence>)>)
        PICK,               // 1 // 1 // pick  // pick(<sequence>,<index>|<function(<item>,<index>,<sequence>)>,[<default>])
        SELECT,             // 1 // 1 // sel   // sel(<sequence>,<value>|<function(<item>,<index>,<sequence>)>)
        TRANSFORM,          // 1 // 1 // trans // trans(<sequence>,<value>|<function(<item>,<index>,<sequence>)>)
        ACCUMULATE,         // 1 // 1 // acc   // acc(<sequence>,<function(<accumulation>,<item>)>,<initial>)
        SUMMATE,            // 1 // 1 // ∑     // ∑(<lower>,<upper>,<function(<x>)>); alias sum
        PRODUCE,            // 1 // 1 // ∏     // ∏(<lower>,<upper>,<function(<x>)>); alias prod
        INTEGRATE,          // 1 // 1 // ∫     // ∫(<lower>,<upper>,<function(<x>)>); alias inte
        DOUBLE_INTEGRATE,   // 1 // 1 // ∫∫    // ∫∫(<xlower>,<xupper>,<ylower>,<yupper>,<function(<x>,<y>)>); alias inte2
        TRIPLE_INTEGRATE    // 1 // 1 // ∫∫∫   // ∫∫∫(<xlower>,<xupper>,<ylower>,<yupper>,<zlower>,<zupper>,<function(<x>,<y>,<z>)>); alias inte3
    };

    operater_type           type;
    operater_kind           kind;
    int                     priority;
    bool                    postpose;
    union {
        logic_operater      logic;
        compare_operater    compare;
        arithmetic_operater arithmetic;
        evaluation_operater evaluation;
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
        ARRAY
    };

    object_type             type;
    union {
        bool                boolean;
        real_t              real;
        complex_t*          complex;
        string_t*           string;
        string_t*           param;
        char_t              variable;
        node_array*         array;
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
            case object::ARRAY:
                if (obj.array) {
                    for (node* item : *(obj.array)) {
                        delete item;
                    }
                    delete obj.array;
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

    bool is_array() const {
        return is_object() && object::ARRAY == obj.type;
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

    bool is_evaluation() const {
        return is_expr() && operater::EVALUATION == expr.oper.type;
    }

    bool is_invocation() const {
        return is_expr() && operater::INVOCATION == expr.oper.type;
    }

    bool is_function() const {
        return is_expr() && operater::FUNCTION == expr.oper.type;
    }

    bool is_unary() const {
        return is_expr() && operater::UNARY == expr.oper.kind;
    }

    bool is_binary() const {
        return is_expr() && operater::BINARY == expr.oper.kind;
    }

    bool is_boolean_expr() const {
        return is_logic() || is_compare();
    }

    bool is_eval_expr() const {
        return is_evaluation() || is_invocation() || is_function();
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
        if (super && !super->obj.array->empty()) {
            if (super->obj.array->back() == this) {
                return TAIL;
            }

            if (super->obj.array->front() == this) {
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
        if (!is_function() || !expr.right || !expr.right->is_array()) {
            return string_t();
        }

        string_t variables;
        for (const node* nd : *expr.right->obj.array) {
            if (nd && nd->is_variable()) {
                variables += nd->obj.variable;
            }
        }

        return variables;
    }

    define_map_ptr define_map() const {
        node* def = nullptr;
        for (const node* nd = this; nd; nd = nd->upper()) {
            if (nd->defines && nd->defines->is_array()) {
                def = nd->defines;
                break;
            }
        }

        if (!def) {
            return nullptr;
        }

        define_map_ptr dm = std::make_shared<define_map_ptr::element_type>();
        for (const node* item : *def->obj.array) {
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
