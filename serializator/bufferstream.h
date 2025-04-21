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
		// copy sizeof(T) bytes from the byte‐buffer into v
		std::memcpy(
			static_cast<void*>(&v),
			static_cast<const void*>(&*it),
			sizeof(T)
		);
		it += sizeof(T);  // advance by that many bytes
		return v;

	}
}