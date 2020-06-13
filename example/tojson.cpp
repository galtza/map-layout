#include "tojson.h"

using namespace std;
using namespace qcstudio::map_layout;

auto to_string(item_category _category) -> const char* {
    switch (_category) {
        case undefined:  return "undefined";
        case arithmetic: return "arithmetic";
        case bitfield:   return "bitfield";
        case pointer:    return "pointer";
        case klass:      return "klass";
        case container:  return "container";
    }
    return "unknown";
}

auto get_arithmetic_type(uint8_t _enc) -> const char* {
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

auto to_json(const qcstudio::map_layout::item_t& _item) -> std::string {
    stringstream out;
    out << quote("category") << " : " << quote(to_string(_item.category)) << ", " << cr();
    switch (_item.category) {
        case item_category::undefined:
        case item_category::pointer: {
            break;
        }
        case item_category::bitfield:
        case item_category::arithmetic: {
            out << quote("type") << " : ";
            out << quote(get_arithmetic_type(_item.data.encoded_arithmetic)) << ", " << cr();
            break;
        }
        case item_category::klass: {
            out << quote("id") << " : " << _item.data.id << ", " << cr();
            break;
        }
        case item_category::container: {
            out << quote("num_items") << " : " << _item.data.container.count << ", " << cr();
            break;
        }
    }

    const auto nranges = _item.ranges.size();
    auto size = size_t{0};
    for (auto i = 0u; i < nranges - 1; i+=2) {
        size += static_cast<size_t>(_item.ranges[i+1] - _item.ranges[i] + 1);
    }
    out << quote("ranges") << " : " << "[ ";
    for (auto i = 0u; i < nranges; ++i) {
        out << _item.ranges[i];
        if (i < nranges-1) {
            out << ", ";
        }
    }
    out << " ], " << cr();

    if (_item.category == item_category::bitfield) {
        out << quote("bits") << " : " << size;
    } else {
        out << quote("bytes") << " : " << size / 8;
    }

    if (_item.category == item_category::container) {
        out << ", " << cr();
        out << quote("items") << " : " << cr();
        out << "[" << cr()++;
        for (auto i = 0u; i < _item.data.container.count; ++i) {
            out << "{" << cr()++;
            out << to_json(_item.data.container.items[i]);
            out << cr()-- << "}";
            if (i != _item.data.container.count - 1) {
                out << ", " << cr();
            }
        }
        out << cr()-- << "]";
    }
    return out.str();
}

auto to_json(const qcstudio::map_layout::class_layout& _layout) -> std::string {
    stringstream out;

    out << "{" << cr()++;
        out << quote(_layout.name) << " : " << cr();
        out << "{" << cr()++;
            out << quote("id") << " : " << dec << _layout.id << ", " << cr();
            out << quote("fields") << " : " << cr();
            out << "{" << cr()++;
            {
                auto i = 0u;
                for (auto& [ name, info ] : _layout.fields) {
                    out << quote(name) << " : " << cr();
                    out << "{" << cr()++;
                        out << quote("user_data") << " : " << info.user_data << "," << cr();
                        out << quote("item") << " : " << cr();
                        out << "{" << cr()++;
                            out << to_json(info.item);
                        out << cr()-- << "}";
                    out << cr()-- << "}";
                    if (i < _layout.fields.size() - 1) {
                        out << "," << cr();
                    }
                    ++i;
                }
            }
            out << cr()-- << "}";
        out << cr()-- << "}";
    out << cr()-- << "}";

    return out.str();
}

