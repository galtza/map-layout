/*
    MIT License

    Copyright (c) 2016-2020 Raúl Ramos

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sub-license, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/

#pragma once

#include <iostream>
#include <iomanip>
#include <sstream>
#include <vector>
#include <array>
#include <map>
#include <cmath>
#include <limits.h>
#include <functional>
#include <cstring>
#include <regex>

namespace qcstudio {
namespace map_layout {
using namespace std;

/*
    == PUBLIC data structures and enumerations ==========
*/

enum item_category {
    undefined, arithmetic, bitfield, pointer, klass, container
};

struct item_t;
struct container_t {
    size_t count;
    item_t* items;
};

struct item_t {
    item_category category = item_category::undefined;
    union {
        uint8_t     encoded_arithmetic; // 0WZZZYXX (XX: bool/char/integer/real; Y: signed/unsigned; ZZZ: 1/2/4/8/16; W: char|wchar_t / char*_t)
        uint64_t    id;                 // class id as retrieved from 'id_of'
        container_t container;          // num items (for indexable types)
    } data;
    vector<size_t> ranges;
    item_t() = default;
    item_t(const item_t& _other) = default;
    ~item_t() {
        if (category == item_category::container && data.container.items) {
            delete[] data.container.items;
            data.container.items = nullptr;
        }
    }
};

struct field_info_t {
    uint64_t user_data;
    item_t   item;
};

struct class_layout {
    string                         name;
    uint32_t                       id = 0;
    size_t                         firstbit, lastbit;
    map<const char*, field_info_t> fields;
};

/*
    == PUBLIC C++ interface ==========
*/

using error_entry = tuple<const char*, size_t, string>;

template<typename T>     auto get_layout()        -> const class_layout&;
template<typename T>     auto get_type_errors()   -> const vector<error_entry>&; // file/line/error message
template<typename ...TS> auto gather_all_errors() -> vector<error_entry>;

template<typename ...TS> struct err_iterator;
template<typename ...TS> auto gather_all_errors() -> vector<error_entry> {
    vector<error_entry> ret;
    err_iterator<TS...>()(ret);
    return ret;
}

template<>
struct err_iterator<> {
    void operator()(vector<error_entry>&) {
    }
};

template<typename T, typename ...TS>
struct err_iterator<T, TS...> {
    void operator()(vector<error_entry>& _out) {
        auto& errs = get_type_errors<T>();
        std::copy(errs.begin(), errs.end(), std::back_inserter(_out));
        err_iterator<TS...>()(_out);
    }
};

/*
    == PUBLIC macros' interface ==========
*/

/*
    ML_[GLOBAL_]REGISTER_[BIT]FIELD(_class_, _field_[, _classname_, _fieldname_][, _user_data_])

    @param _class_      non-quoted class name
    @param _field_      non-quoted field name
    @param _classname_  quoted class name
    @param _fieldname_  quoted field name
    @param _user_data_  uint64_t value associated to the registry for user purposes

    These 16 macro combinations (2 ^ 4) can be used:
    - globally or inside a function (1st option),
    - for bit-field or regular chunks (2nd option)
    - by specifying the class and field names (3rd option)
    - by specifying user data (4th option)
*/

#define ML_REGISTER_FIELD(_class, ...)                          ML_CHOOSE(__VA_ARGS__, ML_BADP, ML_IMPL_RF5P,   ML_IMPL_RF4P,   ML_IMPL_RF3P,   ML_IMPL_RF2P  )(ML_WRAP(_class), __VA_ARGS__, __FILE__, __LINE__)
#define ML_REGISTER_BITFIELD(_class, ...)                       ML_CHOOSE(__VA_ARGS__, ML_BADP, ML_IMPL_RBF5P,  ML_IMPL_RBF4P,  ML_IMPL_RBF3P,  ML_IMPL_RBF2P )(ML_WRAP(_class), __VA_ARGS__, __FILE__, __LINE__)
#define ML_GLOBAL_REGISTER_FIELD(_class, ...)                   ML_CHOOSE(__VA_ARGS__, ML_BADP, ML_IMPL_GRF5P,  ML_IMPL_GRF4P,  ML_IMPL_GRF3P,  ML_IMPL_GRF2P )(ML_WRAP(_class), __VA_ARGS__, __FILE__, __LINE__)
#define ML_GLOBAL_REGISTER_BITFIELD(_class, ...)                ML_CHOOSE(__VA_ARGS__, ML_BADP, ML_IMPL_GRBF5P, ML_IMPL_GRBF4P, ML_IMPL_GRBF3P, ML_IMPL_GRBF2P)(ML_WRAP(_class), __VA_ARGS__, __FILE__, __LINE__)
#define ML_REGISTER_CLASSID(_class, _id, ...)                   ML_IMPL_REGISTER_CLASSID(ML_WRAP(_class), _id, __VA_ARGS__)
#define ML_REGISTER_CLASSID_CODITIONAL(_class, _cond, _id, ...) ML_IMPL_REGISTER_CLASSID_CODITIONAL(ML_WRAP(_class), ML_WRAP(_cond), _id, __VA_ARGS__)

