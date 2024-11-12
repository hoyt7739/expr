#include <map>
#include <algorithm>
#include <numeric>
#include <regex>
#include "expr_defs.h"

namespace expr {

template<class left_t, class right_t>
inline variant operate(const left_t& left, const operater& oper, const right_t& right) {
    switch (oper.type) {
        case operater::ARITHMETIC: {
            switch (oper.arithmetic) {
                case operater::PLUS: {
                    return left + right;
                }
                case operater::MINUS: {
                    return left - right;
                }
                case operater::MULTIPLY: {
                    return left * right;
                }
                case operater::DIVIDE: {
                    return right ? left / right : variant();
                }
                case operater::MOD: {
                    return right ? static_cast<integer_t>(left) % static_cast<integer_t>(right) : variant();
                }
                case operater::NEGATIVE: {
                    return 0 - right;
                }
                case operater::ABS: {
                    return right < 0 ? 0 - right : right;
                }
                case operater::CEIL: {
                    return ceil(right);
                }
                case operater::FLOOR: {
                    return floor(right);
                }
                case operater::ROUND: {
                    return round(right);
                }
                case operater::POW: {
                    return pow(left, right);
                }
                case operater::EXP: {
                    return exp(right);
                }
                case operater::LOG: {
                    if (left <= 0 || left == 1 || right <= 0) {
                        return variant();
                    }
                    if (2 == left) {
                        return log2(right);
                    }
                    if (10 == left) {
                        return log10(right);
                    }
                    if (NATURAL == left) {
                        return log(right);
                    }
                    return log(right) / log(left);
                }
                case operater::LG: {
                    return right <= 0 ? variant() : log10(right);
                }
                case operater::LN: {
                    return right <= 0 ? variant() : log(right);
                }
                case operater::SQRT: {
                    return right < 0 ? variant() : sqrt(right);
                }
                case operater::ROOT: {
                    return left <= 0 ? variant() : pow(right, 1.0 / left);
                }
                case operater::DEG: {
                    return right * 180.0 / PI;
                }
                case operater::RAD: {
                    return right * PI / 180.0;
                }
                case operater::SIN: {
                    return sin(right);
                }
                case operater::COS: {
                    return cos(right);
                }
                case operater::TAN: {
                    return tan(right);
                }
                case operater::COT: {
                    return 1.0 / tan(right);
                }
                case operater::VECTOR: {
                    return std::polar(static_cast<real_t>(left), static_cast<real_t>(right));
                }
                case operater::EXPAND: {
                    return list_t({left - right, left + right});
                }
                case operater::EXPAND_PERCENT: {
                    return list_t({left - left * (right / 100.0), left + left * (right / 100.0)});
                }
            }
            break;
        }
        case operater::COMPARE: {
            switch (oper.compare) {
                case operater::LESS: {
                    return left < right;
                }
                case operater::LESS_EQUAL: {
                    return left <= right;
                }
                case operater::EQUAL: {
                    return left == right;
                }
                case operater::NOT_EQUAL: {
                    return left != right;
                }
                case operater::GREATER_EQUAL: {
                    return left >= right;
                }
                case operater::GREATER: {
                    return left > right;
                }
            }
            break;
        }
    }

    return variant();
}

template<class left_t>
inline variant operate(const left_t& left, const operater& oper, const list_t& right) {
    std::vector<real_t> values(right.size());
    std::transform(right.begin(), right.end(), values.begin(), [](const variant& var) { return var.to_real(); });
    switch (oper.type) {
        case operater::ARITHMETIC: {
            return std::accumulate(values.begin(), values.end(), 0.0, [&left, &oper](real_t acc, real_t val) {
                return acc + operate(left, oper, val).to_real();
            });
        }
        case operater::COMPARE: {
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
        }
    }

    return variant();
}

template<class right_t>
inline variant operate(const list_t& left, const operater& oper, const right_t& right) {
    std::vector<real_t> values(left.size());
    std::transform(left.begin(), left.end(), values.begin(), [](const variant& var) { return var.to_real(); });
    switch (oper.type) {
        case operater::ARITHMETIC: {
            return std::accumulate(values.begin(), values.end(), 0.0, [&oper, &right](real_t acc, real_t val) {
                return acc + operate(val, oper, right).to_real();
            });
        }
        case operater::COMPARE: {
            if (operater::APPROACH == oper.compare) {
                return operate(right, oper, left);
            }

            return std::accumulate(values.begin(), values.end(), true, [&oper, &right](bool acc, real_t val) {
                return acc && operate(val, oper, right).to_boolean();
            });
        }
    }

    return variant();
}

inline variant operate(bool left, const operater& oper, bool right) {
    if (operater::LOGIC == oper.type) {
        switch (oper.logic) {
            case operater::AND: {
                return left && right;
            }
            case operater::OR: {
                return left || right;
            }
            case operater::NOT: {
                return !right;
            }
        }
    }

    return variant();
}

inline variant operate(const complex_t& left, const operater& oper, const complex_t& right) {
    if (operater::ARITHMETIC == oper.type) {
        switch (oper.arithmetic) {
            case operater::PLUS: {
                return left + right;
            }
            case operater::MINUS: {
                return left - right;
            }
            case operater::MULTIPLY: {
                return left * right;
            }
            case operater::DIVIDE: {
                return left / right;
            }
            case operater::ABS:
            case operater::AMP: {
                return abs(right);
            }
            case operater::ARG: {
                return arg(right);
            }
        }
    }

    return variant();
}

inline variant operate(const string_t& left, const operater& oper, const string_t& right) {
    switch (oper.type) {
        case operater::ARITHMETIC: {
            if (operater::PLUS == oper.arithmetic) {
                return left + right;
            }
            break;
        }
        case operater::COMPARE: {
            switch (oper.compare) {
                case operater::EQUAL: {
                    return left == right;
                }
                case operater::APPROACH: {
                    return (left.empty() && right.empty()) ||
                           (!left.empty() && !right.empty() &&
                            (string_t::npos != left.find(right) || string_t::npos != right.find(left)));
                }
                case operater::REGULAR_MATCH: {
                    return std::regex_search(left, std::basic_regex<char_t>(right));
                }
                case operater::NOT_EQUAL: {
                    return left != right;
                }
            }
            break;
        }
    }

    return variant();
}

inline variant operate(const operater& oper, const list_t& right) {
    if (operater::EVALUATION == oper.type && !right.empty()) {
        std::vector<real_t> values(right.size());
        std::transform(right.begin(), right.end(), values.begin(), [](const variant& var) { return var.to_real(); });
        switch (oper.evaluation) {
            case operater::SUM:
            case operater::AVERAGE:
            case operater::VARIANCE:
            case operater::DEVIATION: {
                real_t sum = std::accumulate(values.begin(), values.end(), 0.0);
                if (operater::SUM == oper.evaluation) {
                    return sum;
                }
                size_t size = values.size();
                real_t average = sum / size;
                if (operater::AVERAGE == oper.evaluation) {
                    return average;
                }
                real_t variance = std::accumulate(values.begin(), values.end(), 0.0, [average](real_t acc, real_t val) {
                    return acc + pow(val - average, 2);
                }) / size;
                if (operater::VARIANCE == oper.evaluation) {
                    return variance;
                }
                return sqrt(variance);
            }
            case operater::MEDIAN: {
                std::sort(values.begin(), values.end());
                size_t size = values.size();
                size_t index = size / 2;
                return (size % 2) ? values[index] : (values[index - 1] + values[index]) / 2;
            }
            case operater::MODE: {
                std::map<real_t, size_t> counters;
                for (real_t val : values) {
                    ++counters[val];
                }
                using counter_t = decltype(counters)::const_reference;
                return std::max_element(counters.begin(), counters.end(), [](counter_t c1, counter_t c2) {
                    return c1.second < c2.second;
                })->first;
            }
            case operater::MAX: {
                return *std::max_element(values.begin(), values.end());
            }
            case operater::MIN: {
                return *std::min_element(values.begin(), values.end());
            }
            case operater::RANGE: {
                return *std::max_element(values.begin(), values.end()) -
                       *std::min_element(values.begin(), values.end());
            }
        }
    }

    return variant();
}

inline variant operate(const variant& left, const operater& oper, const variant& right) {
    switch (oper.type) {
        case operater::ARITHMETIC:
        case operater::COMPARE: {
            switch (right.type) {
                case variant::INTEGER: {
                    switch (left.type) {
                        case variant::INTEGER: {
                            return operate(left.integer, oper, right.integer);
                        }
                        case variant::REAL: {
                            return operate(left.real, oper, right.integer);
                        }
                        case variant::COMPLEX: {
                            return operate(*left.complex, oper, right.to_complex());
                        }
                        case variant::LIST: {
                            return operate(*left.list, oper, right.integer);
                        }
                        default: {
                            if (operater::UNARY == oper.mode) {
                                return operate(0ll, oper, right.integer);
                            }
                            break;
                        }
                    }
                    break;
                }
                case variant::REAL: {
                    switch (left.type) {
                        case variant::INTEGER: {
                            return operate(left.integer, oper, right.real);
                        }
                        case variant::REAL: {
                            return operate(left.real, oper, right.real);
                        }
                        case variant::COMPLEX: {
                            return operate(*left.complex, oper, right.to_complex());
                        }
                        case variant::LIST: {
                            return operate(*left.list, oper, right.real);
                        }
                        default: {
                            if (operater::UNARY == oper.mode) {
                                return operate(0.0, oper, right.real);
                            }
                            break;
                        }
                    }
                    break;
                }
                case variant::COMPLEX: {
                    switch (left.type) {
                        case variant::INTEGER:
                        case variant::REAL: {
                            return operate(left.to_complex(), oper, *right.complex);
                        }
                        case variant::COMPLEX: {
                            return operate(*left.complex, oper, *right.complex);
                        }
                        default: {
                            if (operater::UNARY == oper.mode) {
                                return operate(complex_t(), oper, *right.complex);
                            }
                            break;
                        }
                    }
                    break;
                }
                case variant::STRING: {
                    if (variant::STRING == left.type) {
                        return operate(*left.string, oper, *right.string);
                    }
                    break;
                }
                case variant::LIST: {
                    switch (left.type) {
                        case variant::INTEGER: {
                            return operate(left.integer, oper, *right.list);
                        }
                        case variant::REAL: {
                            return operate(left.real, oper, *right.list);
                        }
                    }
                    break;
                }
            }
            break;
        }
        case operater::LOGIC: {
            return operate(left.to_boolean(), oper, right.to_boolean());
        }
        case operater::EVALUATION: {
            return operate(oper, *right.list);
        }
    }

    return variant();
}

}