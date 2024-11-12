#ifndef EXPR_VARIANT_H
#define EXPR_VARIANT_H

#include <cstring>
#include <complex>
#include <string>
#include <vector>

namespace expr {

using integer_t     = long long;
using real_t        = double;
using complex_t     = std::complex<real_t>;
using string_t      = std::wstring;
using char_t        = string_t::value_type;
using list_t        = std::vector<struct variant>;

#define STR(s) L##s

inline string_t to_string(integer_t val) {
    return std::to_wstring(val);
}

inline string_t to_string(real_t val) {
    string_t str = std::to_wstring(val);
    while (STR('0') == str.back()) {
        str.pop_back();
    }
    if (STR('.') == str.back()) {
        str.pop_back();
    }
    return str;
}

struct variant {
    enum variant_type {
        INVALID,
        BOOLEAN,
        INTEGER,
        REAL,
        COMPLEX,
        STRING,
        LIST
    };

    variant_type    type;
    union {
        bool        boolean;
        integer_t   integer;
        real_t      real;
        complex_t*  complex;
        string_t*   string;
        list_t*     list;
    };

    variant() : type(INVALID) {}
    variant(bool value) : type(BOOLEAN), boolean(value) {}
    variant(integer_t value) : type(INTEGER), integer(value) {}
    variant(real_t value) : type(REAL), real(value) {}
    variant(const complex_t& value) : type(COMPLEX), complex(new complex_t(value)) {}
    variant(const string_t& value) : type(STRING), string(new string_t(value)) {}
    variant(const list_t& value) : type(LIST), list(new list_t(value)) {}
    variant(const variant& other) { *this = other; }
    variant(variant&& other) noexcept { *this = other; }

    ~variant() {
        switch (type) {
            case COMPLEX: {
                delete complex;
                break;
            }
            case STRING: {
                delete string;
                break;
            }
            case LIST: {
                delete list;
                break;
            }
        }
    }

    variant& operator=(const variant& other) {
        if (this == &other) {
            return *this;
        }

        memcpy(this, &other, sizeof(variant));
        switch (type) {
            case COMPLEX: {
                complex = new complex_t(*other.complex);
                break;
            }
            case STRING: {
                string = new string_t(*other.string);
                break;
            }
            case LIST: {
                list = new list_t(*other.list);
                break;
            }
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
            case BOOLEAN: {
                return boolean;
            }
            case INTEGER: {
                return 0 != integer;
            }
            case REAL: {
                return 0.0 != real;
            }
            case COMPLEX: {
                return 0.0 != complex->real() && 0.0 != complex->imag();
            }
            case STRING: {
                return !string->empty();
            }
        }

        return false;
    }

    integer_t to_integer() const {
        switch (type) {
            case BOOLEAN: {
                return boolean;
            }
            case INTEGER: {
                return integer;
            }
            case REAL: {
                return static_cast<integer_t>(real);
            }
            case COMPLEX: {
                return static_cast<integer_t>(complex->real());
            }
            case STRING: {
                return std::stoll(*string);
            }
        }

        return 0ll;
    }

    real_t to_real() const {
        switch (type) {
            case BOOLEAN: {
                return boolean;
            }
            case INTEGER: {
                return static_cast<real_t>(integer);
            }
            case REAL: {
                return real;
            }
            case COMPLEX: {
                return complex->real();
            }
            case STRING: {
                return std::stod(*string);
            }
        }

        return 0.0;
    }

    complex_t to_complex() const {
        if (COMPLEX == type) {
            return *complex;
        }

        return complex_t(to_real());
    }

    string_t to_string() const {
        switch (type) {
            case BOOLEAN: {
                return boolean ? STR("true") : STR("false");
            }
            case INTEGER: {
                return expr::to_string(integer);
            }
            case REAL: {
                return expr::to_string(real);
            }
            case COMPLEX: {
                return expr::to_string(complex->real()) + STR('+') + expr::to_string(complex->imag()) + STR('i');
            }
            case STRING: {
                return *string;
            }
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
