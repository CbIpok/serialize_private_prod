#include "serialize.hpp"

// IntegerType
IntegerType::IntegerType(uint64_t value)
    : value_(value) {
}

void IntegerType::serialize(Buffer& buff) const {
    // TODO: implement
}

IntegerType IntegerType::deserialize(Buffer::const_iterator& begin, Buffer::const_iterator end) {
    // TODO: implement
    return IntegerType(0);
}

// FloatType
FloatType::FloatType(double value)
    : value_(value) {
}

void FloatType::serialize(Buffer& buff) const {
    // TODO: implement
}

FloatType FloatType::deserialize(Buffer::const_iterator& begin, Buffer::const_iterator end) {
    // TODO: implement
    return FloatType(0.0);
}

// StringType
StringType::StringType(const std::string& value)
    : value_(value) {
}

void StringType::serialize(Buffer& buff) const {
    // TODO: implement
}

StringType StringType::deserialize(Buffer::const_iterator& begin, Buffer::const_iterator end) {
    // TODO: implement
    return StringType(std::string());
}

// VectorType (templates defined inline in header)

void VectorType::serialize(Buffer& buff) const {
    // TODO: implement
}

VectorType VectorType::deserialize(Buffer::const_iterator& begin, Buffer::const_iterator end) {
    // TODO: implement
    return VectorType();
}

// Any

// constructors and template methods inline or specialized in header

void Any::serialize(Buffer& buff) const {
    // TODO: implement
}

Buffer::const_iterator Any::deserialize(Buffer::const_iterator begin, Buffer::const_iterator end) {
    // TODO: implement
    return begin;
}

TypeId Any::getPayloadTypeId() const {
    return typeId_;
}

bool Any::operator==(const Any& other) const {
    // TODO: implement comparison
    return typeId_ == other.typeId_;
}

Buffer Serializator::serialize() const {
    // TODO: implement
    return Buffer();
}

const std::vector<Any>& Serializator::getStorage() const {
    return storage_;
}
