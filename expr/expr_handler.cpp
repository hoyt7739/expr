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

#include "expr_handler.h"
#include <algorithm>
#include "expr_link.h"
#include "expr_operate.h"

#define EXTRA_EXPR_DEFS
#include "extradefs.h"

namespace expr {

enum class parse_state {
    SEGMENT_OPENING,
    SEGMENT_CLOSED
};

const size_t MAX_GENERATE_SIZE      = 10000000;
const size_t INTEGRATE_PIECE_SIZE   = 1000000;
const size_t INTEGRATE2_PIECE_SIZE  = 8000;
const size_t INTEGRATE3_PIECE_SIZE  = 500;

handler::handler(const string_t& expr) : m_expr(expr) {
    node* defines = parse_defines();
    node* root = parse_atom();
    if (root) {
        std::swap(root->defines, defines);
    }

    if (root && finished() && test_node(root)) {
        m_root = root;
    } else {
        delete defines;
        delete root;
    }
}

handler::handler(handler&& other) noexcept : m_expr(std::move(other.m_expr)), m_pos(other.m_pos), m_root(other.m_root) {
    other.m_root = nullptr;
}

handler::~handler() {
    delete m_root;
}

handler& handler::operator=(handler&& other) noexcept {
    if (this != &other) {
        std::swap(m_expr, other.m_expr);
        m_pos = other.m_pos;
        std::swap(m_root, other.m_root);
    }

    return *this;
}

bool handler::is_valid(size_t* failed_pos) const {
    if (m_root) {
        return true;
    }

    if (failed_pos) {
        *failed_pos = m_pos;
    }

    return false;
}

string_t handler::expr() const {
    return expr(m_root);
}

string_t handler::tree(size_t indent) const {
    return tree(m_root, indent);
}

variant handler::calc(const calc_assist& assist) const {
    variant res = calc(m_root, assist);
    return res.is_complex() && 0 == res.complex->imag() ? res.complex->real() : res;
}

char_t handler::get_char(bool skip_space) {
    while (m_pos < m_expr.size()) {
        char_t ch = m_expr[m_pos++];
        if (!skip_space || !(STR('\t') <= ch && ch <= STR('\r') || STR(' ') == ch)) {
            return ch;
        }
    }

    return 0;
}

char_t handler::peek_char() {
    char_t ch = get_char();
    if (ch) {
        --m_pos;
    }

    return ch;
}

bool handler::try_match(const string_t& str) {
    size_t pos = m_pos;
    for (char_t ch : str) {
        if (get_char() != ch) {
            m_pos = pos;
            return false;
        }
    }

    return true;
}

bool handler::atom_ended() {
    char_t ch = peek_char();
    return !ch || STR(',') == ch || STR(')') == ch || STR('}') == ch;
}

bool handler::finished() {
    char_t ch = peek_char();
    return !ch;
}

node* handler::parse_defines() {
    if (!try_match(STR("{"))) {
        return nullptr;
    }

    node* nd = parse_array(false);

    if (!try_match(STR("}")) || !nd || nd->obj.array->empty()) {
        delete nd;
        return nullptr;
    }

    return nd;
}

node* handler::parse_atom() {
    node* root = nullptr;
    node* semi = nullptr;
    node* pending = nullptr;
    node* current = nullptr;
    parse_state state = parse_state::SEGMENT_OPENING;
    while (true) {
        switch (state) {
        case parse_state::SEGMENT_OPENING:
            if (try_match(STR("("))) {
                pending = parse_atom();
                if (!pending) {
                    goto failed;
                }

                if (try_match(STR(","))) {
                    node* nd = parse_array(false);
                    if (!nd) {
                        goto failed;
                    }

                    nd->obj.array->insert(nd->obj.array->begin(), pending);
                    pending->super = nd;
                    pending = nd;
                }

                if (!try_match(STR(")"))) {
                    goto failed;
                }

                state = parse_state::SEGMENT_CLOSED;
            } else {
                current = parse_unary();
                if (current) {
                    if (current->expr.oper.postpose) {
                        goto failed;
                    }

                    bool is_eval = current->is_eval_expr();
                    if (!insert_node(root, semi, pending, current)) {
                        goto failed;
                    }

                    if (is_eval) {
                        pending = parse_array(true);
                        if (!pending) {
                            goto failed;
                        }

                        state = parse_state::SEGMENT_CLOSED;
                    }
                } else {
                    pending = parse_object();
                    if (!pending) {
                        goto failed;
                    }

                    state = parse_state::SEGMENT_CLOSED;
                }
            }
            break;
        case parse_state::SEGMENT_CLOSED:
            if (atom_ended()) {
                if (!insert_node(root, semi, pending, current = nullptr)) {
                    goto failed;
                }

                return root;
            }

            current = parse_binary();
            if (current) {
                if (!insert_node(root, semi, pending, current)) {
                    goto failed;
                }

                state = parse_state::SEGMENT_OPENING;
            } else {
                current = parse_unary();
                if (!current || !current->expr.oper.postpose || !insert_node(root, semi, pending, current)) {
                    goto failed;
                }
            }
            break;
        default:
            goto failed;
        }
    }

failed:
    delete root;
    delete pending;
    delete current;
    return nullptr;
}

node* handler::parse_unary() {
    char_t ch = get_char();
    switch (ch) {
    case STR('!'):
        return make_node(make_logic(operater::NOT));
    case STR('-'):
        return make_node(make_arithmetic(operater::NEGATIVE));
    case STR('a'):
        if (try_match(STR("bs"))) {
            return make_node(make_arithmetic(operater::ABS));
        }
        if (try_match(STR("cc"))) {
            return make_node(make_invocation(operater::ACCUMULATE));
        }
        if (try_match(STR("cos"))) {
            return make_node(make_arithmetic(operater::ARCCOS));
        }
        if (try_match(STR("cot"))) {
            return make_node(make_arithmetic(operater::ARCCOT));
        }
        if (try_match(STR("csc"))) {
            return make_node(make_arithmetic(operater::ARCCSC));
        }
        if (try_match(STR("rg"))) {
            return make_node(make_arithmetic(operater::PHASE));
        }
        if (try_match(STR("sec"))) {
            return make_node(make_arithmetic(operater::ARCSEC));
        }
        if (try_match(STR("sin"))) {
            return make_node(make_arithmetic(operater::ARCSIN));
        }
        if (try_match(STR("tan"))) {
            return make_node(make_arithmetic(operater::ARCTAN));
        }
        break;
    case STR('c'):
        if (try_match(STR("eil"))) {
            return make_node(make_arithmetic(operater::CEIL));
        }
        if (try_match(STR("nt"))) {
            return make_node(make_statistic(operater::COUNT));
        }
        if (try_match(STR("om"))) {
            return make_node(make_arithmetic(operater::COMPOSITE));
        }
        if (try_match(STR("onj"))) {
            return make_node(make_arithmetic(operater::CONJUGATE));
        }
        if (try_match(STR("os"))) {
            return make_node(make_arithmetic(operater::COS));
        }
        if (try_match(STR("ot"))) {
            return make_node(make_arithmetic(operater::COT));
        }
        if (try_match(STR("sc"))) {
            return make_node(make_arithmetic(operater::CSC));
        }
        break;
    case STR('d'):
        if (try_match(STR("ev"))) {
            return make_node(make_statistic(operater::DEVIATION));
        }
        break;
    case STR('e'):
        if (try_match(STR("xp"))) {
            return make_node(make_arithmetic(operater::EXP));
        }
        break;
    case STR('f'):
        if (try_match(STR("loor"))) {
            return make_node(make_arithmetic(operater::FLOOR));
        }
        break;
    case STR('g'):
        if (try_match(STR("amma"))) {
            return make_node(make_arithmetic(operater::GAMMA));
        }
        if (try_match(STR("cd"))) {
            return make_node(make_statistic(operater::GCD));
        }
        if (try_match(STR("en"))) {
            return make_node(make_invocation(operater::GENERATE));
        }
        if (try_match(STR("mean"))) {
            return make_node(make_statistic(operater::GEOMETRIC_MEAN));
        }
        break;
    case STR('h'):
        if (try_match(STR("as"))) {
            return make_node(make_invocation(operater::HAS));
        }
        if (try_match(STR("mean"))) {
            return make_node(make_statistic(operater::HARMONIC_MEAN));
        }
        break;
    case STR('i'):
        if (try_match(STR("mag"))) {
            return make_node(make_arithmetic(operater::IMAGINARY));
        }
        if (try_match(STR("nte1"))) {
            return make_node(make_invocation(operater::INTEGRATE));
        }
        if (try_match(STR("nte2"))) {
            return make_node(make_invocation(operater::DOUBLE_INTEGRATE));
        }
        if (try_match(STR("nte3"))) {
            return make_node(make_invocation(operater::TRIPLE_INTEGRATE));
        }
        if (try_match(STR("nte"))) {
            return make_node(make_invocation(operater::INTEGRATE));
        }
        break;
    case STR('l'):
        if (try_match(STR("cm"))) {
            return make_node(make_statistic(operater::LCM));
        }
        if (try_match(STR("g"))) {
            return make_node(make_arithmetic(operater::LG));
        }
        if (try_match(STR("n"))) {
            return make_node(make_arithmetic(operater::LN));
        }
        break;
    case STR('m'):
        if (try_match(STR("ax"))) {
            return make_node(make_statistic(operater::MAX));
        }
        if (try_match(STR("ean"))) {
            return make_node(make_statistic(operater::MEAN));
        }
        if (try_match(STR("ed"))) {
            return make_node(make_statistic(operater::MEDIAN));
        }
        if (try_match(STR("in"))) {
            return make_node(make_statistic(operater::MIN));
        }
        if (try_match(STR("ode"))) {
            return make_node(make_statistic(operater::MODE));
        }
        break;
    case STR('n'):
        if (try_match(STR("pri"))) {
            return make_node(make_arithmetic(operater::NTH_PRIME));
        }
        if (try_match(STR("com"))) {
            return make_node(make_arithmetic(operater::NTH_COMPOSITE));
        }
        break;
    case STR('p'):
        if (try_match(STR("ick"))) {
            return make_node(make_invocation(operater::PICK));
        }
        if (try_match(STR("ri"))) {
            return make_node(make_arithmetic(operater::PRIME));
        }
        if (try_match(STR("rod"))) {
            return make_node(make_invocation(operater::PRODUCE));
        }
        break;
    case STR('q'):
        if (try_match(STR("mean"))) {
            return make_node(make_statistic(operater::QUADRATIC_MEAN));
        }
        break;
    case STR('r'):
        if (try_match(STR("ange"))) {
            return make_node(make_statistic(operater::RANGE));
        }
        if (try_match(STR("eal"))) {
            return make_node(make_arithmetic(operater::REAL));
        }
        if (try_match(STR("int"))) {
            return make_node(make_arithmetic(operater::RINT));
        }
        if (try_match(STR("ound"))) {
            return make_node(make_arithmetic(operater::ROUND));
        }
        if (try_match(STR("t"))) {
            return make_node(make_arithmetic(operater::SQRT));
        }
        break;
    case STR('s'):
        if (try_match(STR("ec"))) {
            return make_node(make_arithmetic(operater::SEC));
        }
        if (try_match(STR("el"))) {
            return make_node(make_invocation(operater::SELECT));
        }
        if (try_match(STR("in"))) {
            return make_node(make_arithmetic(operater::SIN));
        }
        if (try_match(STR("um"))) {
            return make_node(make_invocation(operater::SUMMATE));
        }
        break;
    case STR('t'):
        if (try_match(STR("an"))) {
            return make_node(make_arithmetic(operater::TAN));
        }
        if (try_match(STR("odeg"))) {
            return make_node(make_arithmetic(operater::TODEG));
        }
        if (try_match(STR("orad"))) {
            return make_node(make_arithmetic(operater::TORAD));
        }
        if (try_match(STR("ot"))) {
            return make_node(make_statistic(operater::TOTAL));
        }
        if (try_match(STR("rans"))) {
            return make_node(make_invocation(operater::TRANSFORM));
        }
        if (try_match(STR("runc"))) {
            return make_node(make_arithmetic(operater::TRUNC));
        }
        break;
    case STR('u'):
        if (try_match(STR("ni"))) {
            return make_node(make_statistic(operater::UNIQUE));
        }
        break;
    case STR('v'):
        if (try_match(STR("ar"))) {
            return make_node(make_statistic(operater::VARIANCE));
        }
        break;
    case STR('~'):
        if (try_match(STR("!"))) {
            return make_node(make_arithmetic(operater::FACTORIAL));
        }
        break;
    case STR('°'):
        return make_node(make_arithmetic(operater::DEG));
    case STR('∑'):
        return make_node(make_invocation(operater::SUMMATE));
    case STR('√'):
        return make_node(make_arithmetic(operater::SQRT));
    case STR('∫'):
        if (try_match(STR("∫∫"))) {
            return make_node(make_invocation(operater::TRIPLE_INTEGRATE));
        }
        if (try_match(STR("∫"))) {
            return make_node(make_invocation(operater::DOUBLE_INTEGRATE));
        }
        return make_node(make_invocation(operater::INTEGRATE));
    }

    if (ch) {
        --m_pos;
    }

    return parse_function();
}

node* handler::parse_binary() {
    char_t ch = get_char();
    switch (ch) {
    case STR('!'):
        if (try_match(STR("="))) {
            return make_node(make_compare(operater::NOT_EQUAL));
        }
        break;
    case STR('%'):
        return make_node(make_arithmetic(operater::MOD));
    case STR('&'):
        try_match(STR("&"));
        return make_node(make_logic(operater::AND));
    case STR('*'):
        return make_node(make_arithmetic(operater::MULTIPLY));
    case STR('+'):
        return make_node(make_arithmetic(operater::PLUS));
    case STR('-'):
        return make_node(make_arithmetic(operater::MINUS));
    case STR('/'):
        return make_node(make_arithmetic(operater::DIVIDE));
    case STR('<'):
        return make_node(make_compare(try_match(STR("=")) ? operater::LESS_EQUAL : operater::LESS));
    case STR('='):
        try_match(STR("="));
        return make_node(make_compare(operater::EQUAL));
    case STR('>'):
        return make_node(make_compare(try_match(STR("=")) ? operater::GREATER_EQUAL : operater::GREATER));
    case STR('^'):
        return make_node(make_arithmetic(operater::POW));
    case STR('c'):
        if (try_match(STR("b"))) {
            return make_node(make_arithmetic(operater::COMBINE));
        }
        break;
    case STR('h'):
        if (try_match(STR("p"))) {
            return make_node(make_arithmetic(operater::HYPOT));
        }
        break;
    case STR('l'):
        if (try_match(STR("og"))) {
            return make_node(make_arithmetic(operater::LOG));
        }
        break;
    case STR('p'):
        if (try_match(STR("m"))) {
            return make_node(make_arithmetic(operater::PERMUTE));
        }
        break;
    case STR('r'):
        if (try_match(STR("t"))) {
            return make_node(make_arithmetic(operater::ROOT));
        }
        break;
    case STR('v'):
        if (try_match(STR("ec"))) {
            return make_node(make_arithmetic(operater::VECTOR));
        }
        break;
    case STR('|'):
        try_match(STR("|"));
        return make_node(make_logic(operater::OR));
    case STR('~'):
        if (try_match(STR("="))) {
            return make_node(make_compare(operater::APPROACH));
        }
        break;
    case STR('√'):
        return make_node(make_arithmetic(operater::ROOT));
    case STR('∠'):
        return make_node(make_arithmetic(operater::VECTOR));
    case STR('⊿'):
        return make_node(make_arithmetic(operater::HYPOT));
    }

    if (ch) {
        --m_pos;
    }

    return nullptr;
}

node* handler::parse_function() {
    size_t pos = m_pos;
    peek_char();

    string_t str;
    char_t ch;
    while (ch = get_char(false)) {
        if (STR('A') <= ch && ch <= STR('Z') || STR('a') <= ch && ch <= STR('z')) {
            str += ch;
        } else {
            break;
        }
    }

    if (ch) {
        --m_pos;
    }

    if (str.empty() || STR('(') != peek_char()) {
        m_pos = pos;
        return nullptr;
    }

    return make_node(make_function(str));
}

node* handler::parse_object() {
    if (node* nd = parse_constant()) {
        return nd;
    }
    if (node* nd = parse_numeric()) {
        return nd;
    }
    if (node* nd = parse_string()) {
        return nd;
    }
    if (node* nd = parse_param()) {
        return nd;
    }

    return parse_variable();
}

node* handler::parse_constant() {
    if (try_match(STR("false"))) {
        return make_node(make_boolean(false));
    }
    if (try_match(STR("true"))) {
        return make_node(make_boolean(true));
    }
    if (try_match(STR("∞")) || try_match(STR("inf"))) {
        return make_node(make_real(INFINITY));
    }
    if (try_match(STR("π")) || try_match(STR("pi"))) {
        return make_node(make_real(CONST_PI));
    }
    if (try_match(STR("e"))) {
        return make_node(make_real(CONST_E));
    }
    if (try_match(STR("rand"))) {
        static unsigned s = 0;
        if (!s) {
            s = (unsigned)time(nullptr);
            srand(s);
        }
        return make_node(make_real(rand()));
    }

    return nullptr;
}

node* handler::parse_numeric() {
    size_t pos = m_pos;
    peek_char();

    string_t str;
    char_t ch;
    while (ch = get_char(false)) {
        if (STR('0') <= ch && ch <= STR('9') || STR('.') == ch || STR('i') == ch) {
            str += ch;
        } else {
            break;
        }
    }

    if (ch) {
        --m_pos;
    }

    if (str.empty() || 1 < std::count(str.begin(), str.end(), STR('.'))) {
        m_pos = pos;
        return nullptr;
    }

    bool is_imag;
    switch (std::count(str.begin(), str.end(), STR('i'))) {
    case 0:
        is_imag = false;
        break;
    case 1:
        if (STR('i') != str.back()) {
            m_pos = pos;
            return nullptr;
        }
        is_imag = true;
        str.pop_back();
        break;
    default:
        m_pos = pos;
        return nullptr;
    }

    real_t real = str.empty() ? 1 : to_real(str);
    return make_node(is_imag ? make_complex(0, real) : make_real(real));
}

node* handler::parse_string() {
    char_t mark = get_char();
    if (STR('\"') != mark && STR('\'') != mark) {
        if (mark) {
            --m_pos;
        }

        return nullptr;
    }

    string_t str;
    bool closed = false;
    char_t ch;
    while (ch = get_char(false)) {
        if (mark == ch) {
            closed = true;
            break;
        }
        str += ch;
    }

    if (!closed) {
        return nullptr;
    }

    return make_node(make_string(str));
}

node* handler::parse_param() {
    if (!try_match(STR("["))) {
        return nullptr;
    }

    string_t str;
    bool closed = false;
    char_t ch;
    while (ch = get_char(false)) {
        if (STR(']') == ch) {
            closed = true;
            break;
        }
        str += ch;
    }

    if (!closed) {
        return nullptr;
    }

    return make_node(make_param(str));
}

node* handler::parse_variable() {
    char_t ch = get_char();
    if (STR('A') <= ch && ch <= STR('Z') || STR('a') <= ch && ch <= STR('z')) {
        return make_node(make_variable(ch));
    }

    if (ch) {
        --m_pos;
    }

    return nullptr;
}

node* handler::parse_array(bool boundary) {
    if (boundary && !try_match(STR("("))) {
        return nullptr;
    }

    node_array array;
    bool closed = false;
    bool failed = false;
    while (true) {
        if (boundary && try_match(STR(")"))) {
            closed = true;
            break;
        }

        node* item = parse_atom();
        if (!item) {
            failed = true;
            break;
        }

        array.push_back(item);

        if (!try_match(STR(","))) {
            closed = boundary && try_match(STR(")"));
            break;
        }
    }

    if ((boundary && !closed) || failed) {
        for (node* item : array) {
            delete item;
        }
        return nullptr;
    }

    return make_node(make_array(array));
}

string_t handler::text(const node* nd) {
    if (!nd) {
        return string_t();
    }

    switch (nd->type) {
    case node::OBJECT:
        switch (nd->obj.type) {
        case object::BOOLEAN:
            return to_string(nd->obj.boolean);
        case object::REAL:
            return to_string(nd->obj.real);
        case object::COMPLEX:
            return to_string(*nd->obj.complex);
        case object::STRING:
            return STR('\"') + *nd->obj.string + STR('\"');
        case object::PARAM:
            return STR('[') + *nd->obj.param + STR(']');
        case object::VARIABLE:
            return string_t(1, nd->obj.variable);
        case object::ARRAY: {
            string_t str;
            for (const node* item : *nd->obj.array) {
                if (!str.empty()) {
                    str += STR(',');
                }
                str += expr(item);
            }
            return STR('(') + str + STR(')');
        }
        }
        break;
    case node::EXPR:
        switch (nd->expr.oper.type) {
        case operater::LOGIC:
            return EXTRA_LOGIC_OPERATER.text(nd->expr.oper.logic, operater::TEXT);
        case operater::COMPARE:
            return EXTRA_COMPARE_OPERATER.text(nd->expr.oper.compare, operater::TEXT);
        case operater::ARITHMETIC:
            return EXTRA_ARITHMETIC_OPERATER.text(nd->expr.oper.arithmetic, operater::TEXT);
        case operater::STATISTIC:
            return EXTRA_STATISTIC_OPERATER.text(nd->expr.oper.statistic, operater::TEXT);
        case operater::INVOCATION:
            return EXTRA_INVOCATION_OPERATER.text(nd->expr.oper.invocation, operater::TEXT);
        case operater::FUNCTION:
            return *nd->expr.oper.function;
        }
        break;
    }

    return string_t();
}

string_t handler::expr(const node* nd) {
    if (!nd) {
        return string_t();
    }

    string_t str = text(nd);
    if (nd->is_expr()) {
        string_t left = expr(nd->expr.left);
        string_t right = expr(nd->expr.right);
        if (nd->expr.left && nd->higher_than(nd->expr.left)) {
            left = STR('(') + left + STR(')');
        }
        if (nd->expr.right && !nd->lower_than(nd->expr.right)) {
            right = STR('(') + right + STR(')');
        }
        str = left + str + right;
    }

    if (nd->defines) {
        string_t defines_str = expr(nd->defines);
        if (!defines_str.empty()) {
            defines_str.front() = STR('{');
            defines_str.back() = STR('}');
            str = defines_str + str;
        }
    }

    return str;
}

string_t handler::tree(const node* nd, size_t indent) {
    if (!nd) {
        return string_t();
    }

    string_t str = string_t(3, STR('─')) + STR(' ') + (nd->is_array() ? STR("array") : text(nd));
    if (nd->upper()) {
        node::node_side side = nd->side();
        str[0] = (node::LEFT == side ? STR('┌') : (node::TAIL == nd->pos() ? STR('└') : STR('├')));
        for (const node* ancestor = nd->upper(); ancestor; ancestor = ancestor->upper()) {
            str = string_t(4, STR(' ')) + str;
            node::node_side this_side = ancestor->side();
            if (ancestor->upper() && (node::TAIL != ancestor->pos() || this_side != side)) {
                str[0] = STR('│');
            }
            side = this_side;
        }
    }

    str = STR('\n') + string_t(indent, STR(' ')) + str;
    if (nd->is_array()) {
        for (const node* item : *nd->obj.array) {
            str += tree(item, indent);
        }
    } else if (nd->is_expr()) {
        if (nd->expr.left) {
            str = tree(nd->expr.left, indent) + str;
        }
        if (nd->expr.right) {
            str += tree(nd->expr.right, indent);
        }
    }

    return str;
}

variant handler::calc(const node* nd, const calc_assist& assist) {
    if (!nd) {
        return variant();
    }

    if (!assist.dm) {
        assist.dm = nd->define_map();
    }

    switch (nd->type) {
    case node::OBJECT:
        return calc_object(nd, assist);
    case node::EXPR:
        switch (nd->expr.oper.type) {
        case operater::INVOCATION:
            return calc_invocation(nd, assist);
        case operater::FUNCTION:
            return calc_function(nd, assist);
        }
        return operate(calc(nd->expr.left, assist), nd->expr.oper, calc(nd->expr.right, assist));
    }

    return variant();
}

variant handler::calc_object(const node* nd, const calc_assist& assist) {
    switch (nd->obj.type) {
    case object::BOOLEAN:
        return nd->obj.boolean;
    case object::REAL:
        return nd->obj.real;
    case object::COMPLEX:
        return *nd->obj.complex;
    case object::STRING:
        return *nd->obj.string;
    case object::PARAM:
        return assist.pr ? assist.pr(*nd->obj.param) : variant();
    case object::VARIABLE:
        return assist.vr ? assist.vr(nd->obj.variable) : variant();
    case object::ARRAY: {
        sequence_t sequence(nd->obj.array->size());
        std::transform(nd->obj.array->begin(), nd->obj.array->end(), sequence.begin(), [&assist](node* nd) { return calc(nd, assist); });
        return sequence;
    }
    }

    return variant();
}

variant handler::calc_function(const node* nd, const calc_assist& assist) {
    if (!assist.dm) {
        return variant();
    }

    auto iter = assist.dm->find(*nd->expr.oper.function);
    if (assist.dm->end() == iter) {
        return variant();
    }

    variant right = calc(nd->expr.right, assist);
    if (!right.is_sequence()) {
        return variant();
    }

    const string_t& variables = iter->second.first;
    const node* rule = iter->second.second;
    const sequence_t& args = *right.sequence;
    variable_replacer vr = [&variables, &args](char_t variable) {
        size_t pos = variables.find(variable);
        return string_t::npos != pos && pos < args.size() ? args[pos] : variant();
    };

    return calc(rule, {assist.pr, vr, assist.dm});
}

variant handler::calc_invocation(const node* nd, const calc_assist& assist) {
    if (!nd->expr.right || !nd->expr.right->is_array()) {
        return variant();
    }

    const node_array& wrap = *nd->expr.right->obj.array;
    switch (nd->expr.oper.invocation) {
    case operater::GENERATE:
        return calc_generate(wrap, assist);
    case operater::HAS:
    case operater::PICK:
    case operater::SELECT:
    case operater::TRANSFORM:
    case operater::ACCUMULATE:
        return calc_sequence(nd->expr.oper.invocation, wrap, assist);
    case operater::SUMMATE:
    case operater::PRODUCE:
        return calc_cumulate(nd->expr.oper.invocation, wrap, assist);
    case operater::INTEGRATE:
        return calc_integrate(wrap, assist);
    case operater::DOUBLE_INTEGRATE:
        return calc_integrate2(wrap, assist);
    case operater::TRIPLE_INTEGRATE:
        return calc_integrate3(wrap, assist);
    }

    return variant();
}

variant handler::calc_generate(const node_array& wrap, const calc_assist& assist) {
    if (wrap.size() < 2) {
        return variant();
    }

    string_t variables0 = wrap[0]->function_variables();
    variant arg0 = (variables0.empty() ? calc(wrap[0], assist) : variant());

    string_t variables1 = wrap[1]->function_variables();
    variant arg1 = (variables1.empty() ? calc(wrap[1], assist) : variant());
    size_t max_size = (arg1.is_valid() ? std::min(static_cast<size_t>(arg1.to_real()), MAX_GENERATE_SIZE) : MAX_GENERATE_SIZE);

    sequence_t res;
    calc_assist generator_assist = {assist.pr, [&res](char_t) { return res; }, assist.dm};
    while (res.size() < max_size) {
        variant item = (variables0.empty() ? arg0 : calc_function(wrap[0], generator_assist));
        if (!item.is_valid()) {
            break;
        }

        if (!variables1.empty()) {
            variable_replacer vr = [&res, &item, &variables1](char_t variable) { return variables1[0] == variable ? res : item; };
            if (!calc_function(wrap[1], {assist.pr, vr, assist.dm}).to_boolean()) {
                break;
            }
        }

        res.emplace_back(std::move(item));
    }

    return res;
}

variant handler::calc_sequence(operater::invocation_operater invocation, const node_array& wrap, const calc_assist& assist) {
    if (wrap.size() < 2) {
        return variant();
    }

    variant arg0 = calc(wrap[0], assist);
    if (!arg0.is_sequence()) {
        return variant();
    }

    string_t variables = wrap[1]->function_variables();
    variant arg1 = (variables.empty() ? calc(wrap[1], assist) : variant());

    using std::placeholders::_1;
    const sequence_t& sequence = *arg0.sequence;
    size_t size = sequence.size();
    auto sequence_vr = [&sequence](size_t index, const string_t& variables, char_t variable) -> variant {
        if (1 <= variables.size() && variables[0] == variable) {
            return sequence[index];
        }

        if (2 <= variables.size() && variables[1] == variable) {
            return index;
        }

        return sequence;
    };

    switch (invocation) {
    case operater::HAS: {
        if (variables.empty()) {
            return sequence.end() != std::find(sequence.begin(), sequence.end(), arg1);
        }

        for (size_t index = 0; index < size; ++index) {
            variable_replacer vr = std::bind(sequence_vr, index, variables, _1);
            if (calc_function(wrap[1], {assist.pr, vr, assist.dm}).to_boolean()) {
                return true;
            }
        }

        return false;
    }
    case operater::PICK: {
        variant arg2 = (3 <= wrap.size() ? calc(wrap[2], assist) : variant());
        if (variables.empty()) {
            real_t real = arg1.to_real();
            size_t index = static_cast<size_t>(real < 0 ? size + real : real);
            return index < size ? sequence[index] : arg2;
        }

        for (size_t index = 0; index < size; ++index) {
            variable_replacer vr = std::bind(sequence_vr, index, variables, _1);
            if (calc_function(wrap[1], {assist.pr, vr, assist.dm}).to_boolean()) {
                return sequence[index];
            }
        }

        return arg2;
    }
    case operater::SELECT: {
        sequence_t res;
        for (size_t index = 0; index < size; ++index) {
            if (variables.empty()) {
                if (sequence[index] == arg1) {
                    res.push_back(arg1);
                }
            } else {
                variable_replacer vr = std::bind(sequence_vr, index, variables, _1);
                if (calc_function(wrap[1], {assist.pr, vr, assist.dm}).to_boolean()) {
                    res.push_back(sequence[index]);
                }
            }
        }

        return res;
    }
    case operater::TRANSFORM: {
        sequence_t res(size);
        for (size_t index = 0; index < size; ++index) {
            if (variables.empty()) {
                res[index] = arg1;
            } else {
                variable_replacer vr = std::bind(sequence_vr, index, variables, _1);
                res[index] = calc_function(wrap[1], {assist.pr, vr, assist.dm});
            }
        }

        return res;
    }
    case operater::ACCUMULATE: {
        if (wrap.size() < 3) {
            return variant();
        }

        variant arg2 = calc(wrap[2], assist);
        if (!arg2.is_valid() || variables.size() < 2) {
            return arg2;
        }

        for (const variant& item : sequence) {
            variable_replacer vr = [&arg2, &item, &variables](char_t variable) { return variables[0] == variable ? arg2 : item; };
            arg2 = calc_function(wrap[1], {assist.pr, vr, assist.dm});
        }

        return arg2;
    }
    }

    return variant();
}

handler::bound_t handler::calc_bound(const node* lower_nd, const node* upper_nd, const calc_assist& assist, bool to_zahlen) {
    bound_t bound = {calc(lower_nd, assist).to_real(), calc(upper_nd, assist).to_real()};
    if (bound.second < bound.first) {
        std::swap(bound.first, bound.second);
    }

    if (to_zahlen) {
        bound.first = trunc(bound.first);
        bound.second = trunc(bound.second);
    }

    return bound;
}

variant handler::calc_cumulate(operater::invocation_operater invocation, const node_array& wrap, const calc_assist& assist) {
    if (wrap.size() < 3 || wrap[2]->function_variables().empty()) {
        return variant();
    }

    variant res;
    operater oper;
    switch (invocation) {
    case operater::SUMMATE:
        res = real_t(0);
        oper = make_arithmetic(operater::PLUS);
        break;
    case operater::PRODUCE:
        res = real_t(1);
        oper = make_arithmetic(operater::MULTIPLY);
        break;
    default:
        return variant();
    }

    bound_t bound = calc_bound(wrap[0], wrap[1], assist, true);
    for (real_t n = bound.first; n <= bound.second; ++n) {
        res = operate(res, oper, calc_function(wrap[2], {assist.pr, [n](char_t) { return n; }, assist.dm}));
    }

    return res;
}

variant handler::calc_integrate(const node_array& wrap, const calc_assist& assist) {
    if (wrap.size() < 3 || wrap[2]->function_variables().empty()) {
        return variant();
    }

    bound_t bound = calc_bound(wrap[0], wrap[1], assist);
    real_t delta = (bound.second - bound.first) / INTEGRATE_PIECE_SIZE;
    auto integrand = [&wrap, &assist](real_t x) {
        return calc_function(wrap[2], {assist.pr, [x](char_t) { return x; }, assist.dm}).to_real();
    };

    real_t res = (integrand(bound.first) + integrand(bound.second)) * 0.5;
    for (size_t n = 1; n < INTEGRATE_PIECE_SIZE; ++n) {
        res += integrand(bound.first + delta * n);
    }

    return res * delta;
}

variant handler::calc_integrate2(const node_array& wrap, const calc_assist& assist) {
    if (wrap.size() < 5) {
        return variant();
    }

    string_t variables = wrap[4]->function_variables();
    if (variables.size() < 2) {
        return variant();
    }

    bound_t xbound = calc_bound(wrap[0], wrap[1], assist);
    real_t xdelta = (xbound.second - xbound.first) / INTEGRATE2_PIECE_SIZE;

    bound_t ybound = calc_bound(wrap[2], wrap[3], assist);
    real_t ydelta = (ybound.second - ybound.first) / INTEGRATE2_PIECE_SIZE;

    auto integrand = [&wrap, &assist, &variables](real_t x, real_t y) {
        variable_replacer vr = [x, y, &variables](char_t variable) { return variables[0] == variable ? x : y; };
        return calc_function(wrap[4], {assist.pr, vr, assist.dm}).to_real();
    };

    real_t res = 0;
    for (size_t xn = 0; xn <= INTEGRATE2_PIECE_SIZE; ++xn) {
        real_t x = xbound.first + xdelta * xn;
        for (size_t yn = 0; yn <= INTEGRATE2_PIECE_SIZE; ++yn) {
            real_t value = integrand(x, ybound.first + ydelta * yn);
            if (0 == xn || INTEGRATE2_PIECE_SIZE == xn) {
                value *= 0.5;
            }
            if (0 == yn || INTEGRATE2_PIECE_SIZE == yn) {
                value *= 0.5;
            }
            res += value;
        }
    }

    return res * xdelta * ydelta;
}

variant handler::calc_integrate3(const node_array& wrap, const calc_assist& assist) {
    if (wrap.size() < 7) {
        return variant();
    }

    string_t variables = wrap[6]->function_variables();
    if (variables.size() < 3) {
        return variant();
    }

    bound_t xbound = calc_bound(wrap[0], wrap[1], assist);
    real_t xdelta = (xbound.second - xbound.first) / INTEGRATE3_PIECE_SIZE;

    bound_t ybound = calc_bound(wrap[2], wrap[3], assist);
    real_t ydelta = (ybound.second - ybound.first) / INTEGRATE3_PIECE_SIZE;

    bound_t zbound = calc_bound(wrap[4], wrap[5], assist);
    real_t zdelta = (zbound.second - zbound.first) / INTEGRATE3_PIECE_SIZE;

    auto integrand = [&wrap, &assist, &variables](real_t x, real_t y, real_t z) {
        variable_replacer vr = [x, y, z, &variables](char_t variable) {
            return variables[0] == variable ? x : (variables[1] == variable ? y : z);
        };
        return calc_function(wrap[6], {assist.pr, vr, assist.dm}).to_real();
    };

    real_t res = 0;
    for (size_t xn = 0; xn <= INTEGRATE3_PIECE_SIZE; ++xn) {
        real_t x = xbound.first + xdelta * xn;
        for (size_t yn = 0; yn <= INTEGRATE3_PIECE_SIZE; ++yn) {
            real_t y = ybound.first + ydelta * yn;
            for (size_t zn = 0; zn <= INTEGRATE3_PIECE_SIZE; ++zn) {
                real_t value = integrand(x, y, zbound.first + zdelta * zn);
                if (0 == xn || INTEGRATE3_PIECE_SIZE == xn) {
                    value *= 0.5;
                }
                if (0 == yn || INTEGRATE3_PIECE_SIZE == yn) {
                    value *= 0.5;
                }
                if (0 == zn || INTEGRATE3_PIECE_SIZE == zn) {
                    value *= 0.5;
                }
                res += value;
            }
        }
    }

    return res * xdelta * ydelta * zdelta;
}

}
