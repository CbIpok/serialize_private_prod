#pragma once

#include <variant>
#include <type_traits>
#include <cstdint>
#include <vector>
#include <string>
#include <cstddef>
#include <cstring>

using Id = uint64_t;
using Buffer = std::vector<std::byte>;

namespace detail {
    template<typename T>
    void writePrimitive(Buffer& buff, const T& val) {
        const auto raw = reinterpret_cast<const std::byte*>(&val);
        buff.insert(buff.end(), raw, raw + sizeof(T));
    }

    template<typename T>
    T readPrimitive(Buffer::const_iterator& it, Buffer::const_iterator) {
        T v;
        std::memcpy(&v, it.base(), sizeof(T));
        it += sizeof(T);
        return v;
    }
}