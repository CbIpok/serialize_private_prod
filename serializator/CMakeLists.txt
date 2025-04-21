set(CMAKE_CXX_STANDARD 17)

add_library(Serializer STATIC serialize.cpp)

target_include_directories(
        Serializer ${warning_guard}
        PUBLIC
        "\$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/serializator>"
)