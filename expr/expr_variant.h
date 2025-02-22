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

#ifndef EXPR_VARIANT_H
#define EXPR_VARIANT_H

#include <cstring>
#include <algorithm>
#include "expr_common.h"

namespace expr {

using sequence_t = std::vector<struct variant>;

struct variant {
    enum variant_type {
        INVALID,
        BOOLEAN,
        REAL,
        COMPLEX,
        STRING,
        SEQUENCE
    };

    variant_type    type;
    union {
        bool        boolean;
        real_t      real;
        complex_t*  complex;
        string_t*   string;
        sequence_t* sequence;
    };

    variant() : type(INVALID) {}
    variant(bool value) : type(BOOLEAN), boolean(value) {}
    variant(real_t value) : type(REAL), real(value) {}
    variant(size_t value) : type(REAL), real(real_t(value)) {}
    variant(int value) : type(REAL), real(real_t(value)) {}
    variant(const complex_t& value) : type(COMPLEX), complex(new complex_t(value)) {}
    variant(const string_t& value) : type(STRING), string(new string_t(value)) {}
    variant(const char_t* value) : type(STRING), string(new string_t(value)) {}
    variant(const sequence_t& value) : type(SEQUENCE), sequence(new sequence_t(value)) {}

    variant(const variant& other) {
        copy(other);
    }

    variant(variant&& other) noexcept {
        move(std::move(other));
    }

    ~variant() {
        clear();
    }

    variant& operator=(const variant& other) {
        if (this != &other) {
            clear();
            copy(other);
        }

        return *this;
    }

    variant& operator=(variant&& other) noexcept {
        if (this != &other) {
            clear();
            move(std::move(other));
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
        case SEQUENCE:
            delete sequence;
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

    bool is_sequence() const {
        return SEQUENCE == type;
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

        return real_t(0);
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
            return expr::to_string(boolean);
        case REAL:
            return expr::to_string(real);
        case COMPLEX:
            return expr::to_string(*complex);
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
            return format(STR("\"%1\""), *string);
        case SEQUENCE: {
            string_array array(sequence->size());
            std::transform(sequence->begin(), sequence->end(), array.begin(), [](const variant& var) { return var.to_text(); });
            return format(STR("(%1)"), join(array, STR(",")));
        }
        }

        return string_t();
    }

private:
    void copy(const variant& other) {
        memcpy(this, &other, sizeof(variant));
        switch (type) {
        case COMPLEX:
            complex = new complex_t(*other.complex);
            break;
        case STRING:
            string = new string_t(*other.string);
            break;
        case SEQUENCE:
            sequence = new sequence_t(*other.sequence);
            break;
        }
    }

    void move(variant&& other) {
        memcpy(this, &other, sizeof(variant));
        other.type = INVALID;
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
        case variant::SEQUENCE:
            return *left.sequence == *right.sequence;
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
        case expr::variant::SEQUENCE: {
            size_t h2 = 0;
            for (const expr::variant& item : *var.sequence) {
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
