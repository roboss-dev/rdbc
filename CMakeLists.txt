cmake_minimum_required(VERSION 3.8)
project(rdbc)

find_package(GTest)

include_directories(include)
add_executable(rdbc_test rdbc_test.cpp)
add_executable(rdbc_test_nocheck rdbc_test_nocheck.cpp)


include(GoogleTest)
target_link_libraries(rdbc_test GTest::gtest_main)
target_link_libraries(rdbc_test_nocheck GTest::gtest_main)

target_compile_options(rdbc_test PRIVATE -DRDBC_TESTING)
target_compile_options(rdbc_test_nocheck PRIVATE -DRDBC_TESTING -DRDBC_TESTING_INTERNAL)

gtest_discover_tests(rdbc_test)