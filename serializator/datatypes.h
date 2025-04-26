#pragma once

#include "bufferstream.h"


enum class TypeId : Id {
    Uint,
    Float,
    String,
    Vector
};

template<typename Derived, typename ValueT, TypeId KTypeId>
class BaseType {
public:
    static constexpr TypeId static_type_id = KTypeId;

public:
    BaseType() = default;
    BaseType(ValueT v): value_(std::move(v)) {}

    [[nodiscard]]
    static Derived deserialize(Buffer::const_iterator& it, Buffer::const_iterator end) {
        auto&& val = detail::readPrimitive<ValueT>(it, end);
        return Derived{ std::move(val) };
    }


    void serialize(Buffer& buff) const { serialize_impl(buff); }

    bool operator==(Derived const& other) const {
        return value_ == other.value_;
    }

private:
    void serialize_impl(Buffer& buff) const {
        detail::writePrimitive<Id>(buff, static_cast<Id>(KTypeId));
        detail::writePrimitive<ValueT>(buff, value_);
    }

protected:
    ValueT value_;
};


class IntegerType : public  BaseType<IntegerType, std::uint64_t, TypeId::Uint> {};


class FloatType : public BaseType<FloatType, double, TypeId::Float> {};


class StringType final : public BaseType<StringType, std::string, TypeId::String> {
public:
    using BaseType::BaseType;

    void serialize(Buffer& buff) const {
        detail::writePrimitive<Id>(buff, static_cast<Id>(static_type_id));
        detail::writePrimitive<Id>(buff, value_.size());
        auto ptr = reinterpret_cast<const std::byte*>(value_.data());
        buff.insert(buff.end(), ptr, ptr + value_.size());
    }

    static StringType deserialize(Buffer::const_iterator& it, Buffer::const_iterator end) {
        auto len = detail::readPrimitive<uint64_t>(it, end);
        std::string s;
        s.resize(len);
        for (uint64_t i = 0; i < len; ++i) {
            s[i] = static_cast<char>(*it++);
        }
        return StringType{ std::move(s) };
    }
};

class Any;

class VectorType final : public BaseType<VectorType, std::vector<Any>, TypeId::Vector>  {
public:
    template<typename... Args, typename = std::enable_if_t<std::conjunction_v<std::is_constructible<Any, Args>...>>>
    explicit VectorType(Args&&... args);

    template<typename Arg, typename = std::enable_if_t<std::is_constructible<Any, Arg>::value>>
    void push_back(Arg&& val);

    void serialize(Buffer& buff) const;
    static VectorType deserialize(Buffer::const_iterator& it,
                                  Buffer::const_iterator end);
};

class Any final {
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
    explicit Any(T&& v) : payload_(std::forward<T>(v)) {}
    Any() = default;

public:
    void serialize(Buffer& buff) const {
        std::visit([&](auto const& x) { x.serialize(buff); },
                   payload_);
    }

    static Any deserialize(Buffer::const_iterator& it,
                                       Buffer::const_iterator end);

    [[nodiscard]]
    TypeId getPayloadTypeId() const {
        auto visitor = [](auto const& x) {
            return std::decay_t<decltype(x)>::static_type_id;
        };

        return std::visit(std::move(visitor), payload_);
    }

    template<typename T>
    [[nodiscard]]
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
    std::variant<IntegerType, FloatType, StringType, VectorType> payload_{};
};


template<typename... Args, typename>
inline VectorType::VectorType(Args &&... args) : BaseType<VectorType, std::vector<Any>, TypeId::Vector>() {
    (value_.emplace_back(std::forward<Args>(args)), ...);
}

template<typename Arg, typename>
inline void VectorType::push_back(Arg&& val) {
    value_.emplace_back(std::forward<Arg>(val));
}

inline void VectorType::serialize(Buffer& buff) const {
    detail::writePrimitive<Id>(buff, static_cast<Id>(TypeId::Vector));
    detail::writePrimitive<uint64_t>(buff, value_.size());
    for (auto const& e : value_) {
        e.serialize(buff);
    }
}

inline VectorType VectorType::deserialize(Buffer::const_iterator& it, Buffer::const_iterator end) {
    auto count = detail::readPrimitive<uint64_t>(it, end);
    VectorType result;
    for (uint64_t i = 0; i < count; ++i) {
        Any a = Any::deserialize(it, end);
        result.push_back(std::move(a));
    }
    return result;
}