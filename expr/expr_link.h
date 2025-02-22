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

#ifndef EXPR_LINK_H
#define EXPR_LINK_H

#include "expr_node.h"

namespace expr {

operater make_operater(operater::operater_code code);
operater make_function(const string_t& function);
object make_boolean(bool boolean);
object make_real(real_t real);
object make_imaginary(real_t imaginary);
object make_string(const string_t& string);
object make_param(const string_t& param);
object make_variable(char_t variable);
object make_array(const node_array& array);
node* make_node(const object& obj);
node* make_node(const operater& oper);

bool link_node(node* parent, node::node_side side, node* child);
bool insert_node(node*& root, node*& semi, node*& pending, node*& current);
bool detach_node(node* nd);
bool test_link(const node* parent, node::node_side side, const node* child, define_map_ptr dm = nullptr);
bool test_node(const node* nd, define_map_ptr dm = nullptr);

}

#endif
