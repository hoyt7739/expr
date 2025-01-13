#ifndef EXPR_HANDLER_H
#define EXPR_HANDLER_H

#include <functional>
#include "expr_defs.h"

namespace expr {

class handler {
public:
    using param_replacer = std::function<variant(const string_t& param)>;
    using variable_replacer = std::function<variant(char_t variable)>;

public:
    static node* parse(const string_t& expr);
    static bool check(const string_t& expr);
    static variant calculate(const string_t& expr,
                             const param_replacer& pr = nullptr,
                             const variable_replacer& vr = nullptr);

    static string_t text(const node* nd);
    static string_t expr(const node* nd);
    static variant calculate(const node* nd,
                             const param_replacer& pr = nullptr,
                             const variable_replacer& vr = nullptr,
                             define_map_ptr dm = nullptr);

private:
    explicit handler(const string_t& expr);

private:
    char_t get_char(bool skip_space = true);
    char_t peek_char();
    bool try_match(const string_t& text);
    bool atom_ended();
    bool finished();

    node* parse_defines();
    node* parse_atom();
    node* parse_operater(operater::operater_mode mode);
    node* parse_function();
    node* parse_object();
    node* parse_constant();
    node* parse_numeric();
    node* parse_string();
    node* parse_param();
    node* parse_variable();
    node* parse_list(bool closure);

private:
    string_t m_expr;
    int m_pos = 0;
};

}

#endif
