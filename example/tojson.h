#pragma once

#include <string>
#include <sstream>
#include "map_layout.h"

/*
    Tabs control while using basic_ostream
*/

struct tabinfo {
    static auto count() -> int& { static int ret = 0; return ret; }
    static auto size()  -> int& { static int ret = 2; return ret; }
};

struct cmd_cr {
    int inc = 0;
    auto operator++(int) -> cmd_cr  { return cmd_cr{1}; }
    auto operator--(int) -> cmd_cr  { return cmd_cr{-1}; }

    template<typename ELEM, typename TRAITS>
    friend auto operator << (std::basic_ostream<ELEM, TRAITS>& _os, const cmd_cr& _cmd) 
    -> std::basic_ostream<ELEM, TRAITS>& {
        _os.put(_os.widen('\n'));
        if (_cmd.inc != 0) {
            tabinfo::count() += _cmd.inc;
            if (tabinfo::count() < 0) {
                tabinfo::count() = 0;
            }
        }
        _os << std::string(static_cast<size_t>(tabinfo::size() * tabinfo::count()), ' ');
        return _os;
    }
};

inline auto cr() -> cmd_cr { 
    return cmd_cr(); 
}

template<typename T>
auto quote(const T& _data) -> std::string {
    std::stringstream out;
    out << "\"" << _data << "\"";
    return out.str();
}

/*
    type to json
    usage:
        cout << to_json<type1, type2, type3>();
*/

auto to_json(const qcstudio::map_layout::class_layout& _layout) -> std::string;

template<typename ...TS> struct type_iterator;
template<typename ...TS>
auto to_json() -> std::string {
    std::stringstream out;
    out << "{" << cr()++;
    out << quote("classes") << " : " << cr();
    out << "[" << cr()++;
    type_iterator<TS...>()(out);
    out << cr()-- << "]";
    out << cr()-- << "}\n";
    return out.str();
}

template<>
struct type_iterator<> {
    void operator()(std::stringstream&) {
    }
};

template<typename T, typename ...TS>
struct type_iterator<T, TS...> {
    void operator()(std::stringstream& _out) {
        auto& layout = qcstudio::map_layout::get_layout<T>();
        if (!layout.name.empty()) {
            _out << to_json(layout);
            if (sizeof...(TS)) {
                _out << ", " << cr();
            }
        }
        type_iterator<TS...>()(_out);
    }
};
