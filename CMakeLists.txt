cmake_minimum_required(VERSION 3.8)
project(rdbc)

find_package(GTest)

include_directories(include)
add_executable(rdbc_test rdbc_test.cpp)
add_executable(rdbc_test_internal rdbc_test_internal.cpp)
add_executable(rdbc_test_external rdbc_test_external.cpp)


include(GoogleTest)
target_link_libraries(rdbc_test GTest::gtest_main)
target_link_libraries(rdbc_test_internal GTest::gtest_main)
target_link_libraries(rdbc_test_external GTest::gtest_main)

target_compile_options(rdbc_test PRIVATE -DRDBC_TESTING)
target_compile_options(rdbc_test_internal PRIVATE -DRDBC_TESTING -DRDBC_TESTING_INTERNAL)
target_compile_options(rdbc_test_external PRIVATE)

gtest_discover_tests(rdbc_test)

install(
    FILES rdbc.hpp
    DESTINATION ${CMAKE_INSTALL_PREFIX}/include
)
