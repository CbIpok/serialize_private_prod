#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <cstddef>

using Id = uint64_t;
using Buffer = std::vector<std::byte>;

enum class TypeId : Id {
    Uint,
    Float,
    String,
    Vector
};

// IntegerType
class IntegerType {
public:
    IntegerType(uint64_t value);
    void serialize(Buffer& buff) const;
    static IntegerType deserialize(Buffer::const_iterator& begin, Buffer::const_iterator end);
private:
    uint64_t value_;
};

// FloatType
class FloatType {
public:
    FloatType(double value);
    void serialize(Buffer& buff) const;
    static FloatType deserialize(Buffer::const_iterator& begin, Buffer::const_iterator end);
private:
    double value_;
};

// StringType
class StringType {
public:
    StringType(const std::string& value);
    void serialize(Buffer& buff) const;
    static StringType deserialize(Buffer::const_iterator& begin, Buffer::const_iterator end);
private:
    std::string value_;
};

// Forward declaration
class Any;

// VectorType
class VectorType {
public:
    template<typename... Args>
    VectorType(Args&&... args) { /*TODO: implement*/ };

    template<typename Arg>
    void push_back(Arg&& val) { /*TODO: implement*/ };

    void serialize(Buffer& buff) const;
    static VectorType deserialize(Buffer::const_iterator& begin, Buffer::const_iterator end);

private:
    std::vector<Any> elements_;
};

// Any
class Any {
public:
    template<typename... Args>
    Any(Args&&... args) { /*TODO: implement*/ };

    void serialize(Buffer& buff) const;
    Buffer::const_iterator deserialize(Buffer::const_iterator begin, Buffer::const_iterator end);

    TypeId getPayloadTypeId() const;

    template<typename Type>
    const Type& getValue() const { return {}; /*TODO: implement*/ };

    template<TypeId kId>
    const auto& getValue() const { return {}; /*TODO: implement*/ };

    bool operator==(const Any& other) const;

private:
    TypeId typeId_;
    // storage for the actual value
    // using std::variant or union
};

// Serializator
class Serializator {
public:
    template<typename Arg>
    void push(Arg&& val) { /*TODO: implement*/};

    Buffer serialize() const;
    static std::vector<Any> deserialize(const Buffer& buff) { return {}; /*TODO: implement*/ };

    const std::vector<Any>& getStorage() const;

private:
    std::vector<Any> storage_;
};
