//test_main.cpp
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
    EXPECT_TRUE(size, static_cast<std::streamsize>(buff.size()));

    return Serializator::deserialize(buff);
}

// Helper: parse output.json into a vector<Any>
static std::vector<Any> loadJsonAny(const std::string& path) {
    std::ifstream in(path);
    EXPECT_TRUE(in.is_open()) << "Cannot open " << path;

    json j;
    in >> j;
    EXPECT_TRUE(j.is_array()) << "Expected top-level JSON array";

    std::vector<Any> out;
    out.reserve(j.size());
    std::function<Any(const json&)> toAny = [&](const json& elem) -> Any {
        if (elem.is_number_unsigned()) {
            return Any{ IntegerType{ elem.get<uint64_t>() } };
        }
        if (elem.is_number_integer()) {
            return Any{ IntegerType{ static_cast<uint64_t>(elem.get<int64_t>()) } };
        }
        if (elem.is_number_float()) {
            return Any{ FloatType{ elem.get<double>() } };
        }
        if (elem.is_string()) {
            return Any{ StringType{ elem.get<std::string>() } };
        }
        if (elem.is_array()) {
            VectorType vec;
            for (auto const& e : elem) {
                vec.push_back(toAny(e));
            }
            return Any{ std::move(vec) };
        }
        ADD_FAILURE() << "Unsupported JSON element type: " << elem.type_name();
        return Any{ IntegerType{0} };
        };

    for (auto const& el : j) {
        out.push_back(toAny(el));
    }
    return out;
}

TEST(Serializator, RawBinMatchesOutputJson) {
    auto rawVec = loadRawAny("raw.bin");
    auto jsonVec = loadJsonAny("output.json");

    // 1) First ensure they have the same length
    ASSERT_EQ(rawVec.size(), jsonVec.size())
        << "raw.bin and output.json have different element counts";

    // 2) Then compare each element individually
    for (size_t i = 0; i < rawVec.size(); ++i) {
        EXPECT_EQ(rawVec[i], jsonVec[i])
            << "Mismatch at index " << i;
    }
}
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
