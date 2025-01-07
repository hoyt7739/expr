#include <map>
#include <algorithm>
#include <numeric>
#include <regex>
#include "expr_defs.h"

namespace expr {

inline bool is_zahlen(real_t value) {
    const real_t epsilon = 1e-10;
    return fabs(value - round(value)) < epsilon;
}

inline real_t factorial(real_t value) {
    real_t res = 1;
    for (real_t i = 1; i <= value; ++i) {
        res *= i;
    }

    return res;
}

inline variant operate(bool left, const operater& oper, bool right) {
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

inline variant operate(real_t left, const operater& oper, real_t right) {
    switch (oper.type) {
    case operater::COMPARE:
        switch (oper.compare) {
        case operater::LESS:
            return left < right;
        case operater::LESS_EQUAL:
            return left <= right;
        case operater::EQUAL:
            return left == right;
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
        case operater::HYPOT:
            return hypot(left, right);
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
        case operater::EXPAND:
            return list_t({left - right, left + right});
        case operater::EXPAND_PERCENT:
            return list_t({left - left * (right / 100), left + left * (right / 100)});
        }
        break;
    }

    return variant();
}

inline variant operate(real_t left, const operater& oper, const list_t& right) {
    std::vector<real_t> values(right.size());
    std::transform(right.begin(), right.end(), values.begin(), [](const variant& var) { return var.to_real(); });
    switch (oper.type) {
    case operater::COMPARE:
        if (operater::APPROACH == oper.compare) {
            auto min_iter = std::min_element(values.begin(), values.end());
            if (values.end() == min_iter) {
                return variant();
            }

            auto max_iter = std::max_element(values.begin(), values.end());
            if (values.end() == max_iter) {
                return variant();
            }

            return *min_iter <= left && left <= *max_iter;
        }
        return std::accumulate(values.begin(), values.end(), true, [&left, &oper](bool acc, real_t val) {
            return acc && operate(left, oper, val).to_boolean();
        });
    case operater::ARITHMETIC:
        return std::accumulate(values.begin(), values.end(), real_t(), [&left, &oper](real_t acc, real_t val) {
            return acc + operate(left, oper, val).to_real();
        });
    }

    return variant();
}

inline variant operate(const list_t& left, const operater& oper, real_t right) {
    std::vector<real_t> values(left.size());
    std::transform(left.begin(), left.end(), values.begin(), [](const variant& var) { return var.to_real(); });
    switch (oper.type) {
    case operater::COMPARE:
        if (operater::APPROACH == oper.compare) {
            return operate(right, oper, left);
        }
        return std::accumulate(values.begin(), values.end(), true, [&oper, &right](bool acc, real_t val) {
            return acc && operate(val, oper, right).to_boolean();
        });
    case operater::ARITHMETIC:
        return std::accumulate(values.begin(), values.end(), real_t(), [&oper, &right](real_t acc, real_t val) {
            return acc + operate(val, oper, right).to_real();
        });
    }

    return variant();
}

inline variant operate(const complex_t& left, const operater& oper, const complex_t& right) {
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

inline variant operate(const string_t& left, const operater& oper, const string_t& right) {
    switch (oper.type) {
    case operater::COMPARE:
        switch (oper.compare) {
        case operater::EQUAL:
            return left == right;
        case operater::APPROACH:
            return (left.empty() && right.empty()) ||
                   (!left.empty() && !right.empty() &&
                    (string_t::npos != left.find(right) || string_t::npos != right.find(left)));
        case operater::REGULAR_MATCH:
            return std::regex_search(left, std::basic_regex<char_t>(right));
        case operater::NOT_EQUAL:
            return left != right;
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

inline variant operate(const operater& oper, const list_t& right) {
    if (operater::EVALUATION == oper.type && !right.empty()) {
        size_t size = right.size();
        std::vector<real_t> values(size);
        std::transform(right.begin(), right.end(), values.begin(), [](const variant& var) { return var.to_real(); });
        switch (oper.evaluation) {
        case operater::SUM:
        case operater::AVERAGE:
        case operater::VARIANCE:
        case operater::DEVIATION: {
            real_t sum = std::accumulate(values.begin(), values.end(), real_t());
            if (operater::SUM == oper.evaluation) {
                return sum;
            }

            real_t average = sum / size;
            if (operater::AVERAGE == oper.evaluation) {
                return average;
            }

            real_t variance = std::accumulate(values.begin(), values.end(), real_t(),
                                [average](real_t acc, real_t val) { return acc + pow(val - average, 2); }) / size;
            if (operater::VARIANCE == oper.evaluation) {
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
        case operater::ARRANGEMENT:
        case operater::COMBINATION: {
            if (size < 2) {
                return variant();
            }

            real_t n = floor(values[0]);
            real_t m = floor(values[1]);
            if (n < 0 || m < 0 || n < m) {
                return variant();
            }

            real_t res = factorial(n) / factorial(n - m);
            if (operater::ARRANGEMENT == oper.evaluation) {
                return res;
            }

            return res / factorial(m);
        }
        case operater::LERP:
            if (size < 3) {
                return variant();
            }
            return (1 - values[2]) * values[0] + values[2] * values[1];
        }
    }

    return variant();
}

inline variant operate(const variant& left, const operater& oper, const variant& right) {
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
            case variant::LIST:
                return operate(*left.list, oper, right.real);
            default:
                return operate(real_t(), oper, right.real);
            }
            break;
        case variant::COMPLEX:
            switch (left.type) {
            case variant::REAL:
                return operate(left.to_complex(), oper, *right.complex);
            case variant::COMPLEX:
                return operate(*left.complex, oper, *right.complex);
            default:
                return operate(complex_t(), oper, *right.complex);
            }
            break;
        case variant::STRING:
            if (variant::STRING == left.type) {
                return operate(*left.string, oper, *right.string);
            }
            break;
        case variant::LIST:
            switch (left.type) {
            case variant::REAL:
                return operate(left.real, oper, *right.list);
            }
            break;
        default:
            switch (left.type) {
            case variant::REAL:
                return operate(left.real, oper, real_t());
            case variant::COMPLEX:
                return operate(*left.complex, oper, complex_t());
            }
            break;
        }
        break;
    case operater::EVALUATION:
        return operate(oper, *right.list);
    }

    return variant();
}
}