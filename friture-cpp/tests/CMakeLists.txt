# Unit tests with GoogleTest
# (To be populated with actual tests later)

add_executable(placeholder_test placeholder_test.cpp)
target_link_libraries(placeholder_test GTest::gtest GTest::gtest_main)

add_test(NAME PlaceholderTest COMMAND placeholder_test)
