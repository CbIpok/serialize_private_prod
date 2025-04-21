//serialize.hpp
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

enum class TypeId : Id {
    Uint,
    Float,
    String,
    Vector
};

#if defined(_WIN32) || defined(_WIN64)
static constexpr bool HOST_LITTLE_ENDIAN = true;
#elif defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && defined(__ORDER_BIG_ENDIAN__)
static constexpr bool HOST_LITTLE_ENDIAN =
(__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__);
#else
#   error "Cannot determine host endianness"
#endif

namespace detail {

    template<typename T>
    void writePrimitive(Buffer& buff, const T& val) {
        const auto raw = reinterpret_cast<const std::byte*>(&val);
        if constexpr (HOST_LITTLE_ENDIAN) {
            buff.insert(buff.end(), raw, raw + sizeof(T));
        }
        else {
            for (size_t i = 0; i < sizeof(T); ++i) {
                buff.push_back(raw[sizeof(T) - 1 - i]);
            }
        }
    }


    template<typename T>
    T readPrimitive(Buffer::const_iterator& it, Buffer::const_iterator) {
        T v;
        std::byte temp[sizeof(T)];
        if constexpr (HOST_LITTLE_ENDIAN) {
            std::memcpy(&v, &*it, sizeof(T));
        }
        else {
            for (size_t i = 0; i < sizeof(T); ++i) {
                temp[sizeof(T) - 1 - i] = *(it + i);
            }
            std::memcpy(&v, temp, sizeof(T));
        }
        it += sizeof(T);
        return v;
    }

}

template<typename Derived, typename ValueT, TypeId KTypeId>
class BaseType {
public:
    explicit BaseType(ValueT v)
        : value_(std::move(v))
    {
    }

    void serialize(Buffer& buff) const {
        detail::writePrimitive<Id>(buff, static_cast<Id>(KTypeId));
        detail::writePrimitive<ValueT>(buff, value_);
    }

    static Derived deserialize(Buffer::const_iterator& it,
        Buffer::const_iterator end) {
        auto val = detail::readPrimitive<ValueT>(it, end);
        return Derived{ std::move(val) };
    }

    static constexpr TypeId static_type_id = KTypeId;

    bool operator==(Derived const& other) const {
        return value_ == other.value_;
    }

protected:
    ValueT value_;
};


class IntegerType
    : public BaseType<IntegerType, uint64_t, TypeId::Uint>
{
public:
    using BaseType::BaseType;
};


class FloatType
    : public BaseType<FloatType, double, TypeId::Float>
{
public:
    using BaseType::BaseType;
};

class StringType
    : public BaseType<StringType, std::string, TypeId::String>
{
public:
    using BaseType::BaseType;

    void serialize(Buffer& buff) const {

        detail::writePrimitive<Id>(buff, static_cast<Id>(static_type_id));

        detail::writePrimitive<Id>(buff, value_.size());

        auto ptr = reinterpret_cast<const std::byte*>(value_.data());
        buff.insert(buff.end(), ptr, ptr + value_.size());
    }


    static StringType deserialize(Buffer::const_iterator& it,
        Buffer::const_iterator end)
    {
        uint64_t len = detail::readPrimitive<uint64_t>(it, end);
        std::string s;
        s.resize(len);
        for (uint64_t i = 0; i < len; ++i) {
            s[i] = static_cast<char>(*it++);
        }
        return StringType{ std::move(s) };
    }
};

class Any;

class VectorType {
public:
    static constexpr TypeId static_type_id = TypeId::Vector;

    template<typename... Args,
        typename = std::enable_if_t<
        std::conjunction_v<std::is_constructible<Any, Args>...>
        >
    >
    VectorType(Args&&... args);

    template<typename Arg,
        typename = std::enable_if_t<std::is_constructible<Any, Arg>::value>
    >
    void push_back(Arg&& val);

    void serialize(Buffer& buff) const;
    static VectorType deserialize(Buffer::const_iterator& it,
        Buffer::const_iterator end);

