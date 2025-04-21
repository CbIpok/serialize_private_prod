#pragma once

#include "datatypes.h"


class Serializator final {
public:
    template<typename Arg, typename = std::enable_if_t<std::is_constructible<Any, Arg>::value>>
    void push(Arg&& val) {
        storage_.emplace_back(std::forward<Arg>(val));
    }

    [[nodiscard]]
    Buffer serialize() const {
        Buffer buff;
        detail::writePrimitive<uint64_t>(buff, storage_.size());
        for (auto const& a : storage_) {
            a.serialize(buff);
        }
        return buff;
    }

    [[nodiscard]]
    const std::vector<Any>& getStorage() const {
        return storage_;
    }

public:
    static std::vector<Any> deserialize(const Buffer& buff) {
        std::vector<Any> out;
        auto it = buff.cbegin();
        auto end = buff.cend();

        auto count = detail::readPrimitive<uint64_t>(it, end);
        for (uint64_t i = 0; i < count; ++i) {
            Any a = Any::deserialize(it, end);
            out.emplace_back(std::move(a));
        }
        return out;
    }
private:
    std::vector<Any> storage_;
};