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
        clear();
    }

    variant& operator=(const variant& other) {
        if (this != &other) {
            clear();
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
        }

        return *this;
    }

    variant& operator=(variant&& other) noexcept {
        if (this != &other) {
            clear();
            memcpy(this, &other, sizeof(variant));
            other.type = INVALID;
        }

        return *this;
    }

    void clear() {
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

        type = INVALID;
    }

    bool is_valid() const {
        return INVALID != type;
    }

    bool is_boolean() const {
        return BOOLEAN == type;
    }

    bool is_real() const {
        return REAL == type;
    }

    bool is_complex() const {
        return COMPLEX == type;
    }

    bool is_string() const {
        return STRING == type;
    }

    bool is_list() const {
        return LIST == type;
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
        }

        return string_t();
    }

    string_t to_text() const {
        switch (type) {
        case BOOLEAN:
        case REAL:
        case COMPLEX:
            return to_string();
        case STRING:
            return STR('\"') + *string + STR('\"');
        case LIST: {
            string_t str;
            for (const variant& item : *list) {
                if (!str.empty()) {
                    str += STR(',');
                }
                str += item.to_text();
            }
            return STR('(') + str + STR(')');
        }
        }

        return string_t();
    }
};

inline bool operator==(const variant& left, const variant& right) noexcept {
    if (&left == &right) {
        return true;
    }

    if (left.type == right.type) {
        switch (left.type) {
        case variant::INVALID:
            return true;
        case variant::BOOLEAN:
            return left.boolean == right.boolean;
        case variant::REAL:
            return left.real == right.real;
        case variant::COMPLEX:
            return *left.complex == *right.complex;
        case variant::STRING:
            return *left.string == *right.string;
        case variant::LIST:
            return *left.list == *right.list;
        }
    }

    return false;
}

inline bool operator!=(const variant& left, const variant& right) noexcept {
    return !(left == right);
}

}

namespace std {

template<>
struct hash<expr::variant> {
    static size_t combine(size_t h1, size_t h2) {
        return h1 ^ (h2 << 1);
    }

    size_t operator()(const expr::variant var) const noexcept {
        size_t h1 = hash<int>()(var.type);
        switch (var.type) {
        case expr::variant::BOOLEAN:
            return combine(h1, hash<bool>()(var.boolean));
        case expr::variant::REAL:
            return combine(h1, hash<expr::real_t>()(var.real));
        case expr::variant::COMPLEX:
            return combine(h1, combine(hash<expr::real_t>()(var.complex->real()), hash<expr::real_t>()(var.complex->imag())));
        case expr::variant::STRING:
            return combine(h1, hash<expr::string_t>()(*var.string));
        case expr::variant::LIST: {
            size_t h2 = 0;
            for (const expr::variant& item : *var.list) {
                size_t h = hash<expr::variant>()(item);
                h2 = (h2 ? combine(h2, h) : h);
            }
            return combine(h1, h2);
        }
        }

        return h1;
    }
};

}

#endif
