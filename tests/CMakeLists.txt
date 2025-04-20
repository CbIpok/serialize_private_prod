project(MyTests LANGUAGES CXX)

# Где искать заголовки вашей библиотеки
# (тут уже автоматически добавится include из mylib через PUBLIC)
# Но если нужно явно:
# include_directories(${CMAKE_SOURCE_DIR}/lib/include)
set(CMAKE_CXX_STANDARD 17)
# If not using integrate install, uncomment and set your path:
# set(CMAKE_TOOLCHAIN_FILE "/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")

# Grab the GTest targets
find_package(GTest CONFIG REQUIRED)

# Собираем исполняемый файл с тестами
add_executable(test_foo
    test_main.cpp
)

# Линкуем с вашей библиотекой
target_link_libraries(test_foo
    PRIVATE
      Serializer
      GTest::gtest_main
)

include_directories(${CMAKE_SOURCE_DIR}/serializator)
target_include_directories(test_foo
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR} 
)


configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/output.json
  ${CMAKE_CURRENT_BINARY_DIR}/output.json
  COPYONLY
)