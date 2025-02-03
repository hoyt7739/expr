#include "expr_operate.h"
#include <map>
#include <unordered_set>
#include <algorithm>
#include <numeric>
#include <regex>

namespace expr {

class prime_composite {
public:
    enum number_type {
        PRIME,
        COMPOSITE
    };

public:
    static bool test_number(size_t num, number_type type) {
        if (1 < num) {
            generate_bitmap(num + 1);
            return PRIME == type ? s_bitmap[num] : !s_bitmap[num];
        }

        return false;
    }

    static bool is_prime(size_t num) {
        return test_number(num, PRIME);
    }

    static bool is_composite(size_t num) {
        return test_number(num, COMPOSITE);
    }

    static size_t nth_number(size_t nth, number_type type) {
        size_t m = std::max(nth, size_t(100));
        generate_bitmap(PRIME == type ? static_cast<size_t>(m * (log(m) + log(log(m)))) : m * 2);
        for (size_t count = 0, num = 2; num < s_bitmap.size(); ++num) {
            if (PRIME == type ? s_bitmap[num] : !s_bitmap[num]) {
                if (count == nth) {
                    return num;
                }
                ++count;
            }
        }

        return 0;
    }

    static size_t nth_prime(size_t nth) {
        return nth_number(nth, PRIME);
    }

    static size_t nth_composite(size_t nth) {
        return nth_number(nth, COMPOSITE);
    }

private:
    static void generate_bitmap(size_t size) {
        if (size <= s_bitmap.size()) {
            return;
        }

        size = std::max(size + size / 2, size_t(10000));
        s_bitmap = std::vector<bool>(size, true);
        s_bitmap[0] = s_bitmap[1] = false;

        size_t upper = static_cast<size_t>(sqrt(size));
        for (size_t m = 2; m <= upper; ++m) {
            if (s_bitmap[m]) {
                for (size_t n = m * m; n < size; n += m) {
                    s_bitmap[n] = false;
                }
            }
        }
    }

private:
    static std::vector<bool> s_bitmap;
};

std::vector<bool> prime_composite::s_bitmap;

variant operate(const variant& left, const operater& oper, const variant& right) {
    switch (oper.type) {
    case operater::LOGIC:
        switch (right.type) {
        case variant::BOOLEAN:
        case variant::REAL:
            switch (left.type) {
            case variant::BOOLEAN:
            case variant::REAL:
                return operate(left.to_boolean(), oper, right.to_boolean());
            }
            break;
        }
        break;
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
            return right ? left / right : variant();
        case operater::MOD:
            return right ? fmod(left, right) : variant();
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
            if (0 <= left) {
                real_t res = 1;
                for (real_t n = 1; n <= left; ++n) {
                    res *= n;
                }
                return res;
            }
            return variant();
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
            return 0 < right ? log10(right) : variant();
        case operater::LN:
            return 0 < right ? log(right) : variant();
        case operater::SQRT:
            return 0 <= right ? sqrt(right) : variant();
        case operater::ROOT:
            return 0 < left ? pow(right, 1 / left) : variant();
        case operater::DEG:
            return left * CONST_PI / 180;
        case operater::TODEG:
            return right * 180 / CONST_PI;
        case operater::TORAD:
            return right * CONST_PI / 180;
        case operater::SIN:
            return sin(right);
        case operater::ARCSIN:
            return -1 <= right && right <= 1 ? asin(right) : variant();
        case operater::COS:
            return cos(right);
        case operater::ARCCOS:
            return -1 <= right && right <= 1 ? acos(right) : variant();
        case operater::TAN:
            return !is_zahlen(right / CONST_PI - 0.5) ? tan(right) : variant();
        case operater::ARCTAN:
            return atan(right);
        case operater::COT:
            return !is_zahlen(right / CONST_PI) ? cos(right) / sin(right) : variant();
        case operater::ARCCOT:
            return CONST_PI / 2 - atan(right);
        case operater::SEC:
            return !is_zahlen(right / CONST_PI - 0.5) ? 1 / cos(right) : variant();
        case operater::ARCSEC:
            return right <= -1 || 1 <= right ? acos(1 / right) : variant();
        case operater::CSC:
            return !is_zahlen(right / CONST_PI) ? 1 / sin(right) : variant();
        case operater::ARCCSC:
            return right <= -1 || 1 <= right ? asin(1 / right) : variant();
        case operater::VECTOR:
            return std::polar(left, right);
        case operater::PRIME:
            return 2 <= right ? static_cast<real_t>(prime_composite::is_prime(static_cast<size_t>(right))) : real_t();
        case operater::COMPOSITE:
            return 2 <= right ? static_cast<real_t>(prime_composite::is_composite(static_cast<size_t>(right))) : real_t();
        case operater::NTH_PRIME:
            return 0 <= right ? static_cast<real_t>(prime_composite::nth_prime(static_cast<size_t>(right))) : variant();
        case operater::NTH_COMPOSITE:
            return 0 <= right ? static_cast<real_t>(prime_composite::nth_composite(static_cast<size_t>(right))) : variant();
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
        case operater::NEGATIVE:
            return -right;
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
    if (operater::STATISTIC == oper.type) {
        const list_t& sequence = (1 == right.size() && right[0].is_list() ? *right[0].list : right);
        size_t size = sequence.size();
        switch (oper.statistic) {
        case operater::COUNT:
            return static_cast<real_t>(size);
        case operater::UNIQUE: {
            list_t res;
            std::unordered_set<variant> seen;
            for (const variant& item : sequence) {
                if (seen.end() == seen.find(item)) {
                    res.push_back(item);
                    seen.insert(item);
                }
            }
            return res;
        }
        }

        if (!size) {
            return variant();
        }

        std::vector<real_t> values(size);
        std::transform(sequence.begin(), sequence.end(), values.begin(), [](const variant& var) { return var.to_real(); });
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

            real_t variance = std::accumulate(values.begin(), values.end(), real_t(), [average](real_t acc, real_t val) {
                return acc + pow(val - average, 2);
            }) / size;
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
            return std::max_element(counters.begin(), counters.end(), [](counter_t c1, counter_t c2) {
                return c1.second < c2.second;
            })->first;
        }
        case operater::MAX:
            return *std::max_element(values.begin(), values.end());
        case operater::MIN:
            return *std::min_element(values.begin(), values.end());
        case operater::RANGE:
            return *std::max_element(values.begin(), values.end()) - *std::min_element(values.begin(), values.end());
        case operater::GCD:
        case operater::LCM: {
            auto gcd = [](size_t m, size_t n) {
                while (n) {
                    size_t temp = n;
                    n = m % n;
                    m = temp;
                }
                return m;
            };

            auto lcm = [&gcd](size_t m, size_t n) {
                return m && n ? (m / gcd(m, n)) * n : size_t();
            };

            size_t res = static_cast<size_t>(fabs(values[0]));
            for (size_t index = 1; index < values.size(); ++index) {
                size_t value = static_cast<size_t>(fabs(values[index]));
                if (operater::GCD == oper.statistic) {
                    res = gcd(res, value);
                    if (1 == res) {
                        break;
                    }
                } else {
                    res = lcm(res, value);
                }
            }

            return static_cast<real_t>(res);
        }
        }
    }

    return variant();
}

}
