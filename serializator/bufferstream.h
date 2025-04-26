#pragma once

#include <variant>
#include <type_traits>
#include <cstdint>
#include <vector>
#include <string>
#include <cstddef>
#include <cstring>
#include <stdexcept>

using Id = uint64_t;
using Buffer = std::vector<std::byte>;

namespace detail {
	inline void is_available(Buffer::const_iterator it,
		Buffer::const_iterator end,
		std::size_t len)
	{
		auto remaining = std::distance(it, end);
		if (remaining < static_cast<std::ptrdiff_t>(len)) {
			throw std::out_of_range(
				"Buffer underflow: need " + std::to_string(len) +
				" bytes, but only " + std::to_string(remaining) + " remain"
			);
		}
	}

	template<typename T>
	void writePrimitive(Buffer& buff, const T& val) {
		const auto raw = reinterpret_cast<const std::byte*>(&val);
		buff.insert(buff.end(), raw, raw + sizeof(T));
	}

	template<typename T>
	T readPrimitive(Buffer::const_iterator& it, Buffer::const_iterator end) {
		T v;
		is_available(it, end, sizeof(T));
		std::memcpy(
			static_cast<void*>(&v),
			static_cast<const void*>(&*it),
			sizeof(T)
		);
		it += sizeof(T);  
		return v;

	}
}