/*
    == Extensibility ==========

    We can extend it...
    (i)  by declaring certain types as indexable (std::pair, std::tuple, etc.)
    (ii) by specifying ids for certain classes
*/

/*
    Define what is an indexable type via various template specializations
    (is_container, container_size and container_elem)

    Check out the specializations for std::pair, std::tuple and std::array below
*/

template<typename T> struct is_container   : false_type                   { };
template<typename T> struct container_size : integral_constant<size_t, 1> { };
template<size_t I, typename T>
auto container_elem(const T& _item) -> decltype(auto) {
    return _item;
}

// built-in std::pair specializations

template<typename T, typename U> struct is_container<pair<T, U>>   : true_type                    { };
template<typename T, typename U> struct container_size<pair<T, U>> : integral_constant<size_t, 2> { };

template<size_t I, typename T, typename U>
auto container_elem(const pair<T, U>& _item) -> decltype(auto) {
    return get<I>(_item);
}

// built-in std::tuple specializations

template<typename ...TYPES> struct is_container<tuple<TYPES...>>   : true_type                                   { };
template<typename ...TYPES> struct container_size<tuple<TYPES...>> : integral_constant<size_t, sizeof...(TYPES)> { };

template<size_t I, typename ...TYPES>
auto container_elem(const tuple<TYPES...>& _item) -> decltype(auto) {
    return get<I>(_item);
}

// built-in std::array specializations

template<typename T, size_t N> struct is_container<array<T, N>>   : true_type                    {};
template<typename T, size_t N> struct container_size<array<T, N>> : integral_constant<size_t, N> {};

template<size_t I, typename T, size_t N>
auto container_elem(const array<T, N>& _item) -> decltype(auto) {
    return get<I>(_item);
}

// built-in c-array specialization

template<typename T, size_t N> struct is_container<T[N]>   : true_type                    {};
template<typename T, size_t N> struct container_size<T[N]> : integral_constant<size_t, N> {};

template<size_t I, typename T, size_t N>
auto container_elem(T const (&_item)[N]) -> decltype(auto) {
    return _item[I];
}

/*
    'id_of'

    Rationale:

    As a class can have other classes as members, if we wanted to identify those instances
    there is the 'id_of' mechanism that we could use to store the id onside the layout
    information.

    Usage:

    struct my_class { ... };

    template<>
    struct qcstudio::map_layout::id_of<my_class> : std::integral_constant<uint32_t, 0x12345678> {
    };
*/

template<typename T, typename = void> // note: allow SFINAE
struct id_of : std::integral_constant<uint32_t, 0> {
};

/*
    PRIVATE macro implementation
*/

// Utility to be able to get offset of fields generically even for reference fields

namespace details {

    template<typename T>
    auto static_of() -> const T* {
        static unsigned char ret[sizeof(T)];
        return reinterpret_cast<const T*>(&ret[0]);
    }

}

// internal help macros

#define ML_WRAP(...) __VA_ARGS__
#define ML_CHOOSE(_P5, _P4, _P3, _P2, _P1, NAME, ...) NAME
#define ML_BADP(...) static_assert(false, "Invalid number of parameters")
#define ML_PASTE(x,y) x##y
#define ML_MERGE(x,y) ML_PASTE(x,y)
#define ML_UNUSED ML_MERGE(unused,__COUNTER__)
#define FIELD_OFFSET(_class, _field) \
    reinterpret_cast<size_t>(&reinterpret_cast<char const volatile&>((qcstudio::map_layout::details::static_of<_class>()->_field))) - \
    reinterpret_cast<size_t>(reinterpret_cast<char const volatile*>((qcstudio::map_layout::details::static_of<_class>())))

// internal macro implementation

