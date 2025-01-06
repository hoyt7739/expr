#ifndef EXPR_HANDLER_H
#define EXPR_HANDLER_H

#include <functional>
#include "expr_defs.h"

namespace expr {

class handler {
public:
    using string_list = std::vector<string_t>;
    using param_replacer = std::function<variant(const string_t&)>;

public:
    static node* parse(const string_t& expr);
    static bool check(const string_t& expr);
    static string_list params(const string_t& expr);
    static variant calculate(const string_t& expr, const param_replacer& replacer = nullptr);

    static string_t text(const node* nd);
    static string_t expr(const node* nd);
    static string_list params(const node* nd);
    static variant calculate(const node* nd, const param_replacer& replacer = nullptr);

private:
    explicit handler(const string_t& expr);

private:
    char_t get_char(bool skip_space = true);
    char_t peek_char();
    bool try_match(const string_t& text);
    bool atom_ended();
    bool finished();

    node* parse_atom();
    node* parse_operater(operater::operater_mode mode);
    node* parse_object();
    node* parse_constant();
    node* parse_numeric();
    node* parse_string();
    node* parse_param();
    node* parse_list(bool opened);

private:
    string_t m_expr;
    int m_pos = 0;
};

}

#endif