    bool operator==(VectorType const& other) const;

private:
    std::vector<Any> elements_;
};

class Any {
public:
    template<typename T,
        typename = std::enable_if_t<
        std::disjunction_v<
        std::is_same<std::decay_t<T>, IntegerType>,
        std::is_same<std::decay_t<T>, FloatType>,
        std::is_same<std::decay_t<T>, StringType>,
        std::is_same<std::decay_t<T>, VectorType>
        >
        >
    >
    Any(T&& v)
        : payload_(std::forward<T>(v))
    {
    }

    Any()
        : payload_(IntegerType{0xDEADDEADDEADDEAD}) //todo fix
    {

    }

    void serialize(Buffer& buff) const {
        std::visit([&](auto const& x) { x.serialize(buff); },
            payload_);
    }

    Buffer::const_iterator deserialize(Buffer::const_iterator it,
        Buffer::const_iterator end);

    TypeId getPayloadTypeId() const {
        return std::visit([](auto const& x) {
            return std::decay_t<decltype(x)>::static_type_id;
            }, payload_);
    }

    template<typename T>
    const T& getValue() const {
        return std::get<T>(payload_);
    }

    template<TypeId kId>
    const auto& getValue() const {
        if constexpr (kId == TypeId::Uint)    return getValue<IntegerType>();
        else if constexpr (kId == TypeId::Float)  return getValue<FloatType>();
        else if constexpr (kId == TypeId::String) return getValue<StringType>();
        else if constexpr (kId == TypeId::Vector) return getValue<VectorType>(); //todo fix
    }

    bool operator==(Any const& o) const {
        return payload_ == o.payload_;
    }

private:
    std::variant<IntegerType, FloatType, StringType, VectorType> payload_;
};


template<typename... Args, typename>
inline VectorType::VectorType(Args&&... args)
    : elements_{}
{
    (elements_.emplace_back(std::forward<Args>(args)), ...);
}

template<typename Arg, typename>
inline void VectorType::push_back(Arg&& val) {
    elements_.emplace_back(std::forward<Arg>(val));
}

inline void VectorType::serialize(Buffer& buff) const {
    detail::writePrimitive<Id>(buff, static_cast<Id>(TypeId::Vector));
    detail::writePrimitive<uint64_t>(buff, elements_.size());
    for (auto const& e : elements_) {
        e.serialize(buff);
    }
}

inline VectorType VectorType::deserialize(Buffer::const_iterator& it,
    Buffer::const_iterator end)
{
    uint64_t count = detail::readPrimitive<uint64_t>(it, end);
    VectorType result;
    for (uint64_t i = 0; i < count; ++i) {
        Any a{};
        it = a.deserialize(it, end);
        result.elements_.push_back(std::move(a));
    }
    return result;
}

inline bool VectorType::operator==(VectorType const& other) const {
    return elements_ == other.elements_;
}

class Serializator {
public:

    template<typename Arg,
        typename = std::enable_if_t<std::is_constructible<Any, Arg>::value>
    >
    void push(Arg&& val) {
        storage_.emplace_back(std::forward<Arg>(val));
    }


    Buffer serialize() const {
        Buffer buff;

        detail::writePrimitive<uint64_t>(buff, storage_.size());

        for (auto const& a : storage_) {
            a.serialize(buff);
        }
        return buff;
    }


    static std::vector<Any> deserialize(const Buffer& buff) {
        std::vector<Any> out;
        auto it = buff.cbegin();
        auto end = buff.cend();

        uint64_t count = detail::readPrimitive<uint64_t>(it, end);

        for (uint64_t i = 0; i < count; ++i) {
            Any a;         
            it = a.deserialize(it, end);
            out.emplace_back(std::move(a));
        }
        return out;
    }

    const std::vector<Any>& getStorage() const {
        return storage_;
    }

private:
    std::vector<Any> storage_;
};
