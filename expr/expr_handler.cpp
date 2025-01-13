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

node* handler::parse(const string_t& expr) {
    handler hdl(expr);
    node* defines = hdl.parse_defines();
    node* nd = hdl.parse_atom();
    if (nd) {
        std::swap(nd->defines, defines);
    }

    if (!hdl.finished() || !nd || !test_node(nd)) {
        delete defines;
        delete nd;
        return nullptr;
    }

    return nd;
}

bool handler::check(const string_t& expr) {
    node* nd = parse(expr);
    delete nd;
    return nullptr != nd;
}

variant handler::calculate(const string_t& expr, const param_replacer& pr, const variable_replacer& vr) {
    node* nd = parse(expr);
    variant var = calculate(nd, pr, vr);
    delete nd;
    return var;
}

string_t handler::text(const node* nd) {
    if (!nd) {
        return string_t();
    }

    switch (nd->type) {
    case node::OBJECT:
        switch (nd->obj.type) {
        case object::BOOLEAN:
            return nd->obj.boolean ? STR("true") : STR("false");
        case object::REAL:
            return to_string(nd->obj.real);
        case object::COMPLEX: {
            real_t real = nd->obj.complex->real();
            real_t imag = nd->obj.complex->imag();
            if (!imag) {
                return to_string(real);
            }
            if (!real) {
                return to_string(imag) + STR('i');
            }
            return to_string(real) + STR('+') + to_string(imag) + STR('i');
        }
        case object::STRING:
            return STR('\"') + *nd->obj.string + STR('\"');
        case object::PARAM:
            return STR('[') + *nd->obj.param + STR(']');
        case object::VARIABLE:
            return string_t(1, nd->obj.variable);
        case object::LIST: {
            string_t str;
            for (node* item : *nd->obj.list) {
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
            return EXTRA_LOGIC_OPERATER.at(nd->expr.oper.logic).at(2);
        case operater::COMPARE:
            return EXTRA_COMPARE_OPERATER.at(nd->expr.oper.compare).at(2);
        case operater::ARITHMETIC:
            return EXTRA_ARITHMETIC_OPERATER.at(nd->expr.oper.arithmetic).at(2);
        case operater::STATISTIC:
            return EXTRA_STATISTIC_OPERATER.at(nd->expr.oper.statistic).at(2);
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
            return defines_str + str;
        }
    }

    return str;
}

variant handler::calculate(const node* nd, const param_replacer& pr, const variable_replacer& vr, define_map_ptr dm) {
    if (!nd) {
        return variant();
    }

    if (!dm) {
        dm = nd->define_map();
    }

    switch (nd->type) {
    case node::OBJECT:
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
            if (pr) {
                return pr(*nd->obj.param);
            }
            break;
        case object::VARIABLE:
            if (vr) {
                return vr(nd->obj.variable);
            }
            break;
        case object::LIST: {
            list_t list(nd->obj.list->size());
            std::transform(nd->obj.list->begin(), nd->obj.list->end(), list.begin(),
                           [&pr, &vr, &dm](node* nd) { return calculate(nd, pr, vr, dm); });
            return list;
        }
        }
        break;
    case node::EXPR:
        switch (nd->expr.oper.type) {
        case operater::FUNCTION: {
            if (!dm) {
                return variant();
            }

            auto iter = dm->find(*nd->expr.oper.function);
            if (dm->end() == iter) {
                return variant();
            }

            variant var = calculate(nd->expr.right, pr, vr, dm);
            if (variant::LIST != var.type) {
                return variant();
            }

            const string_t& variables = iter->second.first;
            const node* rule = iter->second.second;
            const list_t& values = *var.list;
            return calculate(rule, pr, [&variables, &values](char_t variable) {
                size_t pos = variables.find(variable);
                return string_t::npos != pos && pos < values.size() ? values[pos] : variant();
            }, dm);
        }
        default:
            return operate(calculate(nd->expr.left, pr, vr, dm), nd->expr.oper, calculate(nd->expr.right, pr, vr, dm));
        }
        break;
    }

    return variant();
}

handler::handler(const string_t& expr) : m_expr(expr), m_pos(0) {
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
    int pos = m_pos;
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

    node* nd = parse_list(false);

    if (!try_match(STR("}")) || !nd || nd->obj.list->empty()) {
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
                    node* list_node = parse_list(false);
                    if (!list_node) {
                        goto failed;
                    }

                    list_node->obj.list->insert(list_node->obj.list->begin(), pending);
                    pending = list_node;
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

                    bool is_eval = current->is_eval_expr();
                    if (!insert_node(root, semi, pending, current)) {
                        goto failed;
                    }

                    if (is_eval) {
                        pending = parse_list(true);
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

node* handler::parse_operater(operater::operater_mode mode) {
    char_t ch = get_char();
    switch (mode) {
    case operater::UNARY:
        switch (ch) {
        case STR('!'):
            return make_node(make_logic(operater::NOT));
        case STR('-'):
            return make_node(make_arithmetic(operater::NEGATIVE));
        case STR('A'):
            return make_node(make_statistic(operater::ARRANGEMENT));
        case STR('C'):
            return make_node(make_statistic(operater::COMBINATION));
        case STR('a'):
            if (try_match(STR("bs"))) {
                return make_node(make_arithmetic(operater::ABS));
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
            if (try_match(STR("mp"))) {
                return make_node(make_arithmetic(operater::AMPLITUDE));
            }
            if (try_match(STR("ng"))) {
                return make_node(make_arithmetic(operater::ANGLE));
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
            if (try_match(STR("vg"))) {
                return make_node(make_statistic(operater::AVERAGE));
            }
            break;
        case STR('c'):
            if (try_match(STR("eil"))) {
                return make_node(make_arithmetic(operater::CEIL));
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
        case STR('l'):
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
        case STR('r'):
            if (try_match(STR("ange"))) {
                return make_node(make_statistic(operater::RANGE));
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
            if (try_match(STR("in"))) {
                return make_node(make_arithmetic(operater::SIN));
            }
            if (try_match(STR("um"))) {
                return make_node(make_statistic(operater::SUM));
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
            if (try_match(STR("runc"))) {
                return make_node(make_arithmetic(operater::TRUNC));
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
        case STR('√'):
            return make_node(make_arithmetic(operater::SQRT));
        }
        break;
    case operater::BINARY:
        switch (ch) {
        case STR('!'):
            if (try_match(STR("="))) {
                return make_node(make_compare(operater::NOT_EQUAL));
            }
            break;
        case STR('#'):
        case STR('±'):
            return make_node(make_arithmetic(try_match(STR("%")) ? operater::EXPAND_PERCENT : operater::EXPAND));
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
        case STR('?'):
            if (try_match(STR("="))) {
                return make_node(make_compare(operater::REGULAR_MATCH));
            }
            break;
        case STR('^'):
            return make_node(make_arithmetic(operater::POW));
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
        break;
    }

    if (ch) {
        --m_pos;
    }

    return operater::UNARY == mode ? parse_function() : nullptr;
}

node* handler::parse_function() {
    int pos = m_pos;
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
    int pos = m_pos;
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

    real_t real = to_real(str);
    return make_node(is_imag ? make_complex(0, real) : make_real(real));
}

node* handler::parse_string() {
    if (!try_match(STR("\""))) {
        return nullptr;
    }

    string_t str;
    bool closed = false;
    char_t ch;
    while (ch = get_char(false)) {
        if (STR('\"') == ch) {
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

    if (str.empty() || !closed) {
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

node* handler::parse_list(bool closure) {
    if (closure && !try_match(STR("("))) {
        return nullptr;
    }

    node_list list;
    node* item = nullptr;
    do {
        item = parse_atom();
        list.push_back(item);
    } while (item && try_match(STR(",")));

    if ((closure && !try_match(STR(")"))) || !item) {
        for (node* item : list) {
            delete item;
        }
        return nullptr;
    }

    return make_node(make_list(list));
}

}
