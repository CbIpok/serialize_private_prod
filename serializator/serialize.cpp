#include <exception>
#include "serialize.hpp"


Any Any::deserialize(Buffer::const_iterator& it, Buffer::const_iterator end) {
    Id rawId = detail::readPrimitive<Id>(it, end);
    switch (static_cast<TypeId>(rawId)) {
        case TypeId::Uint: {
            return Any(IntegerType::deserialize(it, end));
        }
        case TypeId::Float: {
            return Any(FloatType::deserialize(it, end));
        }
        case TypeId::String: {
            return Any(StringType::deserialize(it, end));
        }
        case TypeId::Vector: {
            return Any(VectorType::deserialize(it, end));
        }
        default:
            std::terminate();
    }
}

