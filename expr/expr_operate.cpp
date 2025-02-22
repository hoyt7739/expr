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

#include "expr_operate.h"
#include <unordered_set>
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
        size_t m = std::max(nth, MIN_ESTIMATE);
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

        size = std::max(size + size / 2, MIN_BITMAP_SIZE);
        s_bitmap.resize(size, true);
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
    static const size_t MIN_ESTIMATE;
    static const size_t MIN_BITMAP_SIZE;
    static std::vector<bool> s_bitmap;
};

const size_t prime_composite::MIN_ESTIMATE = 100;
const size_t prime_composite::MIN_BITMAP_SIZE = 10000;
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
    case operater::RELATION:
    case operater::ARITHMETIC:
        switch (right.type) {
        case variant::REAL:
            switch (left.type) {
            case variant::REAL:
                return operate(left.real, oper, right.real);
            case variant::COMPLEX:
                return operate(*left.complex, oper, right.to_complex());
            default:
                if (operater::UNARY == oper.kind && !oper.postpose) {
                    return operate(real_t(0), oper, right.real);
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
                if (operater::UNARY == oper.kind && !oper.postpose) {
                    return operate(complex_t(), oper, *right.complex);
                }
                break;
            }
            break;
        case variant::STRING:
            if (variant::STRING == left.type) {
                return operate(*left.string, oper, *right.string);
            }
            break;
        default:
            if (operater::UNARY == oper.kind && oper.postpose) {
                switch (left.type) {
                case variant::REAL:
                    return operate(left.real, oper, real_t(0));
                case variant::COMPLEX:
                    return operate(*left.complex, oper, complex_t());
                }
            }
            break;
        }
        break;
    case operater::EVALUATION:
        if (variant::SEQUENCE == right.type) {
            return operate(oper, *right.sequence);
        }
        break;
    }

    return variant();
}

