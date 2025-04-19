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

    // Записать T (uint64_t, double и т.п.) в буфер в little‑endian
    template<typename T>
    void writePrimitive(Buffer& buff, const T& val) {
        const auto raw = reinterpret_cast<const std::byte*>(&val);
        if constexpr (HOST_LITTLE_ENDIAN) {
            buff.insert(buff.end(), raw, raw + sizeof(T));
        }
        else {
            // переворачиваем байты при записи
            for (size_t i = 0; i < sizeof(T); ++i) {
                buff.push_back(raw[sizeof(T) - 1 - i]);
            }
        }
    }

    // Прочитать T из [it, end) как little‑endian, продвинуть итератор
    template<typename T>
    T readPrimitive(Buffer::const_iterator& it, Buffer::const_iterator /*end*/) {
        T v;
        std::byte temp[sizeof(T)];
        if constexpr (HOST_LITTLE_ENDIAN) {
            // просто копируем побайтово
            std::memcpy(&v, &*it, sizeof(T));
        }
        else {
            // переворачиваем порядок байт при чтении
            for (size_t i = 0; i < sizeof(T); ++i) {
                temp[sizeof(T) - 1 - i] = *(it + i);
            }
            std::memcpy(&v, temp, sizeof(T));
        }
        it += sizeof(T);
        return v;
    }

} // namespace detail

template<typename Derived, typename ValueT, TypeId KTypeId>
class BaseType {
public:
    explicit BaseType(ValueT v)
        : value_(std::move(v))
    {
    }

    // Сериализация: сначала numeric TypeId, потом ValueT
    void serialize(Buffer& buff) const {
        // пишем Id как uint64_t
        detail::writePrimitive<Id>(buff, static_cast<Id>(KTypeId));
        detail::writePrimitive<ValueT>(buff, value_);
    }

    // Десериализация: читаем numeric Id, затем ValueT
    static Derived deserialize(Buffer::const_iterator& it,
        Buffer::const_iterator end) {
        // читаем и (по желанию) проверяем
        auto raw = detail::readPrimitive<Id>(it, end);
        // читаем само значение
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

// IntegerType
class IntegerType
    : public BaseType<IntegerType, uint64_t, TypeId::Uint>
{
public:
    using BaseType::BaseType;  // конструктор берём «как есть»
    // static Derived deserialize(...) уже есть в BaseType
    // serialize() тоже в BaseType
};

// FloatType
class FloatType
    : public BaseType<FloatType, double, TypeId::Float>
{
public:
    using BaseType::BaseType;
};

// StringType
class StringType
    : public BaseType<StringType, std::string, TypeId::String>
{
public:
    using BaseType::BaseType;

    // Переопределяем сериализацию для строки
    void serialize(Buffer& buff) const {
        // 1) Пишем идентификатор TypeId::String
        detail::writePrimitive<Id>(buff, static_cast<Id>(static_type_id));
        // 2) Пишем длину строки
        detail::writePrimitive<Id>(buff, value_.size());
        // 3) Пишем сами байты (UTF-8 без '\0')
        auto ptr = reinterpret_cast<const std::byte*>(value_.data());
        buff.insert(buff.end(), ptr, ptr + value_.size());
    }

    // Переопределяем десериализацию
    static StringType deserialize(Buffer::const_iterator& it,
        Buffer::const_iterator end)
    {
        // (TypeId уже прочитан в Any)
        // 1) Читаем длину
        uint64_t len = detail::readPrimitive<uint64_t>(it, end);
        // 2) Читаем данные в std::string
        std::string s;
        s.resize(len);
        for (uint64_t i = 0; i < len; ++i) {
            s[i] = static_cast<char>(*it++);
        }
        return StringType{ std::move(s) };
    }
};

// Forward declaration
class Any;

// VectorType
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
    // Конструктор из любого из XType
    template<typename T,
        typename = std::enable_if_t<
        std::disjunction_v<
        std::is_same<std::decay_t<T>, IntegerType>,
        std::is_same<std::decay_t<T>, FloatType>,
        std::is_same<std::decay_t<T>, StringType>,
        std::is_same<std::decay_t<T>, VectorType>,
        std::is_same<std::decay_t<T>, Any>
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

    // Сериализация: просто делегируем в payload_
    void serialize(Buffer& buff) const {
        std::visit([&](auto const& x) { x.serialize(buff); },
            payload_);
    }

    // Десериализация по TypeId
    Buffer::const_iterator deserialize(Buffer::const_iterator it,
        Buffer::const_iterator end);

    // Получить сохранённый TypeId
    TypeId getPayloadTypeId() const {
        return std::visit([](auto const& x) {
            return std::decay_t<decltype(x)>::static_type_id;
            }, payload_);
    }

    // Доступ к значению по типу
    template<typename T>
    const T& getValue() const {
        return std::get<T>(payload_);
    }

    // Доступ к значению по TypeId
    template<TypeId kId>
    const auto& getValue() const {
        if constexpr (kId == TypeId::Uint)    return getValue<IntegerType>();
        else if constexpr (kId == TypeId::Float)  return getValue<FloatType>();
        else if constexpr (kId == TypeId::String) return getValue<StringType>();
        else /* kId == TypeId::Vector */         return getValue<VectorType>();
    }

    bool operator==(Any const& o) const {
        return payload_ == o.payload_;
    }

private:
    std::variant<IntegerType, FloatType, StringType, VectorType> payload_;
};


//–– Реализации VectorType ––//

// Конструктор из набора аргументов
template<typename... Args, typename>
inline VectorType::VectorType(Args&&... args)
    : elements_{}
{
    (elements_.emplace_back(std::forward<Args>(args)), ...);
}

// push_back
template<typename Arg, typename>
inline void VectorType::push_back(Arg&& val) {
    elements_.emplace_back(std::forward<Arg>(val));
}

// serialize
inline void VectorType::serialize(Buffer& buff) const {
    detail::writePrimitive<Id>(buff, static_cast<Id>(TypeId::Vector));
    detail::writePrimitive<uint64_t>(buff, elements_.size());
    for (auto const& e : elements_) {
        e.serialize(buff);
    }
}

// deserialize
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

// operator==
inline bool VectorType::operator==(VectorType const& other) const {
    return elements_ == other.elements_;
}

// Serializator
class Serializator {
public:
    // Принимаем любой Arg, из которого можно построить Any
    template<typename Arg,
        typename = std::enable_if_t<std::is_constructible<Any, Arg>::value>
    >
    void push(Arg&& val) {
        storage_.emplace_back(std::forward<Arg>(val));
    }

    // Сериализуем: сначала число элементов (uint64_t LE), потом сами Any
    Buffer serialize() const {
        Buffer buff;
        // 1) Пишем количество элементов
        detail::writePrimitive<uint64_t>(buff, storage_.size());
        // 2) Пишем каждый элемент
        for (auto const& a : storage_) {
            a.serialize(buff);
        }
        return buff;
    }

    // Десериализуем: сначала count, затем count раз Any::deserialize
    static std::vector<Any> deserialize(const Buffer& buff) {
        std::vector<Any> out;
        auto it = buff.cbegin();
        auto end = buff.cend();
        // 1) Считываем количество элементов
        uint64_t count = detail::readPrimitive<uint64_t>(it, end);
        // 2) Для каждого элемента вызываем Any::deserialize
        for (uint64_t i = 0; i < count; ++i) {
            Any a{};           // пустой Any
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
