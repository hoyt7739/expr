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

#ifndef EXPR_COMMON_H
#define EXPR_COMMON_H

#include <complex>
#include <string>
#include <vector>
#include <locale>
#include <codecvt>

namespace expr {

using real_t        = double;
using complex_t     = std::complex<real_t>;
using string_t      = std::wstring;
using char_t        = string_t::value_type;
using string_array  = std::vector<string_t>;

#define STR(s) L##s

const real_t REAL_PI    = 3.1415926535897932384626433832795;
const real_t REAL_E     = 2.7182818284590452353602874713527;
const real_t EPSILON    = 1.0e-9;

inline bool approach_to(real_t real1, real_t real2) {
    return fabs(real1 - real2) < EPSILON;
}

inline bool is_zahlen(real_t real) {
    return approach_to(real, round(real));
}

inline string_t to_string(bool boolean) {
    return boolean ? STR("true") : STR("false");
}

inline string_t to_string(real_t real) {
    string_t str = std::to_wstring(real);
    if (string_t::npos != str.find(STR('.'))) {
        while (STR('0') == str.back()) {
            str.pop_back();
        }
        if (STR('.') == str.back()) {
            str.pop_back();
        }
    }

    return str;
}

inline string_t to_string(const complex_t& complex) {
    real_t real = complex.real();
    real_t imag = complex.imag();
    if (approach_to(imag, 0)) {
        return to_string(real);
    }

    string_t imag_str;
    if (approach_to(imag, 1)) {
        imag_str = STR('i');
    } else if (approach_to(imag, -1)) {
        imag_str = STR("-i");
    } else {
        imag_str = to_string(imag) + STR('i');
    }

    return approach_to(real, 0) ? imag_str : to_string(real) + (imag < 0 ? imag_str : STR('+') + imag_str);
}

inline real_t to_real(const string_t& str) {
    real_t real = 0;
    try { real = std::stod(str); } catch (...) {}
    return real;
}

inline std::string to_utf8(const string_t& str) {
    return std::wstring_convert<std::codecvt_utf8_utf16<char_t>>().to_bytes(str);
}

inline string_t from_utf8(const std::string& str) {
    return std::wstring_convert<std::codecvt_utf8_utf16<char_t>>().from_bytes(str);
}

inline size_t replace(string_t& str, const string_t& before, const string_t& after, bool once = true) {
    if (str.empty() || before.empty()) {
        return 0;
    }

    size_t count = 0;
    for (size_t pos = 0; string_t::npos != (pos = str.find(before, pos)); pos += after.size()) {
        str.replace(pos, before.size(), after);
        ++count;

        if (once) {
            return count;
        }
    }

    return count;
}

inline string_t format(const string_t& str, const string_array& args, bool once = true) {
    if (str.empty() || args.empty()) {
        return str;
    }

    string_t res = str;
    for (size_t index = 0; index < args.size(); ++index) {
        replace(res, STR('%') + std::to_wstring(index + 1), args[index], once);
    }

    return res;
}

inline string_t format(const string_t& str, const string_t& arg, bool once = true) {
    return format(str, string_array(1, arg), once);
}

inline string_t format(const string_t& str, const string_t& arg1, const string_t& arg2, bool once = true) {
    return format(str, {arg1, arg2}, once);
}

inline string_t format(const string_t& str, const string_t& arg1, const string_t& arg2, const string_t& arg3, bool once = true) {
    return format(str, {arg1, arg2, arg3}, once);
}

inline string_t join(const string_array& args, const string_t& sep) {
    if (args.empty()) {
        return string_t();
    }

    string_t res = args[0];
    for (size_t index = 1; index < args.size(); ++index) {
        res += sep + args[index];
    }

    return res;
}

}

#endif
