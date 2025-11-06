# Unit tests with GoogleTest

# Legacy placeholder test
add_executable(placeholder_test placeholder_test.cpp)
target_link_libraries(placeholder_test GTest::gtest GTest::gtest_main)
add_test(NAME PlaceholderTest COMMAND placeholder_test)

# Unit tests subdirectory
add_subdirectory(unit)
