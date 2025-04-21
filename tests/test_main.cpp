
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include "serialize.hpp"
#include <fstream>
#include <functional>

using json = nlohmann::json;
using Buffer = std::vector<std::byte>;


// Helper: deserialize raw.bin into a vector<Any>
static std::vector<Any> loadRawAny(const std::string& path) {
    std::ifstream raw(path, std::ios::binary);
    EXPECT_TRUE(raw.is_open()) << "Cannot open " << path;

    raw.seekg(0, std::ios::end);
    std::streamsize size = raw.tellg();
    raw.seekg(0, std::ios::beg);

    Buffer buff(static_cast<size_t>(size));
    raw.read(reinterpret_cast<char*>(buff.data()), size);

    return Serializator::deserialize(buff);
}

static std::string hex_to_bytes(const std::string& hex) {
    std::string out;
    out.reserve(hex.size() / 2);
    for (size_t i = 0; i < hex.size(); i += 2) {
        uint8_t byte = std::stoul(hex.substr(i, 2), nullptr, 16);
        out.push_back(static_cast<char>(byte));
    }
    return out;
}

// Helper: parse output.json into a vector<Any>
static std::vector<Any> loadJsonAny(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    EXPECT_TRUE(in.is_open()) << "Cannot open " << path;

    json j;
    in >> j;
    EXPECT_TRUE(j.is_array()) << "Expected top-level JSON array";

    std::vector<Any> out;
    out.reserve(j.size());
    std::function<Any(const json &)> toAny = [&](const json &elem) -> Any {
        if (elem.is_number_unsigned()) {
            return Any{IntegerType{elem.get<uint64_t>()}};
        }
        if (elem.is_number_integer()) {
            return Any{IntegerType{static_cast<uint64_t>(elem.get<int64_t>())}};
        }
        if (elem.is_number_float()) {
            return Any{FloatType{elem.get<double>()}};
        }
        if (elem.is_string()) {
            // hex‑encoded raw blob → decode back to bytes
            auto hex = elem.get<std::string>();
            auto raw = hex_to_bytes(hex);
            return Any{StringType{std::move(raw)}};
        }
        if (elem.is_array()) {
            VectorType vec;
            for (auto const &e: elem) {
                vec.push_back(toAny(e));
            }
            return Any{std::move(vec)};
        }
        ADD_FAILURE() << "Unsupported JSON element type: " << elem.type_name();
        return Any{IntegerType{0}};
    };

    for (auto const& el : j) {
        out.push_back(toAny(el));
    }
    return out;
}

TEST(Serializator, RawBinMatchesOutputJson) {
    auto rawVec = loadRawAny("raw.bin");
    auto jsonVec = loadJsonAny("raw.json");

    // 1) First ensure they have the same length
    ASSERT_EQ(rawVec.size(), jsonVec.size())
        << "raw.bin and output.json have different element counts";

    // 2) Then compare each element individually
    for (size_t i = 0; i < rawVec.size(); ++i) {
        EXPECT_EQ(rawVec[i], jsonVec[i])
            << "Mismatch at index " << i;
    }
}

TEST(Serializator, EmptyTest) {
    Serializator s;
    auto buff = s.serialize();
    auto res = Serializator::deserialize(buff);
    EXPECT_EQ(res.size(), 0);
}

TEST(Serializator, SingleElementTest) {
    Serializator s;
    s.push(IntegerType{42});
    auto buff = s.serialize();
    auto res = Serializator::deserialize(buff);
    EXPECT_EQ(res.size(), 1);
    EXPECT_EQ(res[0].getValue<IntegerType>(), IntegerType{42});
}

TEST(Serializator, TestFromDescription) {
    std::initializer_list<unsigned char> raw = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                0x71, 0x77, 0x65, 0x72, 0x74, 0x79, 0x00, 0x00,
                                                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x94, 0x88,
                                                0x01, 0x00, 0x00, 0x00, 0x00, 0x00};

    std::vector<std::byte> input_buff;
    input_buff.reserve(raw.size());
    for (auto &&i: raw) {
        input_buff.push_back(static_cast<std::byte>(i));
    }
    auto buff = Serializator::deserialize(input_buff);
    EXPECT_EQ(buff.size(), 1);
    VectorType vec{
            StringType{"qwerty"},
            IntegerType{100500}
    };

    EXPECT_EQ(buff[0].getValue<VectorType>(), vec);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