variant operate(bool left, const operater& oper, bool right) {
    if (operater::LOGIC == oper.type) {
        switch (oper.code) {
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
    case operater::RELATION:
        switch (oper.code) {
        case operater::LESS:
            return left < right;
        case operater::LESS_EQUAL:
            return left <= right;
        case operater::EQUAL:
            return left == right;
        case operater::APPROACH:
            return approach_to(left, right);
        case operater::NOT_EQUAL:
            return left != right;
        case operater::GREATER_EQUAL:
            return left >= right;
        case operater::GREATER:
            return left > right;
        }
        break;
    case operater::ARITHMETIC: {
        auto extend_complex = [left, &oper, right] { return operate(complex_t(left), oper, complex_t(right)); };
        switch (oper.code) {
        case operater::PLUS:
            return left + right;
        case operater::MINUS:
            return left - right;
        case operater::MULTIPLY:
            return left * right;
        case operater::DIVIDE:
            return 0 != right ? left / right : (0 != left ? copysign(INFINITY, left) : variant());
        case operater::MODULUS:
            return 0 != right ? fmod(left, right) : variant();
        case operater::NEGATIVE:
            return -right;
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
        case operater::ABS:
            return fabs(right);
        case operater::PHASE:
            return 0 <= right ? real_t(0) : REAL_PI;
        case operater::REAL:
        case operater::CONJUGATE:
            return right;
        case operater::IMAGINARY:
            return real_t(0);
        case operater::FACTORIAL:
            return tgamma(left + 1);
        case operater::GAMMA:
            return tgamma(right);
        case operater::PERMUTE:
        case operater::COMBINE:
            if (0 <= left && 0 <= right) {
                if (left < right) {
                    std::swap(left, right);
                }
                real_t res = tgamma(left + 1) / tgamma(left - right + 1);
                return operater::PERMUTE == oper.code ? res : res / tgamma(right + 1);
            }
            return variant();
        case operater::POW:
            return 0 <= left ? pow(left, right) : extend_complex();
        case operater::EXP:
            return exp(right);
        case operater::LOG:
            if (0 <= left && 0 <= right) {
                left = log(left);
                right = log(right);
                return 0 != left ? right / left : (0 != right ? copysign(INFINITY, right) : variant());
            }
            return extend_complex();
        case operater::LG:
            return 0 <= right ? log10(right) : extend_complex();
        case operater::LN:
            return 0 <= right ? log(right) : extend_complex();
        case operater::SQRT:
            return 0 <= right ? sqrt(right) : extend_complex();
        case operater::ROOT:
            return 0 != left ? (0 <= right ? pow(right, 1 / left) : extend_complex()) : variant();
        case operater::HYPOT:
            return hypot(left, right);
        case operater::POLAR:
            return std::polar(left, right);
        case operater::DEG:
            return left * REAL_PI / 180;
        case operater::TODEG:
            return right * 180 / REAL_PI;
        case operater::TORAD:
            return right * REAL_PI / 180;
        case operater::SIN:
            return sin(right);
        case operater::ARCSIN:
            return -1 <= right && right <= 1 ? asin(right) : extend_complex();
        case operater::COS:
            return cos(right);
        case operater::ARCCOS:
            return -1 <= right && right <= 1 ? acos(right) : extend_complex();
        case operater::TAN:
            return !is_zahlen(right / REAL_PI - 0.5) ? tan(right) : INFINITY;
        case operater::ARCTAN:
            return atan(right);
        case operater::COT:
            return !is_zahlen(right / REAL_PI) ? cos(right) / sin(right) : INFINITY;
        case operater::ARCCOT:
            return 0 != right ? atan(1 / right) : REAL_PI / 2;
        case operater::SEC:
            return !is_zahlen(right / REAL_PI - 0.5) ? 1 / cos(right) : INFINITY;
        case operater::ARCSEC:
            return right <= -1 || 1 <= right ? acos(1 / right) : extend_complex();
        case operater::CSC:
            return !is_zahlen(right / REAL_PI) ? 1 / sin(right) : INFINITY;
        case operater::ARCCSC:
            return right <= -1 || 1 <= right ? asin(1 / right) : extend_complex();
        case operater::PRIME:
            return 2 <= right ? real_t(prime_composite::is_prime(static_cast<size_t>(right))) : real_t(0);
        case operater::COMPOSITE:
            return 2 <= right ? real_t(prime_composite::is_composite(static_cast<size_t>(right))) : real_t(0);
        case operater::NTH_PRIME:
            return 0 <= right ? prime_composite::nth_prime(static_cast<size_t>(right)) : variant();
        case operater::NTH_COMPOSITE:
            return 0 <= right ? prime_composite::nth_composite(static_cast<size_t>(right)) : variant();
        case operater::RAND:
            return 0 != right ? fmod(rand(), right) : real_t(rand());
        }
        break;
    }
    }

    return variant();
}

variant operate(const complex_t& left, const operater& oper, const complex_t& right) {
    switch (oper.type) {
    case operater::RELATION:
        switch (oper.code) {
        case operater::EQUAL:
            return left == right;
        case operater::APPROACH:
            return approach_to(left.real(), right.real()) && approach_to(left.imag(), right.imag());
        case operater::NOT_EQUAL:
            return left != right;
        }
        break;
    case operater::ARITHMETIC:
        switch (oper.code) {
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
            return abs(right);
        case operater::PHASE:
            return arg(right);
        case operater::REAL:
            return right.real();
        case operater::IMAGINARY:
            return right.imag();
        case operater::CONJUGATE:
            return conj(right);
        case operater::POW:
            return pow(left, right);
        case operater::EXP:
            return exp(right);
        case operater::LOG:
            return log(right) / log(left);
        case operater::LG:
            return log10(right);
        case operater::LN:
            return log(right);
        case operater::SQRT:
            return sqrt(right);
        case operater::ROOT:
            return pow(right, complex_t(1) / left);
        case operater::SIN:
            return sin(right);
        case operater::ARCSIN:
            return asin(right);
        case operater::COS:
            return cos(right);
        case operater::ARCCOS:
            return acos(right);
        case operater::TAN:
            return tan(right);
        case operater::ARCTAN:
            return atan(right);
        case operater::COT:
            return complex_t(1) / tan(right);
        case operater::ARCCOT:
            return atan(complex_t(1) / right);
        case operater::SEC:
            return complex_t(1) / cos(right);
        case operater::ARCSEC:
            return acos(complex_t(1) / right);
        case operater::CSC:
            return complex_t(1) / sin(right);
        case operater::ARCCSC:
            return asin(complex_t(1) / right);
        }
        break;
    }

    return variant();
}

variant operate(const string_t& left, const operater& oper, const string_t& right) {
    switch (oper.type) {
    case operater::RELATION:
        switch (oper.code) {
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
        if (operater::PLUS == oper.code) {
            return left + right;
        }
        break;
    }

    return variant();
}

variant operate(const operater& oper, const sequence_t& right) {
    if (operater::EVALUATION == oper.type) {
        const sequence_t& sequence = (1 == right.size() && right[0].is_sequence() ? *right[0].sequence : right);
        size_t size = sequence.size();
        switch (oper.code) {
        case operater::COUNT:
            return size;
        case operater::UNIQUE: {
            sequence_t res;
            res.reserve(size);
            std::unordered_set<variant> seen;
            for (const variant& item : sequence) {
                if (seen.insert(item).second) {
                    res.push_back(item);
                }
            }
            return res;
        }
        case operater::DFT:
        case operater::IDFT: {
            sequence_t res;
            res.reserve(size);
            real_t a = (operater::IDFT == oper.code ? 2 : -2) * REAL_PI / size;
            for (size_t m = 0; m < size; ++m) {
                complex_t sum;
                for (size_t n = 0; n < size; ++n) {
                    real_t ang = a * m * n;
                    sum += sequence[n].to_complex() * complex_t(cos(ang), sin(ang));
                }
                if (operater::IDFT == oper.code) {
                    sum /= real_t(size);
                }
                res.emplace_back(std::move(sum));
            }
            return res;
        }
        case operater::FFT:
        case operater::IFFT: {
            size = static_cast<size_t>(pow(2, ceil(log2(size))));
            sequence_t res(size);
            std::transform(sequence.begin(), sequence.end(), res.begin(), [](const variant& var) { return var.to_complex(); });
            std::fill(res.begin() + sequence.size(), res.end(), complex_t());
            for (size_t m = 1, n = 0; m < size; ++m) {
                for (size_t p = size >> 1; (n ^= p) < p; p >>= 1);
                if (m < n) {
                    std::swap(res[m], res[n]);
                }
            }

            real_t a = (operater::FFT == oper.code ? -2 : 2) * REAL_PI;
            for (size_t len = 2; len <= size; len <<= 1) {
                size_t mid = len >> 1;
                real_t ang = a / len;
                complex_t unit(cos(ang), sin(ang));
                for (size_t m = 0; m < size; m += len) {
                    complex_t w(1);
                    for (size_t n = m; n < m + mid; ++n) {
                        complex_t p = *res[n].complex;
                        complex_t q = *res[n + mid].complex * w;
                        *res[n].complex = p + q;
                        *res[n + mid].complex = p - q;
                        w *= unit;
                    }
                }
            }

            if (operater::IFFT == oper.code) {
                for (variant& var : res) {
                    *var.complex /= real_t(size);
                }
            }

            return res;
        }
        case operater::ZT: {
            if (size < 2 || !sequence[0].is_sequence()) {
                return variant();
            }

            const sequence_t& samples = *sequence[0].sequence;
            if (samples.empty()) {
                return variant();
            }

            auto begin = sequence.begin() + 1;
            auto end = sequence.end();
            if (begin->is_sequence()) {
                end = begin->sequence->end();
                begin = begin->sequence->begin();
            }

            sequence_t res;
            res.reserve(end - begin);
            for (; end != begin; ++begin) {
                complex_t sum;
                for (size_t n = 0; n < samples.size(); ++n) {
                    sum += samples[n].to_complex() * pow(begin->to_complex(), -real_t(n));
                }
                res.emplace_back(std::move(sum));
            }

            return res;
        }
        }

        if (0 == size) {
            return variant();
        }

        std::vector<real_t> values(size);
        std::transform(sequence.begin(), sequence.end(), values.begin(), [](const variant& var) { return var.to_real(); });
        switch (oper.code) {
        case operater::MIN:
            return *std::min_element(values.begin(), values.end());
        case operater::MAX:
            return *std::max_element(values.begin(), values.end());
        case operater::RANGE:
        case operater::NORM: {
            auto pair = std::minmax_element(values.begin(), values.end());
            real_t range = *pair.second - *pair.first;
            if (operater::RANGE == oper.code) {
                return range;
            }

            if (0 == range) {
                return sequence_t(size, 0.5);
            }

            sequence_t res(size);
            real_t min = *pair.first;
            std::transform(values.begin(), values.end(), res.begin(), [min, range](real_t value) { return (value - min) / range; });
            return res;
        }
        case operater::TOTAL:
        case operater::MEAN:
        case operater::VARIANCE:
        case operater::DEVIATION:
        case operater::ZSCORE_NORM: {
            real_t total = std::accumulate(values.begin(), values.end(), real_t(0));
            if (operater::TOTAL == oper.code) {
                return total;
            }

            real_t mean = total / size;
            if (operater::MEAN == oper.code) {
                return mean;
            }

            real_t variance = std::accumulate(values.begin(), values.end(), real_t(0), [mean](real_t acc, real_t value) {
                real_t diff = value - mean;
                return acc + diff * diff;
            }) / size;
            if (operater::VARIANCE == oper.code) {
                return variance;
            }

            real_t stddev = sqrt(variance);
            if (operater::DEVIATION == oper.code) {
                return stddev;
            }

            if (0 == stddev) {
                return sequence_t(size, 0);
            }

            sequence_t res(size);
            std::transform(values.begin(), values.end(), res.begin(), [mean, stddev](real_t value) { return (value - mean) / stddev; });
            return res;
        }
        case operater::GEOMETRIC_MEAN: {
            real_t total = std::accumulate(values.begin(), values.end(), real_t(1), std::multiplies<real_t>());
            return pow(total, real_t(1) / size);
        }
        case operater::QUADRATIC_MEAN:
        case operater::HYPOT: {
            real_t total = std::accumulate(values.begin(), values.end(), real_t(0), [](real_t acc, real_t value) {
                return acc + value * value;
            });
            return sqrt(operater::QUADRATIC_MEAN == oper.code ? total / size : total);
        }
        case operater::HARMONIC_MEAN: {
            real_t total = std::accumulate(values.begin(), values.end(), real_t(0), [](real_t acc, real_t value) {
                return acc + 1 / value;
            });
            return size / total;
        }
        case operater::MEDIAN: {
            std::sort(values.begin(), values.end());
            size_t index = size / 2;
            return (size % 2) ? values[index] : (values[index - 1] + values[index]) / 2;
        }
        case operater::MODE: {
            std::map<real_t, size_t> counters;
            for (real_t value : values) {
                ++counters[value];
            }

            using counter_t = decltype(counters)::const_reference;
            return std::max_element(counters.begin(), counters.end(), [](counter_t c1, counter_t c2) {
                return c1.second < c2.second;
            })->first;
        }
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
                return m && n ? (m / gcd(m, n)) * n : 0;
            };

            size_t res = static_cast<size_t>(fabs(values[0]));
            for (size_t index = 1; index < size; ++index) {
                size_t value = static_cast<size_t>(fabs(values[index]));
                if (operater::GCD == oper.code) {
                    res = gcd(res, value);
                    if (1 == res) {
                        break;
                    }
                } else {
                    res = lcm(res, value);
                }
            }

            return res;
        }
        }
    }

    return variant();
}

}
