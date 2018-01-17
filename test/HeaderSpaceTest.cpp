#include "gtest/gtest.h"

#include "../src/header_space/HeaderSpace.hpp"

class HeaderSpaceTest : public ::testing::Test
{
protected:
    using H = HeaderSpace;

    virtual HeaderSpace empty() const {
        return HeaderSpace::emptySpace(header_length_);
    }
    virtual HeaderSpace zeros() const {
        return HeaderSpace("00000000");
    }
    virtual HeaderSpace ones() const {
        return HeaderSpace("11111111");
    }
    virtual HeaderSpace whole() const {
        return HeaderSpace::wholeSpace(header_length_);
    }

    const int header_length_ = 1;
};

TEST_F(HeaderSpaceTest, CreationTest)
{
    EXPECT_EQ(header_length_, zeros().length());

    EXPECT_TRUE(empty().empty());
    EXPECT_FALSE(zeros().empty());
    EXPECT_FALSE(ones().empty());
    EXPECT_FALSE(whole().empty());

    EXPECT_TRUE(H(empty()).empty());
    EXPECT_TRUE(H(std::move(empty())).empty());
    EXPECT_FALSE(H(zeros()).empty());
    EXPECT_FALSE(H(std::move(zeros())).empty());
    EXPECT_FALSE(H(whole()).empty());
    EXPECT_FALSE(H(std::move(whole())).empty());

    EXPECT_TRUE((zeros() = empty()).empty());
    EXPECT_TRUE((zeros() = std::move(empty())).empty());
    EXPECT_FALSE((empty() = whole()).empty());
    EXPECT_FALSE((empty() = std::move(whole())).empty());
}

TEST_F(HeaderSpaceTest, EqualityTest)
{
    EXPECT_EQ(H("xxxxxxxx"), whole());

    EXPECT_EQ(empty(), empty());
    EXPECT_EQ(zeros(), zeros());
    EXPECT_EQ(ones() , ones() );
    EXPECT_EQ(whole(), whole());
    EXPECT_EQ(H("xx0011xx"), H("xx0011xx"));

    EXPECT_NE(empty(), whole());
    EXPECT_NE(H("00000001"), empty());
    EXPECT_NE(H("00000001"), zeros());
    EXPECT_NE(H("00000001"), ones());
    EXPECT_NE(H("00000001"), whole());

    EXPECT_EQ(empty(), H(empty()));
    EXPECT_EQ(zeros(), H(zeros()));
    EXPECT_EQ(ones(), H(ones()));
    EXPECT_EQ(whole(), H(whole()));
    EXPECT_EQ(empty(), H(std::move(empty())));
    EXPECT_EQ(zeros(), H(std::move(zeros())));
    EXPECT_EQ(ones(), H(std::move(ones())));
    EXPECT_EQ(whole(), H(std::move(whole())));

    EXPECT_EQ(empty(), whole() = empty());
    EXPECT_EQ(zeros(), whole() = zeros());
    EXPECT_EQ(ones(), whole() = ones());
    EXPECT_EQ(whole(), empty() = whole());
    EXPECT_EQ(empty(), whole() = std::move(empty()));
    EXPECT_EQ(zeros(), whole() = std::move(zeros()));
    EXPECT_EQ(ones(), whole() = std::move(ones()));
    EXPECT_EQ(whole(), empty() = std::move(whole()));
}
#include <iostream>
TEST_F(HeaderSpaceTest, UnionTest)
{
    EXPECT_EQ(empty(), empty() + empty());
    EXPECT_EQ(zeros(), zeros() + empty());
    EXPECT_EQ(ones() , ones()  + empty());
    EXPECT_EQ(whole(), whole() + empty());
}

TEST_F(HeaderSpaceTest, IntersectionTest)
{
    EXPECT_EQ(empty(), empty() & empty());
    EXPECT_EQ(empty(), empty() &= empty());
}

TEST_F(HeaderSpaceTest, MinusTest)
{
    EXPECT_EQ(empty(), empty() - empty());
    EXPECT_EQ(empty(), empty() - zeros());
    EXPECT_EQ(empty(), empty() - ones() );
    EXPECT_EQ(empty(), empty() - whole());
    EXPECT_EQ(empty(), zeros() - zeros());
    EXPECT_EQ(empty(), zeros() - whole());
    EXPECT_EQ(empty(), ones()  - ones() );
    EXPECT_EQ(empty(), ones()  - whole());
    EXPECT_EQ(empty(), whole() - whole());

    EXPECT_EQ(zeros(), zeros() - empty());
    EXPECT_EQ(zeros(), zeros() - ones() );
    EXPECT_EQ(ones() , ones()  - empty());
    EXPECT_EQ(whole(), whole() - empty());

    EXPECT_EQ(empty(), H("xx0011xx") - whole());
    EXPECT_EQ(empty(), H("xx0011xx") - H("xx0011xx"));
    EXPECT_EQ(empty(), H("11110000") - H("1111000x"));
    EXPECT_EQ(empty(), H("11110000") - H("1111xxxx"));
    EXPECT_EQ(empty(), H("1111000x") - H("11110000") - H("11110001"));
    EXPECT_EQ(H("11110000"), H("1111000x") - H("11110001"));

    EXPECT_NE(empty(), whole() - zeros());
    EXPECT_NE(empty(), zeros() - ones());
    EXPECT_NE(empty(), H("0000000x") - H("xxxxxxx1"));

    EXPECT_EQ(zeros(), H("0000000x") - H("xxxxxxx1"));
    EXPECT_EQ(zeros(), H("000000xx") - H("xxxxxxx1") - H("xxxxxx1x"));
    EXPECT_EQ(H("0000000x") - H("xxxxxxx1"), H("x0000000") - H("10000000"));
}
