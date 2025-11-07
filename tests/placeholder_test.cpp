#include <gtest/gtest.h>

TEST(PlaceholderTest, BasicAssertion) {
    EXPECT_EQ(1 + 1, 2);
    EXPECT_TRUE(true);
}

TEST(PlaceholderTest, StringTest) {
    std::string str = "Friture C++";
    EXPECT_EQ(str.length(), 11);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
