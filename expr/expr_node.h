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

#ifndef EXPR_NODE_H
#define EXPR_NODE_H

#include <map>
#include <memory>
#include "expr_variant.h"

namespace expr {

using node_array = std::vector<struct node*>;
using define_map_ptr = std::shared_ptr<std::map<string_t, std::pair<string_t, const struct node*>>>;

struct operater {
    enum operater_type {
        LOGIC = 1,
        RELATION,
        ARITHMETIC,
        EVALUATION,
        INVOCATION,
        LARGESCALE,
        FUNCTION
    };

    enum operater_kind {
        UNARY = 1,
        BINARY
    };

    enum operater_attribute {
        TYPE,
        KIND,
        PRIORITY,
        NAME,
        ALIAS,
        COMMENT,
        POSTPOSE
    };

    // extradefs(expr::operater::operater_code) // type // kind // priority // name // alias // comment // postpose
    enum operater_code {
        // logic
        AND,                // 1 // 2 // 9 // &&    // &
        OR,                 // 1 // 2 // 9 // ||    // |
        NOT,                // 1 // 1 // 1 // !

        // relation
        EQUAL,              // 2 // 2 // 8 // ==    // =
        APPROACH,           // 2 // 2 // 8 // ~=
        NOT_EQUAL,          // 2 // 2 // 8 // !=
        LESS,               // 2 // 2 // 7 // <
        LESS_EQUAL,         // 2 // 2 // 7 // <=
        GREATER,            // 2 // 2 // 7 // >
        GREATER_EQUAL,      // 2 // 2 // 7 // >=

        // arithmetic
        PLUS,               // 3 // 2 // 5 // +
        MINUS,              // 3 // 2 // 5 // -
        MULTIPLY,           // 3 // 2 // 4 // *
        DIVIDE,             // 3 // 2 // 4 // /
        MODULUS,            // 3 // 2 // 4 // %
        NEGATIVE,           // 3 // 1 // 3 // -
        CEIL,               // 3 // 1 // 1 // ceil
        FLOOR,              // 3 // 1 // 1 // floor
        TRUNC,              // 3 // 1 // 1 // trunc
        ROUND,              // 3 // 1 // 1 // round
        RINT,               // 3 // 1 // 1 // rint
        ABS,                // 3 // 1 // 1 // abs
        PHASE,              // 3 // 1 // 1 // arg
        REAL,               // 3 // 1 // 1 // real
        IMAGINARY,          // 3 // 1 // 1 // imag
        CONJUGATE,          // 3 // 1 // 1 // conj
        FACTORIAL,          // 3 // 1 // 2 // ~!    // // // 1
        GAMMA,              // 3 // 1 // 1 // Γ     // gamma
        PERMUTE,            // 3 // 2 // 6 // pm
        COMBINE,            // 3 // 2 // 6 // cb
        POW,                // 3 // 2 // 2 // ^
        EXP,                // 3 // 1 // 1 // exp
        LOG,                // 3 // 2 // 2 // log
        LG,                 // 3 // 1 // 1 // lg
        LN,                 // 3 // 1 // 1 // ln
        SQRT,               // 3 // 1 // 1 // √     // rt
        ROOT,               // 3 // 2 // 2 // √     // rt
        POLAR,              // 3 // 2 // 6 // ∠     // pl
        DEG,                // 3 // 1 // 1 // °     // deg // // 1
        TODEG,              // 3 // 1 // 1 // todeg
        TORAD,              // 3 // 1 // 1 // torad
        SIN,                // 3 // 1 // 1 // sin
        ARCSIN,             // 3 // 1 // 1 // asin
        COS,                // 3 // 1 // 1 // cos
        ARCCOS,             // 3 // 1 // 1 // acos
        TAN,                // 3 // 1 // 1 // tan
        ARCTAN,             // 3 // 1 // 1 // atan
        COT,                // 3 // 1 // 1 // cot
        ARCCOT,             // 3 // 1 // 1 // acot
        SEC,                // 3 // 1 // 1 // sec
        ARCSEC,             // 3 // 1 // 1 // asec
        CSC,                // 3 // 1 // 1 // csc
        ARCCSC,             // 3 // 1 // 1 // acsc
        PRIME,              // 3 // 1 // 1 // pri
        COMPOSITE,          // 3 // 1 // 1 // com
        NTH_PRIME,          // 3 // 1 // 1 // npri
        NTH_COMPOSITE,      // 3 // 1 // 1 // ncom
        RAND,               // 3 // 1 // 1 // rand

