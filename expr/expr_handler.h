#ifndef EXPR_HANDLER_H
#define EXPR_HANDLER_H

#include <functional>
#include "expr_defs.h"

namespace expr {

class handler {
public:
    using param_replacer = std::function<variant(const string_t& param)>;
    using variable_replacer = std::function<variant(char_t variable)>;
    struct calc_assist {
        param_replacer pr;
        variable_replacer vr;
        mutable define_map_ptr dm;
    };

public:
    explicit handler(const string_t& expr);
    handler(const handler& other) = delete;
    handler(handler&& other) noexcept;
    ~handler();

    handler& operator=(const handler& other) = delete;
    handler& operator=(handler&& other) noexcept;

public:
    bool is_valid(size_t* failed_pos = nullptr) const;
    string_t expr() const;
    string_t tree(size_t indent = 0) const;
    variant calc(const calc_assist& assist = calc_assist()) const;

private:
    char_t get_char(bool skip_space = true);
    char_t peek_char();
    bool try_match(const string_t& text);
    bool atom_ended();
    bool finished();

    node* parse_defines();
    node* parse_atom();
    node* parse_unary();
    node* parse_binary();
    node* parse_function();
    node* parse_object();
    node* parse_constant();
    node* parse_numeric();
    node* parse_string();
    node* parse_param();
    node* parse_variable();
    node* parse_list(bool boundary);

    static string_t text(const node* nd);
    static string_t expr(const node* nd);
    static string_t tree(const node* nd, size_t indent);

    static variant calc(const node* nd, const calc_assist& assist);
    static variant calc_object(const node* nd, const calc_assist& assist);
    static variant calc_function(const node* nd, const calc_assist& assist);
    static variant calc_invocation(const node* nd, const calc_assist& assist);
    static variant calc_sequence(operater::invocation_operater invocation, const node_list& wrap, const calc_assist& assist);
    static variant calc_generate(const node_list& wrap, const calc_assist& assist);

private:
    string_t m_expr;
    size_t m_pos = 0;
    node* m_root = nullptr;
};

}

#endif
