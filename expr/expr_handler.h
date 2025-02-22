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

#ifndef EXPR_HANDLER_H
#define EXPR_HANDLER_H

#include <functional>
#include "expr_node.h"

namespace expr {

class handler {
public:
    using param_replacer = std::function<variant(const string_t& param)>;
    using variable_replacer = std::function<variant(char_t variable)>;
    using bound_t = std::pair<real_t, real_t>;
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
    string_t latex() const;
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
    node* parse_operater(operater::operater_kind kind);
    node* parse_function();
    node* parse_object();
    node* parse_constant();
    node* parse_numeric();
    node* parse_string();
    node* parse_param();
    node* parse_variable();
    node* parse_array(bool boundary);

    static string_t text(const node* nd);
    static string_t expr(const node* nd);
    static string_t latex(const node* nd);
    static string_t tree(const node* nd, size_t indent);

    static variant calc(const node* nd, const calc_assist& assist);
    static variant calc_object(const node* nd, const calc_assist& assist);
    static variant calc_function(const node* nd, const calc_assist& assist);
    static variant calc_calls(const node* nd, const calc_assist& assist);
    static variant calc_generate(const node_array& wrap, const calc_assist& assist);
    static variant calc_sequence(operater::operater_code code, const node_array& wrap, const calc_assist& assist);
    static bound_t calc_bound(const node* lower_nd, const node* upper_nd, const calc_assist& assist, bool to_zahlen = false);
    static variant calc_cumulate(operater::operater_code code, const node_array& wrap, const calc_assist& assist);
    static variant calc_integrate(const node_array& wrap, const calc_assist& assist);
    static variant calc_integrate2(const node_array& wrap, const calc_assist& assist);
    static variant calc_integrate3(const node_array& wrap, const calc_assist& assist);

private:
    string_t m_expr;
    size_t m_pos = 0;
    node* m_root = nullptr;
};

}

#endif
