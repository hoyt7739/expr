#ifndef EXPR_LINK_H
#define EXPR_LINK_H

#include "expr_defs.h"

namespace expr {

operater make_logic(operater::logic_operater logic);
operater make_compare(operater::compare_operater compare);
operater make_arithmetic(operater::arithmetic_operater arithmetic);
operater make_statistic(operater::statistic_operater statistic);
operater make_invocation(operater::invocation_operater invocation);
operater make_function(const string_t& function);
object make_boolean(bool boolean);
object make_real(real_t real);
object make_complex(real_t real, real_t imag);
object make_string(const string_t& string);
object make_param(const string_t& param);
object make_variable(char_t variable);
object make_list(const node_list& list);
node* make_node(const object& obj);
node* make_node(const operater& oper);

bool link_node(node* parent, node::node_side side, node* child);
bool insert_node(node*& root, node*& semi, node*& pending, node*& current);
bool detach_node(node* nd);
bool test_link(const node* parent, node::node_side side, const node* child, define_map_ptr dm = nullptr);
bool test_node(const node* nd, define_map_ptr dm = nullptr);

}

#endif
