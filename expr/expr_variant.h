#ifndef EXPR_VARIANT_H
#define EXPR_VARIANT_H

#include <cstring>
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
using list_t        = std::vector<struct variant>;

#define STR(s) L##s

const real_t CONST_PI   = 3.1415926535897932384626433832795;
const real_t CONST_E    = 2.7182818284590452353602874713527;
const real_t EPSILON    = 1.0e-9;

inline bool is_zahlen(real_t value) {
    return fabs(value - round(value)) < EPSILON;
}

inline real_t to_real(const string_t& str) {
    real_t real{};
    try { real = std::stod(str); } catch (...) {}
    return real;
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

inline std::string to_utf8(const string_t& str) {
    return std::wstring_convert<std::codecvt_utf8_utf16<char_t>>().to_bytes(str);
}

inline string_t from_utf8(const std::string& str) {
    return std::wstring_convert<std::codecvt_utf8_utf16<char_t>>().from_bytes(str);
}

struct variant {
    enum variant_type {
        INVALID,
        BOOLEAN,
        REAL,
        COMPLEX,
        STRING,
        LIST
    };

    variant_type    type;
    union {
        bool        boolean;
        real_t      real;
        complex_t*  complex;
        string_t*   string;
        list_t*     list;
    };

    variant() : type(INVALID) {}
    variant(bool value) : type(BOOLEAN), boolean(value) {}
    variant(real_t value) : type(REAL), real(value) {}
    variant(const complex_t& value) : type(COMPLEX), complex(new complex_t(value)) {}
    variant(const string_t& value) : type(STRING), string(new string_t(value)) {}
    variant(const list_t& value) : type(LIST), list(new list_t(value)) {}
    variant(const variant& other) { *this = other; }
    variant(variant&& other) noexcept { *this = other; }

    ~variant() {
        switch (type) {
        case COMPLEX:
            delete complex;
            break;
        case STRING:
            delete string;
            break;
        case LIST:
            delete list;
            break;
        }
    }

    variant& operator=(const variant& other) {
        if (this == &other) {
            return *this;
        }

        memcpy(this, &other, sizeof(variant));
        switch (type) {
        case COMPLEX:
            complex = new complex_t(*other.complex);
            break;
        case STRING:
            string = new string_t(*other.string);
            break;
        case LIST:
            list = new list_t(*other.list);
            break;
        }

        return *this;
    }

    variant& operator=(variant&& other) noexcept {
        if (this == &other) {
            return *this;
        }

        memcpy(this, &other, sizeof(variant));
        other.type = INVALID;

        return *this;
    }

    bool to_boolean() const {
        switch (type) {
        case BOOLEAN:
            return boolean;
        case REAL:
            return 0 != real;
        case COMPLEX:
            return 0 != complex->real() && 0 != complex->imag();
        case STRING:
            return !string->empty();
        }

        return false;
    }

    real_t to_real() const {
        switch (type) {
        case BOOLEAN:
            return boolean;
        case REAL:
            return real;
        case COMPLEX:
            return complex->real();
        case STRING:
            return expr::to_real(*string);
        }

        return real_t();
    }

    complex_t to_complex() const {
        if (COMPLEX == type) {
            return *complex;
        }

        return complex_t(to_real());
    }

    string_t to_string() const {
        switch (type) {
        case BOOLEAN:
            return boolean ? STR("true") : STR("false");
        case REAL:
            return expr::to_string(real);
        case COMPLEX:
            return expr::to_string(complex->real()) + STR('+') + expr::to_string(complex->imag()) + STR('i');
        case STRING:
            return *string;
        case LIST: {
            string_t str;
            for (variant& var : *list) {
                if (!str.empty()) {
                    str += STR(',');
                }
                str += var.to_string();
            }
            return STR('(') + str + STR(')');
        }
        }

        return string_t();
    }
};

}

#endif