        // evaluation
        COUNT,              // 4 // 1 // 1 // cnt   // // cnt(<sequence>)
        MIN,                // 4 // 1 // 1 // min
        MAX,                // 4 // 1 // 1 // max
        RANGE,              // 4 // 1 // 1 // range
        UNIQUE,             // 4 // 1 // 1 // uniq
        TOTAL,              // 4 // 1 // 1 // total
        MEAN,               // 4 // 1 // 1 // mean
        GEOMETRIC_MEAN,     // 4 // 1 // 1 // gmean
        QUADRATIC_MEAN,     // 4 // 1 // 1 // qmean
        HARMONIC_MEAN,      // 4 // 1 // 1 // hmean
        VARIANCE,           // 4 // 1 // 1 // var
        DEVIATION,          // 4 // 1 // 1 // dev
        MEDIAN,             // 4 // 1 // 1 // med
        MODE,               // 4 // 1 // 1 // mode
        HYPOT,              // 4 // 1 // 1 // hypot
        NORM,               // 4 // 1 // 1 // norm
        ZSCORE_NORM,        // 4 // 1 // 1 // znorm
        GCD,                // 4 // 1 // 1 // gcd
        LCM,                // 4 // 1 // 1 // lcm
        DFT,                // 4 // 1 // 1 // dft
        IDFT,               // 4 // 1 // 1 // idft
        FFT,                // 4 // 1 // 1 // fft
        IFFT,               // 4 // 1 // 1 // ifft
        ZT,                 // 4 // 1 // 1 // zt

        // invocation
        GENERATE,           // 5 // 1 // 1 // gen   // // gen(<value>|<function(<sequence>)>,<size>|<function(<sequence>,<item>)>)
        HAS,                // 5 // 1 // 1 // has   // // has(<sequence>,<value>|<function(<item>,<index>,<sequence>)>)
        PICK,               // 5 // 1 // 1 // pick  // // pick(<sequence>,<index>|<function(<item>,<index>,<sequence>)>,[<default>])
        SELECT,             // 5 // 1 // 1 // sel   // // sel(<sequence>,<value>|<function(<item>,<index>,<sequence>)>)
        SORT,               // 5 // 1 // 1 // sort  // // sort(<sequence>,<function(<item1>,<item2>)>)
        TRANSFORM,          // 5 // 1 // 1 // trans // // trans(<sequence>,<value>|<function(<item>,<index>,<sequence>)>)
        ACCUMULATE,         // 5 // 1 // 1 // acc   // // acc(<sequence>,<function(<accumulation>,<item>,<index>,<sequence>)>,<initial>)

        // largescale
        SUMMATE,            // 6 // 1 // 1 // Σ     // sum   // Σ(<lower>,<upper>,<function(<x>)>)
        PRODUCE,            // 6 // 1 // 1 // Π     // prod  // Π(<lower>,<upper>,<function(<x>)>)
        INTEGRATE,          // 6 // 1 // 1 // ∫     // int   // ∫(<lower>,<upper>,<function(<x>)>)
        DOUBLE_INTEGRATE,   // 6 // 1 // 1 // ∫∫    // int2  // ∫∫(<ylower>,<yupper>,<xlower>,<xupper>,<function(<x>,<y>)>)
        TRIPLE_INTEGRATE    // 6 // 1 // 1 // ∫∫∫   // int3  // ∫∫∫(<zlower>,<zupper>,<ylower>,<yupper>,<xlower>,<xupper>,<function(<x>,<y>,<z>)>)
    };

    operater_type           type;
    operater_kind           kind;
    int                     priority;
    bool                    postpose;
    union {
        operater_code       code;
        string_t*           function;
    };
};

struct object {
    enum object_type {
        BOOLEAN = 1,
        REAL,
        IMAGINARY,
        STRING,
        PARAM,
        VARIABLE,
        ARRAY
    };

    enum object_attribute {
        NAME,
        ALIAS
    };

    // extradefs(expr::object::object_constant) // name // alias
    enum object_constant {
        CONST_FALSE,        // false
        CONST_TRUE,         // true
        CONST_INFINITY,     // ∞     // inf
        CONST_PI,           // π     // pi
        CONST_E             // e
    };

    object_type             type;
    union {
        bool                boolean;
        real_t              real;
        real_t              imaginary;
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

    bool is_imaginary() const {
        return is_object() && object::IMAGINARY == obj.type;
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
        return is_real() || is_imaginary();
    }

    bool is_expr() const {
        return EXPR == type;
    }

    bool is_logic() const {
        return is_expr() && operater::LOGIC == expr.oper.type;
    }

    bool is_relation() const {
        return is_expr() && operater::RELATION == expr.oper.type;
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

    bool is_largescale() const {
        return is_expr() && operater::LARGESCALE == expr.oper.type;
    }

    bool is_function() const {
        return is_expr() && operater::FUNCTION == expr.oper.type;
    }

    bool is_boolean_result() const {
        return is_boolean() || is_logic() || is_relation();
    }

    bool is_value_result() const {
        return is_numeric() || is_string() || is_param() || is_variable() ||
               is_arithmetic() || is_evaluation() || is_invocation() || is_largescale() || is_function();
    }

    bool is_unary() const {
        return is_expr() && operater::UNARY == expr.oper.kind;
    }

    bool is_binary() const {
        return is_expr() && operater::BINARY == expr.oper.kind;
    }

    node* upper() const {
        return super ? super : parent;
    }

    node_pos pos() const {
        if (super && super->obj.array && !super->obj.array->empty()) {
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
            if (!item || !item->is_relation() || operater::EQUAL != item->expr.oper.code) {
                continue;
            }

            const node* rule = item->expr.right;
            if (!rule) {
                continue;
            }

            const node* function = item->expr.left;
            if (!function || !function->is_function()) {
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