#define ML_IMPL_RF5P(_class, _field, _classname, _fieldname, _user_data, _file, _line) /* register field; 5 parameters version */\
    do {\
        static auto unused =\
            qcstudio::map_layout::details::register_field<ML_WRAP(_class), decltype(ML_WRAP(_class)::_field)>(\
                reinterpret_cast<size_t>(&reinterpret_cast<char const volatile&>(((reinterpret_cast<ML_WRAP(_class)*>(0))->_field))),\
                _classname, _fieldname, _user_data, nullptr,\
                _file, _line\
            );\
        (void)unused;\
    } while (false)

#define ML_IMPL_RF4P(_class, _field, _classname, _fieldname, _file, _line) ML_IMPL_RF5P(ML_WRAP(_class), _field, _classname, _fieldname, 0,          _file, _line)
#define ML_IMPL_RF3P(_class, _field, _user_data, _file, _line)             ML_IMPL_RF5P(ML_WRAP(_class), _field, #_class,    #_field,    _user_data, _file, _line)
#define ML_IMPL_RF2P(_class, _field, _file, _line)                         ML_IMPL_RF5P(ML_WRAP(_class), _field, #_class,    #_field,    0,          _file, _line)

#define ML_IMPL_RBF5P(_class, _field, _classname, _fieldname, _user_data, _file, _line) /* register bit-field; 5 parameter version */\
    do {\
        static_assert(!is_reference<decltype(ML_WRAP(_class)::_field)>::value, "Reference attribute layout is not possible");\
        static auto unused =\
        qcstudio::map_layout::details::register_bitfield<ML_WRAP(_class), decltype(ML_WRAP(_class)::_field)>(\
            _classname, _fieldname, _user_data,\
            [](const ML_WRAP(_class)& _inst) { return _inst._field; },\
            [](ML_WRAP(_class)& _inst, auto _b) { _inst._field = _b; },\
            _file, _line\
        );\
        (void)unused;\
    } while (false)

#define ML_IMPL_RBF4P(_class, _field, _classname, _fieldname, _file, _line) ML_IMPL_RBF5P(ML_WRAP(_class), _field, _classname, _fieldname, 0,          _file, _line)
#define ML_IMPL_RBF3P(_class, _field, _user_data, _file, _line)             ML_IMPL_RBF5P(ML_WRAP(_class), _field, #_class,    #_field,    _user_data, _file, _line)
#define ML_IMPL_RBF2P(_class, _field, _file, _line)                         ML_IMPL_RBF5P(ML_WRAP(_class), _field, #_class,    #_field,    0,          _file, _line)

#define ML_IMPL_GRF5P(_class, _field, _classname, _fieldname, _user_data, _file, _line) /* global register field; 5 parameter version */\
    static auto ML_UNUSED =\
        qcstudio::map_layout::details::register_field<ML_WRAP(_class), decltype(ML_WRAP(_class)::_field)>(\
            (reinterpret_cast<size_t>(&reinterpret_cast<char const volatile&>(((reinterpret_cast<ML_WRAP(_class)*>(0))->_field)))),\
            _classname, _fieldname, _user_data, nullptr,\
            _file, _line\
        )\

#define ML_IMPL_GRF4P(_class, _field, _classname, _fieldname, _file, _line) ML_IMPL_GRF5P(ML_WRAP(_class), _field, _classname, _fieldname, 0,          _file, _line)
#define ML_IMPL_GRF3P(_class, _field, _user_data, _file, _line)             ML_IMPL_GRF5P(ML_WRAP(_class), _field, #_class,    #_field,    _user_data, _file, _line)
#define ML_IMPL_GRF2P(_class, _field, _file, _line)                         ML_IMPL_GRF5P(ML_WRAP(_class), _field, #_class,    #_field,    0,          _file, _line)

#define ML_IMPL_GRBF5P(_class, _field, _classname, _fieldname, _user_data, _file, _line) /* global register bitfield; 5 paramameter version */\
    static auto ML_UNUSED =\
        qcstudio::map_layout::details::register_bitfield<ML_WRAP(_class), decltype(ML_WRAP(_class)::_field)>(\
            _classname, _fieldname, _user_data,\
            [](const ML_WRAP(_class)& _inst)    { return _inst._field; },\
            [](ML_WRAP(_class)& _inst, auto _b) { _inst._field = _b; },\
            _file, _line\
        )

