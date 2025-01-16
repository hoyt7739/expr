#ifndef EXPR_OPERATE_H
#define EXPR_OPERATE_H

#include "expr_defs.h"

namespace expr {

variant operate(const variant& left, const operater& oper, const variant& right);
variant operate(bool left, const operater& oper, bool right);
variant operate(real_t left, const operater& oper, real_t right);
variant operate(const complex_t& left, const operater& oper, const complex_t& right);
variant operate(const string_t& left, const operater& oper, const string_t& right);
variant operate(const operater& oper, const list_t& right);

}

#endif
