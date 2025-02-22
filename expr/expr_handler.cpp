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

#define EXTRA_EXPR_NODE
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

string_t handler::latex() const {
    return latex(m_root);
}

string_t handler::tree(size_t indent) const {
    return tree(m_root, indent);
}

variant handler::calc(const calc_assist& assist) const {
    static unsigned s = 0;
    if (!s) {
        s = (unsigned)time(nullptr);
        srand(s);
    }

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
    if (str.empty()) {
        return false;
    }

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
                current = parse_operater(operater::UNARY);
                if (current) {
                    if (current->expr.oper.postpose) {
                        goto failed;
                    }

                    bool need_array =
                        current->is_evaluation() || current->is_invocation() || current->is_largescale() || current->is_function();

                    if (!insert_node(root, semi, pending, current)) {
                        goto failed;
                    }

                    if (need_array) {
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

            current = parse_operater(operater::BINARY);
            if (current) {
                if (!insert_node(root, semi, pending, current)) {
                    goto failed;
                }
                state = parse_state::SEGMENT_OPENING;
            } else {
                current = parse_operater(operater::UNARY);
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

node* handler::parse_operater(operater::operater_kind kind) {
    for (auto iter = EXTRA_OPERATER_CODE.rbegin(); EXTRA_OPERATER_CODE.rend() != iter; ++iter) {
        if (kind == iter->second.integer(operater::KIND)) {
            if (try_match(iter->second.string(operater::NAME)) || try_match(iter->second.string(operater::ALIAS))) {
                return make_node(make_operater(iter->first));
            }
        }
    }

    return operater::UNARY == kind ? parse_function() : nullptr;
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
    for (auto iter = EXTRA_OBJECT_CONSTANT.rbegin(); EXTRA_OBJECT_CONSTANT.rend() != iter; ++iter) {
        if (try_match(iter->second.string(object::NAME)) || try_match(iter->second.string(object::ALIAS))) {
            switch (iter->first) {
            case object::CONST_FALSE:
                return make_node(make_boolean(false));
            case object::CONST_TRUE:
                return make_node(make_boolean(true));
            case object::CONST_INFINITY:
                return make_node(make_real(INFINITY));
            case object::CONST_PI:
                return make_node(make_real(REAL_PI));
            case object::CONST_E:
                return make_node(make_real(REAL_E));
            }
        }
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

    real_t num = str.empty() ? 1 : to_real(str);
    return make_node(is_imag ? make_imaginary(num) : make_real(num));
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

    auto constant_text = [](real_t num) {
        real_t abs = fabs(num);
        string_t str;
        if (approach_to(abs, REAL_PI)) {
            str = EXTRA_OBJECT_CONSTANT.string(object::CONST_PI, object::NAME);
        } else if (approach_to(abs, REAL_E)) {
            str = EXTRA_OBJECT_CONSTANT.string(object::CONST_E, object::NAME);
        }
        if (!str.empty() && num < 0) {
            str = STR('-') + str;
        }
        return str;
    };

    switch (nd->type) {
    case node::OBJECT:
        switch (nd->obj.type) {
        case object::BOOLEAN:
            return to_string(nd->obj.boolean);
        case object::REAL: {
            string_t str = constant_text(nd->obj.real);
            return !str.empty() ? str : to_string(nd->obj.real);
        }
        case object::IMAGINARY: {
            string_t str = constant_text(nd->obj.imaginary);
            return !str.empty() ? str + STR('i') : to_string(complex_t(0, nd->obj.imaginary));
        }
        case object::STRING:
            return format(STR("\"%1\""), *nd->obj.string);
        case object::PARAM:
            return format(STR("[%1]"), *nd->obj.param);
        case object::VARIABLE:
            return string_t(1, nd->obj.variable);
        case object::ARRAY: {
            const node_array& na = *nd->obj.array;
            string_array sa(na.size());
            std::transform(na.begin(), na.end(), sa.begin(), [](const node* nd) { return expr(nd); });
            return format(STR("(%1)"), join(sa, STR(",")));
        }
        }
        break;
    case node::EXPR:
        return nd->is_function() ? *nd->expr.oper.function : EXTRA_OPERATER_CODE.string(nd->expr.oper.code, operater::NAME);
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
            left = format(STR("(%1)"), left);
        }
        if (nd->expr.right && !nd->lower_than(nd->expr.right)) {
            right = format(STR("(%1)"), right);
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

string_t handler::latex(const node* nd) {
    if (!nd) {
        return string_t();
    }

    string_t str;
    switch (nd->type) {
    case node::OBJECT: {
        switch (nd->obj.type) {
        case object::BOOLEAN:
        case object::VARIABLE:
            str = text(nd);
            break;
        case object::REAL:
        case object::IMAGINARY: {
            str = text(nd);
            for (auto& pair : {std::make_pair(object::CONST_INFINITY, STR("\\infty ")), std::make_pair(object::CONST_PI, STR("\\pi "))}) {
                for (auto& name : EXTRA_OBJECT_CONSTANT.at(pair.first)) {
                    if (replace(str, name, pair.second)) {
                        goto done;
                    }
                }
            }
        done:
            break;
        }
        case object::STRING:
            str = format(STR("``%1\""), *nd->obj.string);
            break;
        case object::PARAM:
            str = format(STR("\\left[%1\\right]"), *nd->obj.param);
            break;
        case object::ARRAY: {
            const node_array& na = *nd->obj.array;
            string_array sa(na.size());
            std::transform(na.begin(), na.end(), sa.begin(), [](const node* nd) { return latex(nd); });
            str = format(STR("\\left(%1\\right)"), join(sa, STR(",")));
            break;
        }
        }
        break;
    }
    case node::EXPR: {
        string_t simple, packed;
        if (nd->is_function()) {
            simple = STR(' ') + text(nd);
        } else {
            switch (nd->expr.oper.code) {
            case operater::AND:
                simple = STR("\\land ");
                break;
            case operater::OR:
                simple = STR("\\lor ");
                break;
            case operater::NOT:
                simple = STR("\\neg ");
                break;
            case operater::EQUAL:
                simple = STR('=');
                break;
            case operater::APPROACH:
                simple = STR("\\approx ");
                break;
            case operater::NOT_EQUAL:
                simple = STR("\\neq ");
                break;
            case operater::LESS_EQUAL:
                simple = STR("\\leq ");
                break;
            case operater::GREATER_EQUAL:
                simple = STR("\\geq ");
                break;
            case operater::MULTIPLY:
                simple = STR("\\cdot ");
                break;
            case operater::DIVIDE:
                packed = STR("\\frac{%1}{%2}");
                break;
            case operater::MODULUS:
                simple = STR("\\%");
                break;
            case operater::CEIL:
                packed = STR("\\left\\lceil %2\\right\\rceil ");
                break;
            case operater::FLOOR:
                packed = STR("\\left\\lfloor %2\\right\\rfloor ");
                break;
            case operater::TRUNC:
            case operater::ROUND:
            case operater::RINT:
            case operater::PHASE:
            case operater::REAL:
            case operater::IMAGINARY:
            case operater::CONJUGATE:
            case operater::EXP:
            case operater::LG:
            case operater::LN:
            case operater::TODEG:
            case operater::TORAD:
            case operater::SIN:
            case operater::COS:
            case operater::TAN:
            case operater::COT:
            case operater::SEC:
            case operater::CSC:
            case operater::PRIME:
            case operater::COMPOSITE:
            case operater::NTH_PRIME:
            case operater::NTH_COMPOSITE:
            case operater::RAND:
                packed = format(STR(" %1\\left(%2\\right)"), text(nd));
                break;
            case operater::ABS:
                packed = STR("\\left|%2\\right|");
                break;
            case operater::FACTORIAL:
                simple = STR('!');
                break;
            case operater::GAMMA:
                simple = STR("\\Gamma ");
                break;
            case operater::PERMUTE:
                packed = STR(" P_{%1}^{%2}");
                break;
            case operater::COMBINE:
                packed = STR(" C_{%1}^{%2}");
                break;
            case operater::POW: {
                const node* left = nd->expr.left;
                if (left && (left->is_real() || left->is_variable() ||
                             (left->is_imaginary() && (0 == left->obj.imaginary || 1 == left->obj.imaginary)))) {
                    packed = STR(" %1^{%2}");
                } else {
                    packed = STR("\\left(%1\\right)^{%2}");
                }
                break;
            }
            case operater::LOG:
                packed = STR(" log_{%1}\\left(%2\\right)");
                break;
            case operater::SQRT:
                packed = STR("\\sqrt{%2}");
                break;
            case operater::ROOT:
                packed = STR("\\sqrt[%1]{%2}");
                break;
            case operater::POLAR:
                simple = STR("\\angle ");
                break;
            case operater::DEG:
                simple = STR("^{\\circ}");
                break;
            case operater::ARCSIN:
            case operater::ARCCOS:
            case operater::ARCTAN:
            case operater::ARCCOT:
            case operater::ARCSEC:
            case operater::ARCCSC:
                packed = format(STR(" %1^{-1}\\left(%2\\right)"), text(nd).substr(1));
                break;
            case operater::SUMMATE:
            case operater::PRODUCE:
                if (nd->expr.right && nd->expr.right->is_array()) {
                    const node_array& wrap = *nd->expr.right->obj.array;
                    if (3 <= wrap.size()) {
                        string_t name = EXTRA_OPERATER_CODE.string(nd->expr.oper.code, operater::ALIAS);
                        string_t variable = wrap[2]->function_variables();
                        if (!variable.empty()) {
                            variable.replace(variable.begin() + 1, variable.end(), STR("="));
                        }
                        packed = format(STR("\\%1_{%2}^{%3}%4 "), {name, variable + latex(wrap[0]), latex(wrap[1]), latex(wrap[2])});
                    }
                }
                if (packed.empty()) {
                    simple = text(nd);
                }
                break;
            case operater::INTEGRATE:
            case operater::DOUBLE_INTEGRATE:
            case operater::TRIPLE_INTEGRATE:
                if (nd->expr.right && nd->expr.right->is_array()) {
                    const std::pair<size_t, string_t> attrs[] = {
                        {3, STR("\\int_{%1}^{%2}%3")},
                        {5, STR("\\int_{%1}^{%2}\\int_{%3}^{%4}%5")},
                        {7, STR("\\int_{%1}^{%2}\\int_{%3}^{%4}\\int_{%5}^{%6}%7")}
                    };
                    size_t index = nd->expr.oper.code - operater::INTEGRATE;
                    auto& attr = attrs[index];

                    const node_array& wrap = *nd->expr.right->obj.array;
                    if (attr.first <= wrap.size()) {
                        string_array args(wrap.size());
                        std::transform(wrap.begin(), wrap.end(), args.begin(), [](const node* nd) { return latex(nd); });
                        packed = format(attr.second, args);
                        for (char_t variable : wrap[attr.first - 1]->function_variables().substr(0, index + 1)) {
                            packed += STR("\\cdot d");
                            packed += variable;
                        }
                    }
                }
                if (packed.empty()) {
                    simple = text(nd);
                }
                break;
            default:
                simple = text(nd);
                break;
            }
        }

        string_t left = latex(nd->expr.left);
        string_t right = latex(nd->expr.right);
        if (!simple.empty()) {
            if (nd->expr.left && (nd->higher_than(nd->expr.left) || nd->expr.left->is_largescale())) {
                left = format(STR("\\left(%1\\right)"), left);
            }
            if (nd->expr.right && !nd->lower_than(nd->expr.right)) {
                right = format(STR("\\left(%1\\right)"), right);
            }
            str = left + simple + right;
        } else if (!packed.empty()) {
            str = format(packed, left, right);
        }
        break;
    }
    }

    if (nd->defines && nd->defines->is_array()) {
        const node_array& na = *nd->defines->obj.array;
        string_array sa(na.size());
        std::transform(na.begin(), na.end(), sa.begin(), [](const node* nd) { return format(STR("&%1\\\\ "), latex(nd)); });
        if (!sa.empty()) {
            str = format(STR("\\begin{align}%1&%2\\end{align}"), join(sa, string_t()), str);
        }
    }

    return str;
}

string_t handler::tree(const node* nd, size_t indent) {
    if (!nd) {
        return string_t();
    }

    string_t str = STR("─── ") + (nd->is_array() ? STR("array") : text(nd));
    if (nd->upper()) {
        node::node_side side = nd->side();
        str[0] = (node::LEFT == side ? STR('┌') : (node::TAIL == nd->pos() ? STR('└') : STR('├')));
        for (const node* ancestor = nd->upper(); ancestor; ancestor = ancestor->upper()) {
            str = STR("    ") + str;
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
        case operater::LARGESCALE:
            return calc_calls(nd, assist);
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
    case object::IMAGINARY:
        return complex_t(0, nd->obj.imaginary);
    case object::STRING:
        return *nd->obj.string;
    case object::PARAM:
        return assist.pr ? assist.pr(*nd->obj.param) : variant();
    case object::VARIABLE:
        return assist.vr ? assist.vr(nd->obj.variable) : variant();
    case object::ARRAY: {
        const node_array& na = *nd->obj.array;
        sequence_t sequence(na.size());
        std::transform(na.begin(), na.end(), sequence.begin(), [&assist](const node* nd) { return calc(nd, assist); });
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

variant handler::calc_calls(const node* nd, const calc_assist& assist) {
    if (!nd->expr.right || !nd->expr.right->is_array()) {
        return variant();
    }

    const node_array& wrap = *nd->expr.right->obj.array;
    switch (nd->expr.oper.code) {
    case operater::GENERATE:
        return calc_generate(wrap, assist);
    case operater::HAS:
    case operater::PICK:
    case operater::SELECT:
    case operater::SORT:
    case operater::TRANSFORM:
    case operater::ACCUMULATE:
        return calc_sequence(nd->expr.oper.code, wrap, assist);
    case operater::SUMMATE:
    case operater::PRODUCE:
        return calc_cumulate(nd->expr.oper.code, wrap, assist);
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

variant handler::calc_sequence(operater::operater_code code, const node_array& wrap, const calc_assist& assist) {
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
    auto sequence_vr = [&sequence](size_t index, const string_t& variables, size_t offset, char_t variable) -> variant {
        if (offset < variables.size() && variables[offset] == variable) {
            return sequence[index];
        }

        ++offset;
        if (offset < variables.size() && variables[offset] == variable) {
            return index;
        }

        return sequence;
    };

    switch (code) {
    case operater::HAS: {
        if (variables.empty()) {
            return sequence.end() != std::find(sequence.begin(), sequence.end(), arg1);
        }

        for (size_t index = 0; index < size; ++index) {
            variable_replacer vr = std::bind(sequence_vr, index, variables, 0, _1);
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
            variable_replacer vr = std::bind(sequence_vr, index, variables, 0, _1);
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
                variable_replacer vr = std::bind(sequence_vr, index, variables, 0, _1);
                if (calc_function(wrap[1], {assist.pr, vr, assist.dm}).to_boolean()) {
                    res.push_back(sequence[index]);
                }
            }
        }

        return res;
    }
    case operater::SORT: {
        std::function<bool(const variant& var1, const variant& var2)> pred;
        if (variables.size() < 2) {
            operater oper = make_operater(arg1.to_boolean() ? operater::LESS : operater::GREATER);
            pred = [oper](const variant& var1, const variant& var2) { return operate(var1, oper, var2).to_boolean(); };
        } else {
            pred = [&wrap, &assist, &variables](const variant& var1, const variant& var2) {
                variable_replacer vr = [&var1, &var2, &variables](char_t variable) { return variables[0] == variable ? var1 : var2; };
                return calc_function(wrap[1], {assist.pr, vr, assist.dm}).to_boolean();
            };
        }

        sequence_t res(sequence);
        std::sort(res.begin(), res.end(), pred);
        return res;
    }
    case operater::TRANSFORM: {
        sequence_t res(size);
        for (size_t index = 0; index < size; ++index) {
            if (variables.empty()) {
                res[index] = arg1;
            } else {
                variable_replacer vr = std::bind(sequence_vr, index, variables, 0, _1);
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

        for (size_t index = 0; index < size; ++index) {
            variable_replacer vr1 = std::bind(sequence_vr, index, variables, 1, _1);
            variable_replacer vr = [&arg2, &variables, &vr1](char_t variable) { return variables[0] == variable ? arg2 : vr1(variable); };
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

variant handler::calc_cumulate(operater::operater_code code, const node_array& wrap, const calc_assist& assist) {
    if (wrap.size() < 3 || wrap[2]->function_variables().empty()) {
        return variant();
    }

    variant res;
    operater oper;
    switch (code) {
    case operater::SUMMATE:
        res = real_t(0);
        oper = make_operater(operater::PLUS);
        break;
    case operater::PRODUCE:
        res = real_t(1);
        oper = make_operater(operater::MULTIPLY);
        break;
    default:
        return variant();
    }

    bound_t bn = calc_bound(wrap[0], wrap[1], assist, true);
    for (real_t n = bn.first; n <= bn.second; ++n) {
        res = operate(res, oper, calc_function(wrap[2], {assist.pr, [n](char_t) { return n; }, assist.dm}));
    }

    return res;
}

variant handler::calc_integrate(const node_array& wrap, const calc_assist& assist) {
    if (wrap.size() < 3 || wrap[2]->function_variables().empty()) {
        return variant();
    }

    bound_t bx = calc_bound(wrap[0], wrap[1], assist);
    real_t dx = (bx.second - bx.first) / INTEGRATE_PIECE_SIZE;

    auto integrand = [&wrap, &assist](real_t x) {
        return calc_function(wrap[2], {assist.pr, [x](char_t) { return x; }, assist.dm}).to_real();
    };

    real_t res = (integrand(bx.first) + integrand(bx.second)) * 0.5;
    for (size_t n = 1; n < INTEGRATE_PIECE_SIZE; ++n) {
        res += integrand(bx.first + dx * n);
    }

    return res * dx;
}

variant handler::calc_integrate2(const node_array& wrap, const calc_assist& assist) {
    if (wrap.size() < 5) {
        return variant();
    }

    string_t variables = wrap[4]->function_variables();
    if (variables.size() < 2) {
        return variant();
    }

    bound_t by = calc_bound(wrap[0], wrap[1], assist);
    real_t dy = (by.second - by.first) / INTEGRATE2_PIECE_SIZE;

    bound_t bx = calc_bound(wrap[2], wrap[3], assist);
    real_t dx = (bx.second - bx.first) / INTEGRATE2_PIECE_SIZE;

    auto integrand = [&wrap, &assist, &variables](real_t x, real_t y) {
        variable_replacer vr = [x, y, &variables](char_t variable) { return variables[0] == variable ? x : y; };
        return calc_function(wrap[4], {assist.pr, vr, assist.dm}).to_real();
    };

    auto adjust = [](real_t& value, size_t n) {
        if (0 == n || INTEGRATE2_PIECE_SIZE == n) {
            value *= 0.5;
        }
    };

    real_t res = 0;
    for (size_t ny = 0; ny <= INTEGRATE2_PIECE_SIZE; ++ny) {
        real_t y = by.first + dy * ny;
        for (size_t nx = 0; nx <= INTEGRATE2_PIECE_SIZE; ++nx) {
            real_t value = integrand(bx.first + dx * nx, y);
            adjust(value, nx);
            adjust(value, ny);
            res += value;
        }
    }

    return res * dx * dy;
}

variant handler::calc_integrate3(const node_array& wrap, const calc_assist& assist) {
    if (wrap.size() < 7) {
        return variant();
    }

    string_t variables = wrap[6]->function_variables();
    if (variables.size() < 3) {
        return variant();
    }

    bound_t bz = calc_bound(wrap[0], wrap[1], assist);
    real_t dz = (bz.second - bz.first) / INTEGRATE3_PIECE_SIZE;

    bound_t by = calc_bound(wrap[2], wrap[3], assist);
    real_t dy = (by.second - by.first) / INTEGRATE3_PIECE_SIZE;

    bound_t bx = calc_bound(wrap[4], wrap[5], assist);
    real_t dx = (bx.second - bx.first) / INTEGRATE3_PIECE_SIZE;

    auto integrand = [&wrap, &assist, &variables](real_t x, real_t y, real_t z) {
        variable_replacer vr = [x, y, z, &variables](char_t variable) {
            return variables[0] == variable ? x : (variables[1] == variable ? y : z);
        };
        return calc_function(wrap[6], {assist.pr, vr, assist.dm}).to_real();
    };

    auto adjust = [](real_t& value, size_t n) {
        if (0 == n || INTEGRATE3_PIECE_SIZE == n) {
            value *= 0.5;
        }
    };

    real_t res = 0;
    for (size_t nz = 0; nz <= INTEGRATE3_PIECE_SIZE; ++nz) {
        real_t z = bz.first + dz * nz;
        for (size_t ny = 0; ny <= INTEGRATE3_PIECE_SIZE; ++ny) {
            real_t y = by.first + dy * ny;
            for (size_t nx = 0; nx <= INTEGRATE3_PIECE_SIZE; ++nx) {
                real_t value = integrand(bx.first + dx * nx, y, z);
                adjust(value, nx);
                adjust(value, ny);
                adjust(value, nz);
                res += value;
            }
        }
    }

    return res * dx * dy * dz;
}

}