#define ML_IMPL_GRBF4P(_class, _field, _classname, _fieldname, _file, _line) ML_IMPL_GRBF5P(ML_WRAP(_class), _field, _classname, _fieldname, 0,          _file, _line)
#define ML_IMPL_GRBF3P(_class, _field, _user_data, _file, _line)             ML_IMPL_GRBF5P(ML_WRAP(_class), _field, #_class,    #_field,    _user_data, _file, _line)
#define ML_IMPL_GRBF2P(_class, _field, _file, _line)                         ML_IMPL_GRBF5P(ML_WRAP(_class), _field, #_class,    #_field,    0,          _file, _line)

#define ML_IMPL_REGISTER_CLASSID(_class, _id, ...)\
    template <__VA_ARGS__>\
    struct qcstudio::map_layout::id_of<ML_WRAP(_class)> : std::integral_constant<uint32_t, _id> {\
    }

#define ML_IMPL_REGISTER_CLASSID_CODITIONAL(_class, _cond, _id, ...)\
    template <__VA_ARGS__>\
    struct qcstudio::map_layout::id_of<ML_WRAP(_class), ML_WRAP(_cond)> : std::integral_constant<uint32_t, _id> {\
    }

/*
    == PRIVATE Implementation details ==========
*/

// layout access

template<typename T>
auto get_layout() -> const class_layout& {
    static class_layout ret;
    return ret;
}

// error list access

template<typename T>
auto get_type_errors() -> const vector<tuple<const char*, size_t, string>>& {
    static vector<tuple<const char*, size_t, string>> ret;
    return ret;
}

namespace details {

    // debug


    inline auto decode_arithmetic(uint8_t _enc) -> const char* {
        auto XX  =  _enc & 0b00000011;
        auto Y   = (_enc & 0b00000100) >> 2;
        auto ZZZ = (_enc & 0b00111000) >> 3;
        auto W   = (_enc & 0b01000000) >> 6;
        // 0WZZZYXX
        // XX:  bool/char/integer/real;
        // Y:   signed/unsigned;
        // ZZZ: 1/2/4/8/16 bytes;
        // W:   char|wchar_t / char*_t)
        switch (XX) {
            case /*bool*/0: return "bool";
            case /*char*/1: {
                if (ZZZ == 0) {
                    if (W) {
                        return "char8_t";
                    } else {
                        switch (Y) {
                            case /*signed*/  0: return "char";
                            case /*unsigned*/1: return "unsigned char";
                        }
                    }
                } else {
                    if (W == 0) {
                        return "wchar_t";
                    } else {
                        switch (ZZZ) {
                            case 1: return "char16_t";
                            case 2: return "char32_t";
                        }
                    }
                }
                break;
            }
            case /*integer*/2: {
                switch (ZZZ) {
                    case 0: return Y? "uint8_t"  : "int8_t";
                    case 1: return Y? "uint16_t" : "int16_t";
                    case 2: return Y? "uint32_t" : "int32_t";
                    case 3: return Y? "uint64_t" : "int64_t";
                }
                break;
            }
            case /*real*/3: {
                switch (ZZZ) {
                    case 2: return "float";
                    case 3: return "double";
                    case 4: return "long double";
                }
                break;
            }
        }
        return "undefined";
    }
}

