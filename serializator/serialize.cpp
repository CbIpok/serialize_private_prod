//serialize.cpp
#include "serialize.hpp"


Buffer::const_iterator Any::deserialize(Buffer::const_iterator it,
    Buffer::const_iterator end) {
    // Сначала читаем Id типа
    Id rawId = detail::readPrimitive<Id>(it, end);
    auto tid = static_cast<TypeId>(rawId);

    // В зависимости от tid десериализуем нужный XType и сохраняем в payload_
    switch (tid) {
    case TypeId::Uint: {
        auto v = IntegerType::deserialize(it, end);
        payload_ = std::move(v);
        break;
    }
    case TypeId::Float: {
        auto v = FloatType::deserialize(it, end);
        payload_ = std::move(v);
        break;
    }
    case TypeId::String: {
        auto v = StringType::deserialize(it, end);
        payload_ = std::move(v);
        break;
    }
    case TypeId::Vector: {
        auto v = VectorType::deserialize(it, end);
        payload_ = std::move(v);
        break;
    }
    }
    // Возвращаем итератор, указывающий на следующий неподсчитанный байт
    return it;
}

