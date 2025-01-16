#include "expr_operate.h"
#include <map>
#include <algorithm>
#include <numeric>
#include <regex>

namespace expr {

inline real_t factorial(real_t value) {
    real_t res = 1;
    for (real_t i = 1; i <= value; ++i) {
        res *= i;
    }

    return res;
}

variant operate(const variant& left, const operater& oper, const variant& right) {
    switch (oper.type) {
    case operater::LOGIC:
        return operate(left.to_boolean(), oper, right.to_boolean());
    case operater::COMPARE:
    case operater::ARITHMETIC:
        switch (right.type) {
        case variant::REAL:
            switch (left.type) {
            case variant::REAL:
                return operate(left.real, oper, right.real);
            case variant::COMPLEX:
                return operate(*left.complex, oper, right.to_complex());
            default:
                if (operater::UNARY == oper.mode && !oper.postpose) {
                    return operate(real_t(), oper, right.real);
                }
                break;
            }
            break;
        case variant::COMPLEX:
            switch (left.type) {
            case variant::REAL:
                return operate(left.to_complex(), oper, *right.complex);
            case variant::COMPLEX:
                return operate(*left.complex, oper, *right.complex);
            default:
                if (operater::UNARY == oper.mode && !oper.postpose) {
                    return operate(complex_t(), oper, *right.complex);
                }
                break;
            }
            break;
        case variant::STRING:
            switch (left.type) {
            case variant::STRING:
                return operate(*left.string, oper, *right.string);
            }
            break;
        default:
            if (operater::UNARY == oper.mode && oper.postpose) {
                switch (left.type) {
                case variant::REAL:
                    return operate(left.real, oper, real_t());
                case variant::COMPLEX:
                    return operate(*left.complex, oper, complex_t());
                }
            }
            break;
        }
        break;
    case operater::STATISTIC:
        switch (right.type) {
        case variant::LIST:
            return operate(oper, *right.list);
        }
        break;
    }

    return variant();
}

variant operate(bool left, const operater& oper, bool right) {
    if (operater::LOGIC == oper.type) {
        switch (oper.logic) {
        case operater::AND:
            return left && right;
        case operater::OR:
            return left || right;
        case operater::NOT:
            return !right;
        }
    }

    return variant();
}

variant operate(real_t left, const operater& oper, real_t right) {
    switch (oper.type) {
    case operater::COMPARE:
        switch (oper.compare) {
        case operater::LESS:
            return left < right;
        case operater::LESS_EQUAL:
            return left <= right;
        case operater::EQUAL:
            return left == right;
        case operater::APPROACH:
            return fabs(left - right) < EPSILON;
        case operater::NOT_EQUAL:
            return left != right;
        case operater::GREATER_EQUAL:
            return left >= right;
        case operater::GREATER:
            return left > right;
        }
        break;
    case operater::ARITHMETIC:
        switch (oper.arithmetic) {
        case operater::PLUS:
            return left + right;
        case operater::MINUS:
            return left - right;
        case operater::MULTIPLY:
            return left * right;
        case operater::DIVIDE:
            return 0 == right ? variant() : left / right;
        case operater::MOD:
            return 0 == right ? variant() : fmod(left, right);
        case operater::NEGATIVE:
            return -right;
        case operater::ABS:
            return fabs(right);
        case operater::CEIL:
            return ceil(right);
        case operater::FLOOR:
            return floor(right);
        case operater::TRUNC:
            return trunc(right);
        case operater::ROUND:
            return round(right);
        case operater::RINT:
            return rint(right);
        case operater::FACTORIAL:
            return left < 0 ? variant() : factorial(floor(left));
        case operater::POW:
            return pow(left, right);
        case operater::EXP:
            return exp(right);
        case operater::LOG:
            if (left <= 0 || left == 1 || right <= 0) {
                return variant();
            }
            if (2 == left) {
                return log2(right);
            }
            if (10 == left) {
                return log10(right);
            }
            if (CONST_E == left) {
                return log(right);
            }
            return log(right) / log(left);
        case operater::LG:
            return right <= 0 ? variant() : log10(right);
        case operater::LN:
            return right <= 0 ? variant() : log(right);
        case operater::SQRT:
            return right < 0 ? variant() : sqrt(right);
        case operater::ROOT:
            return left <= 0 ? variant() : pow(right, 1 / left);
        case operater::DEG:
            return left * CONST_PI / 180;
        case operater::TODEG:
            return right * 180 / CONST_PI;
        case operater::TORAD:
            return right * CONST_PI / 180;
        case operater::SIN:
            return sin(right);
        case operater::ARCSIN:
            return right < -1 || 1 < right ? variant() : asin(right);
        case operater::COS:
            return cos(right);
        case operater::ARCCOS:
            return right < -1 || 1 < right ? variant() : acos(right);
        case operater::TAN:
            return is_zahlen(right / CONST_PI - 0.5) ? variant() : tan(right);
        case operater::ARCTAN:
            return atan(right);
        case operater::COT:
            return is_zahlen(right / CONST_PI) ? variant() : cos(right) / sin(right);
        case operater::ARCCOT:
            return CONST_PI / 2 - atan(right);
        case operater::SEC:
            return is_zahlen(right / CONST_PI - 0.5) ? variant() : 1 / cos(right);
        case operater::ARCSEC:
            return -1 < right && right < 1 ? variant() : acos(1 / right);
        case operater::CSC:
            return is_zahlen(right / CONST_PI) ? variant() : 1 / sin(right);
        case operater::ARCCSC:
            return -1 < right && right < 1 ? variant() : asin(1 / right);
        case operater::VECTOR:
            return std::polar(left, right);
        }
        break;
    }

    return variant();
}

variant operate(const complex_t& left, const operater& oper, const complex_t& right) {
    if (operater::ARITHMETIC == oper.type) {
        switch (oper.arithmetic) {
        case operater::PLUS:
            return left + right;
        case operater::MINUS:
            return left - right;
        case operater::MULTIPLY:
            return left * right;
        case operater::DIVIDE:
            return left / right;
        case operater::ABS:
        case operater::AMPLITUDE:
            return abs(right);
        case operater::ANGLE:
            return arg(right);
        }
    }

    return variant();
}

variant operate(const string_t& left, const operater& oper, const string_t& right) {
    switch (oper.type) {
    case operater::COMPARE:
        switch (oper.compare) {
        case operater::LESS:
            return left < right;
        case operater::LESS_EQUAL:
            return left <= right;
        case operater::EQUAL:
            return left == right;
        case operater::APPROACH:
            return std::regex_search(left, std::basic_regex<char_t>(right));
        case operater::NOT_EQUAL:
            return left != right;
        case operater::GREATER_EQUAL:
            return left >= right;
        case operater::GREATER:
            return left > right;
        }
        break;
    case operater::ARITHMETIC:
        if (operater::PLUS == oper.arithmetic) {
            return left + right;
        }
        break;
    }

    return variant();
}

variant operate(const operater& oper, const list_t& right) {
    if (operater::STATISTIC == oper.type && !right.empty()) {
        size_t size = right.size();
        std::vector<real_t> values(size);
        std::transform(right.begin(), right.end(), values.begin(), [](const variant& var) { return var.to_real(); });
        switch (oper.statistic) {
        case operater::SUM:
        case operater::AVERAGE:
        case operater::VARIANCE:
        case operater::DEVIATION: {
            real_t sum = std::accumulate(values.begin(), values.end(), real_t());
            if (operater::SUM == oper.statistic) {
                return sum;
            }

            real_t average = sum / size;
            if (operater::AVERAGE == oper.statistic) {
                return average;
            }

            real_t variance = std::accumulate(values.begin(), values.end(), real_t(),
                                [average](real_t acc, real_t val) { return acc + pow(val - average, 2); }) / size;
            if (operater::VARIANCE == oper.statistic) {
                return variance;
            }

            return sqrt(variance);
        }
        case operater::MEDIAN: {
            std::sort(values.begin(), values.end());
            size_t index = size / 2;
            return (size % 2) ? values[index] : (values[index - 1] + values[index]) / 2;
        }
        case operater::MODE: {
            std::map<real_t, size_t> counters;
            for (real_t val : values) {
                ++counters[val];
            }

            using counter_t = decltype(counters)::const_reference;
            return std::max_element(counters.begin(), counters.end(),
                    [](counter_t c1, counter_t c2) { return c1.second < c2.second; })->first;
        }
        case operater::MAX:
            return *std::max_element(values.begin(), values.end());
        case operater::MIN:
            return *std::min_element(values.begin(), values.end());
        case operater::RANGE:
            return *std::max_element(values.begin(), values.end()) - *std::min_element(values.begin(), values.end());
        }
    }

    return variant();
}

}