namespace details {

// write-access to layout

template<typename T>
static auto get_layout_mod() -> class_layout&
{
    return const_cast<class_layout&>(get_layout<T>());
}

// error handling

template<typename T>
auto get_type_errors_mod() -> vector<tuple<const char*, size_t, string>>& {
    return const_cast<vector<tuple<const char*, size_t, string>>&>(get_type_errors<T>());
}

template<typename T>
void add_error(const char* _file, size_t _line, string&& _message) {
    auto& errors = get_type_errors_mod<T>();
    errors.push_back(make_tuple(_file, _line, move(_message)));
    sort(errors.begin(), errors.end(), [](auto& _a, auto& _b) { return get<1>(_a) < get<1>(_b); });
}

template<typename T>
void add_duplication_error(const char* _file, size_t _line, const char* _classname, const char* _fieldname) {
    stringstream out;
    out << "Duplicated field registration " << _classname << "::" << _fieldname;
    add_error<T>(_file, _line, out.str());
}

/*
    'is_defined' utility used to know when a type is defined
*/

template<typename T, size_t = sizeof(T)>
auto is_defined_helper(T*) -> true_type;
auto is_defined_helper(...) -> false_type;

template<typename T>
struct is_defined : integral_constant<bool, is_same<true_type, decltype(is_defined_helper(declval<T*>()))>::value> {
};

/*
    trait 'is_one_of' to compare a type against a list of types
*/

template<typename...>                           struct is_one_of             : integral_constant<bool, false>                                             { };
template<typename F, typename S, typename... T> struct is_one_of<F, S, T...> : integral_constant<bool, is_same<F, S>::value || is_one_of<F, T...>::value> { };

/*
    'IF' Help aliases
*/

template<typename T,           typename U = void>    using if_arithmetic          = typename enable_if<is_arithmetic<T>::value, U>::type;
template<typename T,           typename U = void>    using if_pointer             = typename enable_if<is_pointer<T>::value, U>::type;
template<typename T,           typename U = void>    using if_ref                 = typename enable_if<is_reference<T>::value, U>::type;
template<typename T,           typename U = void>    using if_not_ref             = typename enable_if<!is_reference<T>::value, U>::type;
template<typename T,           typename U = void>    using if_non_container_class = typename enable_if<is_class<T>::value && !is_container<T>::value, U>::type;
template<typename T,           typename U = void>    using if_container           = typename enable_if<is_container<T>::value, U>::type;
template<typename T,           typename U = uint8_t> using if_bool                = typename enable_if<is_same<T, bool>::value, U>::type;
template<typename T,           typename U = uint8_t> using if_char                = typename enable_if<is_one_of<T, char, wchar_t, char16_t, char32_t>::value, U>::type;
template<typename T,           typename U = uint8_t> using if_charchar            = typename enable_if<is_one_of<T, unsigned char, char, wchar_t>::value, U>::type;
template<typename T,           typename U = uint8_t> using if_charnum             = typename enable_if<is_one_of<T, char16_t, char32_t>::value, U>::type;
template<typename T,           typename U = uint8_t> using if_no_char             = typename enable_if<!is_one_of<T, unsigned char, char, wchar_t, char16_t, char32_t>::value, U>::type;
template<typename T,           typename U = uint8_t> using if_integer             = typename enable_if<numeric_limits<T>::is_integer && !is_one_of<T, bool, char, wchar_t, char16_t, char32_t>::value, U>::type;
template<typename T,           typename U = uint8_t> using if_floating_point      = typename enable_if<is_floating_point<T>::value, U>::type;
template<typename T, size_t S, typename U = uint8_t> using if_sizeof_is           = typename enable_if<sizeof(T) == S, U>::type;
template<typename T,           typename U = uint8_t> using if_signed              = typename enable_if<is_signed<T>::value, U>::type;
template<typename T,           typename U = uint8_t> using if_unsigned            = typename enable_if<is_unsigned<T>::value, U>::type;

/*
    Arithmetic flags deduction

    The format is 0WZZZYXX where
    -  XX: bool/char/integer/real [arithmetic category flags]
    -   Y: signed/unsigned        [sign flags]
    - ZZZ: 1/2/4/8/16 bytes       [size flags]
    -   W: char|wchar_t / char*_t [char falgs]
*/

template<typename T> auto get_arithmetic_type_flags () ->           if_bool<T>     { return 0b00;        }
template<typename T> auto get_arithmetic_type_flags () ->           if_char<T>     { return 0b01;        }
template<typename T> auto get_arithmetic_type_flags () ->        if_integer<T>     { return 0b10;        }
template<typename T> auto get_arithmetic_type_flags () -> if_floating_point<T>     { return 0b11;        }

template<typename T> auto get_sign_flags            () ->         if_signed<T>     { return 0b0    << 2; }
template<typename T> auto get_sign_flags            () ->       if_unsigned<T>     { return 0b1    << 2; }

template<typename T> auto get_size_flags            () ->      if_sizeof_is<T,  1> { return 0b0000 << 3; }
template<typename T> auto get_size_flags            () ->      if_sizeof_is<T,  2> { return 0b0001 << 3; }
template<typename T> auto get_size_flags            () ->      if_sizeof_is<T,  4> { return 0b0010 << 3; }
template<typename T> auto get_size_flags            () ->      if_sizeof_is<T,  8> { return 0b0011 << 3; }
template<typename T> auto get_size_flags            () ->      if_sizeof_is<T, 16> { return 0b0100 << 3; }

template<typename T> auto get_char_flags            () ->       if_charchar<T>     { return 0b0    << 6; }
template<typename T> auto get_char_flags            () ->        if_charnum<T>     { return 0b1    << 6; }
template<typename T> auto get_char_flags            () ->        if_no_char<T>     { return 0b0    << 6; }

template<typename T>
auto get_encoded_arithmetic() -> if_arithmetic<T, uint8_t> {
    return get_arithmetic_type_flags<T> ()
         | get_sign_flags<T>            ()
         | get_size_flags<T>            ()
         | get_char_flags<T>            ();
}

// initial setup of class/field

inline auto get_filtered_classname(const char* _classname) -> string {

    const auto wrap = regex{ "ML_WRAP\\s*\\((.*)\\)" };
    const auto find_type = regex{ "(\\w+\\s*(<[^>]+>)?)" };

    smatch m;
    auto current = string { _classname };
    do {
        current = regex_replace(current.c_str(), wrap, "$1");
    } while(regex_search(current, m, wrap));

    if (regex_search(current, m, find_type)) {
        return m[1].str();
    }
    return _classname;
}

template<typename CLASS, typename FIELD>
auto setup_class_field(const char* _classname, const char* _fieldname, uint64_t _user_data, const char* _file, size_t _line) -> field_info_t* {

    auto filtered_classname = get_filtered_classname(_classname);
    auto& layout = get_layout_mod<CLASS>();
    if (layout.name.empty()) {
        layout.id = id_of<CLASS>::value;
        layout.name = filtered_classname;
        layout.firstbit = numeric_limits<decltype(layout.firstbit)>::max();
        layout.lastbit = numeric_limits<decltype(layout.lastbit )>::min();
    }

    auto ret = layout.fields.insert({_fieldname, field_info_t{_user_data, {}}});
    if (ret.second) {
        ret.first->second.user_data = _user_data;
        return &(ret.first->second);
    }

    details::add_duplication_error<CLASS>(_file, _line, filtered_classname.c_str(), _fieldname);

    return nullptr;
}

// Arithmetic types

template<typename CLASS, typename FIELD>
auto register_field(size_t _offset, const char* _classname, const char* _fieldname, uint64_t _user_data, item_t* _item, const char* _file, size_t _line)
-> if_arithmetic<FIELD, bool> {

    auto item = _item;
    if (!item) {
        if (auto field = setup_class_field<CLASS, FIELD>(_classname, _fieldname, _user_data, _file, _line)) {
            item = &field->item;
        }
    }

    if (!item) {
        return false;
    }

    item->category                = item_category::arithmetic;
    item->data.encoded_arithmetic = get_encoded_arithmetic<FIELD>();
    item->ranges.push_back (_offset * 8);
    item->ranges.push_back (((_offset + sizeof(FIELD)) * 8) - 1);

    auto& layout = get_layout_mod<CLASS>();
    for (auto i = 0u; i < item->ranges.size(); i+=2) {
        if (item->ranges[i    ] < layout.firstbit) { layout.firstbit = item->ranges[i    ]; }
        if (item->ranges[i + 1] > layout.lastbit)  { layout.lastbit  = item->ranges[i + 1]; }
    }

    return true;
}

// Pointers and references

template<typename CLASS, typename FIELD>
auto register_field(size_t _offset, const char* _classname, const char* _fieldname, uint64_t _user_data, item_t* _item, const char* _file, size_t _line)
-> if_pointer<FIELD, bool> {

    auto item = _item;
    if (!item) {
        if (auto field = setup_class_field<CLASS, FIELD>(_classname, _fieldname, _user_data, _file, _line)) {
            item = &field->item;
        }
    }

    if (!item) {
        return false;
    }

    item->category = item_category::pointer;
    item->ranges.push_back (_offset * 8);
    item->ranges.push_back (((_offset + sizeof(FIELD)) * 8) - 1);

    auto& layout = get_layout_mod<CLASS>();
    for (auto i = 0u; i < item->ranges.size(); i+=2) {
        if (item->ranges[i    ] < layout.firstbit) { layout.firstbit = item->ranges[i    ]; }
        if (item->ranges[i + 1] > layout.lastbit)  { layout.lastbit  = item->ranges[i + 1]; }
    }

    return true;
}

template<typename CLASS, typename FIELD>
auto register_field(size_t /*_offset*/, const char* /*_classname*/, const char* /*_fieldname*/, uint64_t /*_user_data*/, item_t* /*_item*/, const char* /*_file*/, size_t /*_line*/)
-> if_ref<FIELD, bool> {
    static_assert(!is_reference<FIELD>::value, "Reference attribute layout is not possible");
    return false;
}

// Identifiable/no-identifiable classes

template<typename CLASS, typename FIELD>
auto register_field(size_t _offset, const char* _classname, const char* _fieldname, uint64_t _user_data, item_t* _item, const char* _file, size_t _line)
-> if_non_container_class<FIELD, bool> {

    auto item = _item;
    if (!item) {
        if (auto field = setup_class_field<CLASS, FIELD>(_classname, _fieldname, _user_data, _file, _line)) {
            item = &field->item;
        }
    }

    if (!item) {
        return false;
    }

    item->category = item_category::klass;
    item->data.id = id_of<FIELD>::value;;
    item->ranges.push_back (_offset * 8);
    item->ranges.push_back (((_offset + sizeof(FIELD)) * 8) - 1);

    auto& layout = get_layout_mod<CLASS>();
    for (auto i = 0u; i < item->ranges.size(); i+=2) {
        if (item->ranges[i    ] < layout.firstbit) { layout.firstbit = item->ranges[i    ]; }
        if (item->ranges[i + 1] > layout.lastbit)  { layout.lastbit  = item->ranges[i + 1]; }
    }

    return true;
}

// Indexable types

inline auto get_max_bit (size_t _val, item_t* _item) -> size_t {
    if (!_item) {
        return _val;
    }

    if (_item->category == item_category::container) {
        for (auto i = 0u; i < _item->data.container.count; ++i) {
            auto tmp = get_max_bit(_val, &_item->data.container.items[i]);
            if (tmp > _val) {
                _val = tmp;
            }
        }
    } else {
        return max(_val, _item->ranges.back());
    }
    return _val;
}

template<typename CLASS, typename FIELD, size_t IDX, size_t SIZE>
struct register_container;

template<typename CLASS, typename FIELD>
auto register_field(
    size_t      _offset,
    const char* _classname,
    const char* _fieldname,
    uint64_t    _user_data,
    item_t*     _item,
    const char* _file,
    size_t      _line
) -> if_container<FIELD, bool> {

    auto item = _item;
    if (!item) {
        if (auto field = setup_class_field<CLASS, FIELD>(_classname, _fieldname, _user_data, _file, _line)) {
            item = &field->item;
        }
    }

    if (!item) {
        return false;
    }

    item->category = item_category::container;
    item->data.container.count = container_size<FIELD>::value;
    item->data.container.items = new item_t[container_size<FIELD>::value];
    item->ranges.push_back(_offset * 8);
    item->ranges.push_back(_offset * 8);
    register_container<CLASS, FIELD, 0, container_size<FIELD>::value>()(_offset, /*WE CAN USE THIS FUNCTION INSIDE THE CLASS, CAN WE?*/
        *static_of<FIELD>(), item, _file, _line);

    item->ranges.back() = details::get_max_bit(0, item);

    auto& layout = get_layout_mod<CLASS>();
    for (auto i = 0u; i < item->ranges.size(); i+=2) {
        if (item->ranges[i    ] < layout.firstbit) { layout.firstbit = item->ranges[i    ]; }
        if (item->ranges[i + 1] > layout.lastbit)  { layout.lastbit  = item->ranges[i + 1]; }
    }

    return true;
}

template<typename CLASS, typename FIELD, size_t SIZE>
struct register_container<CLASS, FIELD, SIZE, SIZE> {
    constexpr auto operator()(size_t /*_offset*/, const FIELD& /*_field*/, item_t* /*_container_item*/, const char* /*_file*/, size_t /*_line*/) -> size_t {
        return 0;
    }
};

template<typename CLASS, typename FIELD, size_t IDX, size_t SIZE>
struct register_container {
    constexpr auto operator()(size_t _offset, const FIELD& _field, item_t* _container_item, const char* _file, size_t _line) -> size_t {
        using item_type = decltype(container_elem<IDX>(_field));

        static_assert(is_reference<item_type>::value, "item accessor function must return a reference");
        auto elem_idx_ptr = reinterpret_cast<uintptr_t>(&container_elem<IDX>(_field));
        auto elem_0_ptr   = reinterpret_cast<uintptr_t>(&_field);
        auto item_offset  = elem_idx_ptr - elem_0_ptr;
        register_field<CLASS, typename decay<item_type>::type>(
            _offset + item_offset, nullptr, nullptr, 0, &_container_item->data.container.items[IDX],
            _file, _line
        );
        return register_container<CLASS, FIELD, IDX + 1, container_size<FIELD>::value>()(_offset, _field, _container_item, _file, _line);
    }
};

// bit-field registering functions

template<typename CLASS, typename FIELD>
auto register_bitfield(const char* /*_classname*/, const char* /*_fieldname*/, uint64_t /*_user_data*/,
    const function<FIELD(const CLASS&)>& /*_getter*/,
    const function<void(CLASS&, FIELD)>& /*_setter*/,
    const char* /*_file*/, size_t /*_line*/
) -> if_ref<FIELD, bool> {
    static_assert(is_integral<typename decay<FIELD>::type>::value, "Only integral types can be bit fields and the specified type is not");
    static_assert(!is_reference<FIELD>::value, "Reference attribute layout is not possible");
}

template<typename CLASS, typename FIELD>
auto register_bitfield(const char* _classname, const char* _fieldname, uint64_t _user_data,
    const function<FIELD(const CLASS&)>& _getter,
    const function<void(CLASS&, FIELD)>& _setter,
    const char* _file, size_t _line
) -> if_not_ref<FIELD, bool> {

    static_assert(is_integral<typename decay<FIELD>::type>::value, "Only integral types can be bit fields and the specified type is not");

    auto field = setup_class_field<CLASS, FIELD>(_classname, _fieldname, _user_data, _file, _line);
    if (!field) {
        return false;
    }

    field->item.category = item_category::bitfield;
    field->item.data.encoded_arithmetic = get_encoded_arithmetic<FIELD>();

    static unsigned char buffer[sizeof(CLASS)];
    auto& layout   = details::get_layout_mod<CLASS>();
    auto  instance = reinterpret_cast<CLASS*>(buffer);

    // local functions

    const auto get_first_bit_position = [](unsigned char* _addr, int _max_bytes_scan) {
        for (auto pc = _addr; pc < (_addr + _max_bytes_scan); ++pc) {
            if (auto val = *pc) {
                for (auto i = 0; i < CHAR_BIT; ++i) {
                    if (val & (1 << i)) {
                        return (static_cast<int64_t>(pc - _addr) * CHAR_BIT) + i;
                    }
                }
            }
        }
        return int64_t{-1};
    };

    // zero the instance, set bit 1, check where it activated

    memset(reinterpret_cast<void*>(instance), 0, sizeof(CLASS));
    _setter(*instance, 1);
    const auto first_bit = get_first_bit_position(reinterpret_cast<unsigned char*>(instance), sizeof(CLASS));
    if (first_bit == -1) {
        return false;
    }

    // keep shifting value left, set it / get it, register where the bit is until we get out of

    auto curr_bit = static_cast<size_t>(first_bit);
    auto last_bit = curr_bit;

    const auto max_num_bits = (CHAR_BIT * sizeof(typename decay<decltype(_getter(declval<CLASS>()))>::type));
    for (auto offset = 1u; offset < max_num_bits; ++offset) {
        auto expected = 1u << offset;

        // Zero the instance, set / get and compare

        memset(reinterpret_cast<void*>(instance), 0, sizeof(CLASS));
        _setter(*instance, static_cast<FIELD>(expected));
        auto result = _getter(*instance);
        if (result != static_cast<decltype(result)>(expected) && static_cast<int>(result) >= 0) {
            field->item.ranges.push_back(curr_bit);
            field->item.ranges.push_back(last_bit);
            break;
        }

        // if the current bit distance to previous one is higher than 1 we create another range

        auto nthbit = get_first_bit_position(reinterpret_cast<unsigned char*>(instance), sizeof(CLASS));
        if (nthbit != -1) {
            auto idx = static_cast<size_t>(nthbit);
            if (abs(static_cast<int64_t>(idx - last_bit)) > 1) {
                field->item.ranges.push_back(curr_bit);
                field->item.ranges.push_back(last_bit);
                curr_bit = idx;
            }
            last_bit = idx;
        }
    }

    // Maybe the bitfield covers the whole range of bits

    if (field->item.ranges.size() == 0)
    {
        field->item.ranges.push_back(curr_bit);
        field->item.ranges.push_back(last_bit);
    }

    // Update the global range

    for (auto i = 0u; i < field->item.ranges.size(); i+=2) {
        if (field->item.ranges[i    ] < layout.firstbit) { layout.firstbit = field->item.ranges[i    ]; }
        if (field->item.ranges[i + 1] > layout.lastbit)  { layout.lastbit  = field->item.ranges[i + 1]; }
    }

    return true;
}

} // namespace details

} // namespace map_layout
} // namespace qcstudio
