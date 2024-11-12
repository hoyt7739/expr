#include "expr_handler.h"
#include "expr_maker.h"
#include "expr_operate.h"

namespace expr {

enum class parse_state {
    SEGMENT_OPENING,
    SEGMENT_CLOSED
};

inline bool test_link(const node* parent, node::node_side side, const node* child) {
    if (!parent || node::EXPR != parent->type) {
        return false;
    }

    if (!child) {
        return node::LEFT == side && operater::UNARY == parent->expr.oper->mode;
    }

    switch (parent->expr.oper->type) {
        case operater::LOGIC: {
            return child->is_boolean_result();
        }
        case operater::EVALUATION: {
            return child->is_list();
        }
    }

    return child->is_value_result() || child->is_list();
}

inline bool test_node(const node* nd) {
    if (!nd) {
        return true;
    }

    switch (nd->type) {
        case node::OBJECT: {
            if (object::LIST == nd->obj->type) {
                for (node* item : *nd->obj->list) {
                    if (!test_node(item)) {
                        return false;
                    }
                }
            }
            return true;
        }
        case node::EXPR: {
            return test_link(nd, node::LEFT, nd->expr.left) &&
                   test_link(nd, node::RIGHT, nd->expr.right) &&
                   test_node(nd->expr.left) &&
                   test_node(nd->expr.right);
        }
    }

    return false;
}

inline bool link_node(node* parent, node::node_side side, node* child) {
    if (!parent || node::EXPR != parent->type) {
        return false;
    }

    if (operater::UNARY == parent->expr.oper->mode && node::LEFT == side) {
        return !child;
    }

    switch (side) {
        case node::LEFT: {
            parent->expr.left = child;
            break;
        }
        case node::RIGHT: {
            parent->expr.right = child;
            break;
        }
        default: {
            return false;
        }
    }

    child->parent = parent;

    return true;
}

inline bool insert_node(node*& root, node*& semi, node*& pending, node*& current) {
#define CHECK_VALID(cond) \
    if (!(cond)) {        \
        return false;     \
    }

    if (!semi) {
        if (!current) {
            root = pending;
            pending = nullptr;
            return true;
        }

        CHECK_VALID(link_node(current, node::LEFT, pending));
        root = current;
        semi = current;
        pending = nullptr;
        current = nullptr;
        return true;
    }

    if (!current) {
        CHECK_VALID(link_node(semi, node::RIGHT, pending));
        pending = nullptr;
        return true;
    }

    CHECK_VALID(node::EXPR == semi->type && node::EXPR == current->type);

    if (current->expr.oper->priority < semi->expr.oper->priority || operater::UNARY == current->expr.oper->mode) {
        CHECK_VALID(link_node(current, node::LEFT, pending));
        pending = nullptr;

        CHECK_VALID(link_node(semi, node::RIGHT, current));
        semi = current;
        current = nullptr;
        return true;
    } else {
        CHECK_VALID(link_node(semi, node::RIGHT, pending));
        pending = nullptr;

        node* ancestor = semi->parent;
        while (ancestor && ancestor->expr.oper->priority <= current->expr.oper->priority) {
            ancestor = ancestor->parent;
        }

        if (ancestor) {
            CHECK_VALID(link_node(current, node::LEFT, ancestor->expr.right));
            CHECK_VALID(link_node(ancestor, node::RIGHT, current));
            semi = current;
            current = nullptr;
            return true;
        } else {
            CHECK_VALID(link_node(current, node::LEFT, root));
            root = current;
            semi = current;
            current = nullptr;
            return true;
        }
    }

    CHECK_VALID(false);

#undef CHECK_VALID
}

node* handler::parse(const string_t& expr) {
    handler hdl(expr);
    node* nd = hdl.parse_atom();
    if (!hdl.finished() || !test_node(nd)) {
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

handler::string_list handler::params(const string_t& expr) {
    node* nd = parse(expr);
    string_list list = params(nd);
    delete nd;
    return list;
}

string_t handler::convert(const string_t& expr, const param_convertor& convertor) {
    if (!convertor) {
        return expr;
    }

    auto replace = [](string_t& str, const string_t& before, const string_t& after) {
        for (size_t pos = 0; string_t::npos != (pos = str.find(before, pos)); pos += after.size()) {
            str.replace(pos, before.size(), after);
        }
    };

    node* nd = parse(expr);
    string_list list = params(nd);
    delete nd;

    string_t converted = expr;
    for (string_t& param : list) {
        string_t before = STR('[') + param + STR(']');
        variant var = convertor(param);
        switch (var.type) {
            case variant::STRING: {
                replace(converted, before, STR('[') + *var.string + STR(']'));
                break;
            }
            case variant::LIST: {
                if (1 == var.list->size()) {
                    replace(converted, before, STR('[') + var.list->front().to_string() + STR(']'));
                } else if (1 < var.list->size()) {
                    string_t after;
                    for (variant& item : *var.list) {
                        if (!after.empty()) {
                            after += STR(',');
                        }
                        after += STR('[') + item.to_string() + STR(']');
                    }
                    replace(converted, before, STR('(') + after + STR(')'));
                }
                break;
            }
        }
    }

    return converted;
}

variant handler::calculate(const string_t& expr, const param_calculator& calculator) {
    node* nd = parse(expr);
    variant var = calculate(nd, calculator);
    delete nd;
    return var;
}

string_t handler::text(const node* nd) {
    if (!nd) {
        return string_t();
    }

    switch (nd->type) {
        case node::OBJECT: {
            switch (nd->obj->type) {
                case object::BOOLEAN: {
                    return nd->obj->boolean ? STR("true") : STR("false");
                }
                case object::INTEGER: {
                    return to_string(nd->obj->integer);
                }
                case object::REAL: {
                    return to_string(nd->obj->real);
                }
                case object::COMPLEX: {
                    real_t real = nd->obj->complex->real();
                    real_t imag = nd->obj->complex->imag();
                    if (!imag) {
                        return to_string(real);
                    }
                    if (!real) {
                        return to_string(imag) + STR('i');
                    }
                    return to_string(real) + STR('+') + to_string(imag) + STR('i');
                }
                case object::STRING: {
                    return STR('\"') + *nd->obj->string + STR('\"');
                }
                case object::PARAM: {
                    return STR('[') + *nd->obj->param + STR(']');
                }
                case object::LIST: {
                    string_t str;
                    for (node* item : *nd->obj->list) {
                        if (!str.empty()) {
                            str += STR(',');
                        }
                        str += expr(item);
                    }
                    return STR('(') + str + STR(')');
                }
            }
            break;
        }
        case node::EXPR: {
            switch (nd->expr.oper->type) {
                case operater::ARITHMETIC: {
                    return EXTRA_ARITHMETIC_OPERATER.at(nd->expr.oper->arithmetic).at(2);
                }
                case operater::COMPARE: {
                    return EXTRA_COMPARE_OPERATER.at(nd->expr.oper->compare).at(2);
                }
                case operater::LOGIC: {
                    return EXTRA_LOGIC_OPERATER.at(nd->expr.oper->logic).at(2);
                }
                case operater::EVALUATION: {
                    return EXTRA_EVALUATION_OPERATER.at(nd->expr.oper->evaluation).at(2);
                }
            }
            break;
        }
    }

    return string_t();
}

string_t handler::expr(const node* nd) {
    if (!nd) {
        return string_t();
    }

    string_t str = text(nd);

    switch (nd->type) {
        case node::OBJECT: {
            return str;
        }
        case node::EXPR: {
            string_t left = expr(nd->expr.left);
            string_t right = expr(nd->expr.right);
            if (nd->expr.left && node::EXPR == nd->expr.left->type &&
                nd->expr.oper->priority < nd->expr.left->expr.oper->priority) {
                left = STR('(') + left + STR(')');
            }
            if (nd->expr.right && node::EXPR == nd->expr.right->type &&
                nd->expr.oper->priority <= nd->expr.right->expr.oper->priority) {
                right = STR('(') + right + STR(')');
            }
            return left + str + right;
        }
    }

    return string_t();
}

handler::string_list handler::params(const node* nd) {
    if (!nd) {
        return string_list();
    }

    auto merge = [](const string_list& left, const string_list& right) {
        string_list list(left);
        list.insert(list.end(), right.begin(), right.end());
        return list;
    };

    switch (nd->type) {
        case node::OBJECT: {
            switch (nd->obj->type) {
                case object::PARAM: {
                    return string_list({*nd->obj->param});
                }
                case object::LIST: {
                    return std::accumulate(nd->obj->list->begin(), nd->obj->list->end(), string_list(),
                        [&merge](const string_list& list, node* nd) { return merge(list, params(nd)); });
                }
            }
            break;
        }
        case node::EXPR: {
            return merge(params(nd->expr.left), params(nd->expr.right));
        }
    }

    return string_list();
}

variant handler::calculate(const node* nd, const param_calculator& calculator) {
    if (!nd) {
        return variant();
    }

    switch (nd->type) {
        case node::OBJECT: {
            switch (nd->obj->type) {
                case object::BOOLEAN: {
                    return nd->obj->boolean;
                }
                case object::INTEGER: {
                    return nd->obj->integer;
                }
                case object::REAL: {
                    return nd->obj->real;
                }
                case object::COMPLEX: {
                    return *nd->obj->complex;
                }
                case object::STRING: {
                    return *nd->obj->string;
                }
                case object::PARAM: {
                    if (calculator) {
                        return calculator(*nd->obj->param);
                    }
                    break;
                }
                case object::LIST: {
                    list_t list(nd->obj->list->size());
                    std::transform(nd->obj->list->begin(), nd->obj->list->end(), list.begin(),
                                   [&calculator](node* nd) { return calculate(nd, calculator); });
                    return list;
                }
            }
            break;
        }
        case node::EXPR: {
            return operate(calculate(nd->expr.left, calculator), *nd->expr.oper, calculate(nd->expr.right, calculator));
        }
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
    return !ch || STR(')') == ch || STR(',') == ch;
}

bool handler::finished() {
    char_t ch = peek_char();
    return !ch;
}

node* handler::parse_atom() {
#define CHECK_VALID(cond) \
    if (!(cond)) {        \
        delete root;      \
        delete pending;   \
        delete current;   \
        return nullptr;   \
    }

    node* root = nullptr;
    node* semi = nullptr;
    node* pending = nullptr;
    node* current = nullptr;
    parse_state state = parse_state::SEGMENT_OPENING;
    while (true) {
        switch (state) {
            case parse_state::SEGMENT_OPENING: {
                if (try_match(STR("("))) {
                    CHECK_VALID(pending = parse_atom());
                    if (try_match(STR(","))) {
                        node* list_node = parse_list(true);
                        CHECK_VALID(list_node);
                        list_node->obj->list->insert(list_node->obj->list->begin(), pending);
                        pending = list_node;
                    } else {
                        CHECK_VALID(try_match(STR(")")));
                    }
                    state = parse_state::SEGMENT_CLOSED;
                } else {
                    current = parse_operater(operater::UNARY);
                    if (current) {
                        bool is_evaluation = operater::EVALUATION == current->expr.oper->type;
                        CHECK_VALID(insert_node(root, semi, pending, current));
                        if (is_evaluation) {
                            CHECK_VALID(pending = parse_list(false));
                            state = parse_state::SEGMENT_CLOSED;
                        }
                    } else {
                        CHECK_VALID(pending = parse_object());
                        state = parse_state::SEGMENT_CLOSED;
                    }
                }
                break;
            }
            case parse_state::SEGMENT_CLOSED: {
                if (atom_ended()) {
                    CHECK_VALID(insert_node(root, semi, pending, current = nullptr));
                    return root;
                }
                CHECK_VALID(current = parse_operater(operater::BINARY));
                CHECK_VALID(insert_node(root, semi, pending, current));
                state = parse_state::SEGMENT_OPENING;
                break;
            }
            default: {
                CHECK_VALID(false);
            }
        }
    }

    CHECK_VALID(false);

#undef CHECK_VALID
}

node* handler::parse_operater(operater::operater_mode mode) {
    operater* oper = nullptr;
    char_t ch = get_char();
    switch (mode) {
        case operater::UNARY: {
            switch (ch) {
                case STR('!'): {
                    oper = make_logic(operater::NOT);
                    break;
                }
                case STR('-'): {
                    oper = make_arithmetic(operater::NEGATIVE);
                    break;
                }
                case STR('a'): {
                    if (try_match(STR("bs"))) {
                        oper = make_arithmetic(operater::ABS);
                    } else if (try_match(STR("mp"))) {
                        oper = make_arithmetic(operater::AMP);
                    } else if (try_match(STR("rg"))) {
                        oper = make_arithmetic(operater::ARG);
                    } else if (try_match(STR("vg"))) {
                        oper = make_evaluation(operater::AVERAGE);
                    }
                    break;
                }
                case STR('c'): {
                    if (try_match(STR("eil"))) {
                        oper = make_arithmetic(operater::CEIL);
                    } else if (try_match(STR("os"))) {
                        oper = make_arithmetic(operater::COS);
                    } else if (try_match(STR("ot"))) {
                        oper = make_arithmetic(operater::COT);
                    }
                    break;
                }
                case STR('d'): {
                    if (try_match(STR("eg"))) {
                        oper = make_arithmetic(operater::DEG);
                    } else if (try_match(STR("ev"))) {
                        oper = make_evaluation(operater::DEVIATION);
                    }
                    break;
                }
                case STR('e'): {
                    if (try_match(STR("xp"))) {
                        oper = make_arithmetic(operater::EXP);
                    }
                    break;
                }
                case STR('f'): {
                    if (try_match(STR("loor"))) {
                        oper = make_arithmetic(operater::FLOOR);
                    }
                    break;
                }
                case STR('l'): {
                    if (try_match(STR("g"))) {
                        oper = make_arithmetic(operater::LG);
                    } else if (try_match(STR("n"))) {
                        oper = make_arithmetic(operater::LN);
                    }
                    break;
                }
                case STR('m'): {
                    if (try_match(STR("ax"))) {
                        oper = make_evaluation(operater::MAX);
                    } else if (try_match(STR("ed"))) {
                        oper = make_evaluation(operater::MEDIAN);
                    } else if (try_match(STR("in"))) {
                        oper = make_evaluation(operater::MIN);
                    } else if (try_match(STR("ode"))) {
                        oper = make_evaluation(operater::MODE);
                    }
                    break;
                }
                case STR('r'): {
                    if (try_match(STR("ad"))) {
                        oper = make_arithmetic(operater::RAD);
                    } else if (try_match(STR("ange"))) {
                        oper = make_evaluation(operater::RANGE);
                    } else if (try_match(STR("ound"))) {
                        oper = make_arithmetic(operater::ROUND);
                    } else if (try_match(STR("t"))) {
                        oper = make_arithmetic(operater::SQRT);
                    }
                    break;
                }
                case STR('s'): {
                    if (try_match(STR("in"))) {
                        oper = make_arithmetic(operater::SIN);
                    } else if (try_match(STR("um"))) {
                        oper = make_evaluation(operater::SUM);
                    }
                    break;
                }
                case STR('t'): {
                    if (try_match(STR("an"))) {
                        oper = make_arithmetic(operater::TAN);
                    }
                    break;
                }
                case STR('v'): {
                    if (try_match(STR("ar"))) {
                        oper = make_evaluation(operater::VARIANCE);
                    }
                    break;
                }
                case STR('√'): {
                    oper = make_arithmetic(operater::SQRT);
                    break;
                }
            }
            break;
        }
        case operater::BINARY: {
            switch (ch) {
                case STR('!'): {
                    if (try_match(STR("="))) {
                        oper = make_compare(operater::NOT_EQUAL);
                    }
                    break;
                }
                case STR('#'):
                case STR('±'): {
                    oper = make_arithmetic(try_match(STR("%")) ? operater::EXPAND_PERCENT : operater::EXPAND);
                    break;
                }
                case STR('%'): {
                    oper = make_arithmetic(operater::MOD);
                    break;
                }
                case STR('&'): {
                    try_match(STR("&"));
                    oper = make_logic(operater::AND);
                    break;
                }
                case STR('*'): {
                    oper = make_arithmetic(operater::MULTIPLY);
                    break;
                }
                case STR('+'): {
                    oper = make_arithmetic(operater::PLUS);
                    break;
                }
                case STR('-'): {
                    oper = make_arithmetic(operater::MINUS);
                    break;
                }
                case STR('/'): {
                    oper = make_arithmetic(operater::DIVIDE);
                    break;
                }
                case STR('<'): {
                    oper = make_compare(try_match(STR("=")) ? operater::LESS_EQUAL : operater::LESS);
                    break;
                }
                case STR('='): {
                    try_match(STR("="));
                    oper = make_compare(operater::EQUAL);
                    break;
                }
                case STR('>'): {
                    oper = make_compare(try_match(STR("=")) ? operater::GREATER_EQUAL : operater::GREATER);
                    break;
                }
                case STR('?'): {
                    if (try_match(STR("="))) {
                        oper = make_compare(operater::REGULAR_MATCH);
                    }
                    break;
                }
                case STR('^'): {
                    oper = make_arithmetic(operater::POW);
                    break;
                }
                case STR('l'): {
                    if (try_match(STR("og"))) {
                        oper = make_arithmetic(operater::LOG);
                    }
                    break;
                }
                case STR('r'): {
                    if (try_match(STR("t"))) {
                        oper = make_arithmetic(operater::ROOT);
                    }
                    break;
                }
                case STR('v'): {
                    if (try_match(STR("ec"))) {
                        oper = make_arithmetic(operater::VECTOR);
                    }
                    break;
                }
                case STR('|'): {
                    try_match(STR("|"));
                    oper = make_logic(operater::OR);
                    break;
                }
                case STR('~'): {
                    if (try_match(STR("="))) {
                        oper = make_compare(operater::APPROACH);
                    }
                    break;
                }
                case STR('√'): {
                    oper = make_arithmetic(operater::ROOT);
                    break;
                }
                case STR('∠'): {
                    oper = make_arithmetic(operater::VECTOR);
                    break;
                }
            }
            break;
        }
    }

    if (!oper && ch) {
        --m_pos;
    }

    return oper ? make_node(oper) : nullptr;
}

node* handler::parse_object() {
    node* nd = parse_constant();
    if (nd) {
        return nd;
    }

    nd = parse_numeric();
    if (nd) {
        return nd;
    }

    nd = parse_string();
    if (nd) {
        return nd;
    }

    return parse_param();
}

node* handler::parse_constant() {
    object* obj = nullptr;
    if (try_match(STR("false"))) {
        obj = make_boolean(false);
    } else if (try_match(STR("true"))) {
        obj = make_boolean(true);
    } else if (try_match(STR("π")) || try_match(STR("pi"))) {
        obj = make_real(PI);
    } else if (try_match(STR("e"))) {
        obj = make_real(NATURAL);
    } else if (try_match(STR("rand"))) {
        static unsigned s = 0;
        if (!s) {
            s = (unsigned)time(nullptr);
            srand(s);
        }
        obj = make_integer(rand());
    }

    return obj ? make_node(obj) : nullptr;
}

node* handler::parse_numeric() {
    string_t str;
    char_t ch;
    while (ch = get_char()) {
        if (STR('0') <= ch && ch <= STR('9') || STR('.') == ch || STR('i') == ch) {
            str += ch;
        } else {
            break;
        }
    }

    if (ch) {
        --m_pos;
    }

    if (str.empty()) {
        return nullptr;
    }

    bool is_real;
    switch (std::count(str.begin(), str.end(), STR('.'))) {
        case 0: {
            is_real = false;
            break;
        }
        case 1: {
            is_real = true;
            break;
        }
        default: {
            return nullptr;
        }
    }

    bool is_imag;
    switch (std::count(str.begin(), str.end(), STR('i'))) {
        case 0: {
            is_imag = false;
            break;
        }
        case 1: {
            if (STR('i') != str.back()) {
                return nullptr;
            }
            is_imag = true;
            str.pop_back();
            break;
        }
        default: {
            return nullptr;
        }
    }

    object* obj = nullptr;
    if (is_imag) {
        obj = make_complex(0.0, std::stod(str));
    } else {
        if (is_real) {
            obj = make_real(std::stod(str));
        } else {
            obj = make_integer(std::stoll(str));
        }
    }

    return obj ? make_node(obj) : nullptr;
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

    object* obj = make_string(str);
    return obj ? make_node(obj) : nullptr;
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

    object* obj = make_param(str);
    return obj ? make_node(obj) : nullptr;
}

node* handler::parse_list(bool opened) {
    if (!opened && !try_match(STR("("))) {
        return nullptr;
    }

    node_list list;
    bool ok = false;
    do {
        node* nd = parse_atom();
        ok = nd && nd->is_value_result();
        if (ok) {
            list.push_back(nd);
        }
    } while (ok && try_match(STR(",")));

    if (!ok || !try_match(STR(")"))) {
        for (node* nd : list) {
            delete nd;
        }
        return nullptr;
    }

    object* obj = make_list(list);
    return obj ? make_node(obj) : nullptr;
}

}
