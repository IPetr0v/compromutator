#include "gtest/gtest.h"

#include "../../src/header_space/HeaderSpace.hpp"
#include "../../src/NetworkSpace.hpp"

class HeaderSpaceTest : public ::testing::Test
{
protected:
    using H = HeaderSpace;
    using T = HeaderChanger;
    using N = NetworkSpace;

    virtual HeaderSpace empty() const {
        return H::emptySpace(header_length_);
    }
    virtual HeaderSpace zeros() const {
        return H("00000000");
    }
    virtual HeaderSpace ones() const {
        return H("11111111");
    }
    virtual HeaderSpace whole() const {
        return H::wholeSpace(header_length_);
    }
    virtual HeaderChanger identity() const {
        return T::identityHeaderChanger(header_length_);
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

    auto empty_header = empty();
    EXPECT_TRUE(H(empty_header).empty());
    EXPECT_TRUE(H(std::move(empty_header)).empty());

    auto zero_header = zeros();
    EXPECT_FALSE(H(zero_header).empty());
    EXPECT_FALSE(H(std::move(zero_header)).empty());

    auto whole_header = whole();
    EXPECT_FALSE(H(whole_header).empty());
    EXPECT_FALSE(H(std::move(whole_header)).empty());

    empty_header = empty();
    EXPECT_TRUE((zeros() = empty_header).empty());
    EXPECT_TRUE((zeros() = std::move(empty_header)).empty());

    whole_header = whole();
    EXPECT_FALSE((empty() = whole_header).empty());
    EXPECT_FALSE((empty() = std::move(whole_header)).empty());
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

    auto empty_header = empty();
    auto zero_header = zeros();
    auto one_header = ones();
    auto whole_header = whole();
    EXPECT_EQ(empty(), whole() = empty_header);
    EXPECT_EQ(zeros(), whole() = std::move(zero_header));
    EXPECT_EQ(zeros(), whole() = zeros());
    EXPECT_EQ(ones(), whole() = ones());
    EXPECT_EQ(whole(), empty() = whole());
    EXPECT_EQ(ones(), whole() = std::move(one_header));
    EXPECT_EQ(whole(), empty() = std::move(whole_header));
}

TEST_F(HeaderSpaceTest, UnionTest)
{
    EXPECT_EQ(empty(), empty() + empty());
    EXPECT_EQ(zeros(), zeros() + empty());
    EXPECT_EQ(ones() , ones()  + empty());
    EXPECT_EQ(whole(), whole() + empty());

    EXPECT_EQ(whole(), H("xxxxxxx0") + H("xxxxxxx1"));
    EXPECT_EQ(H("0000000x"), zeros() + H("00000001"));
    EXPECT_EQ(H("xx0011xx"), H("xx001100") + H("xx001101") +
                             H("xx001110") + H("xx001111"));
}

TEST_F(HeaderSpaceTest, IntersectionTest)
{
    EXPECT_EQ(empty(), empty() &  empty());
    EXPECT_EQ(empty(), empty() &= empty());
    EXPECT_EQ(empty(), zeros() &  ones() );
    EXPECT_EQ(empty(), zeros() &= ones() );
    EXPECT_EQ(zeros(), zeros() &  zeros());
    EXPECT_EQ(zeros(), zeros() &= zeros());
    EXPECT_EQ(zeros(), zeros() &  whole());
    EXPECT_EQ(zeros(), zeros() &= whole());
    EXPECT_EQ(ones() , ones()  &  whole());
    EXPECT_EQ(ones() , ones()  &= whole());
    EXPECT_EQ(whole(), whole() &  whole());
    EXPECT_EQ(whole(), whole() &= whole());

    EXPECT_EQ(zeros(), H("0000xxxx") & H("xxxx0000"));
    EXPECT_EQ(zeros(), H("0000xxxx") & H("xxxxxx00") & H("xxxx00xx"));
    EXPECT_EQ(H("xx0011xx"), whole() & H("xx00xxxx") & H("xxxx11xx"));
    EXPECT_EQ(empty(), H("0000xxxx") & H("1111xxxx"));
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

TEST_F(HeaderSpaceTest, ComplementTest)
{
    EXPECT_EQ(empty(), ~whole());
    EXPECT_EQ(whole(), ~empty());
    EXPECT_EQ(H("xxxxxxx1"), ~H("xxxxxxx0"));
    EXPECT_EQ(H("xxxxxxx1") + H("xxxxxx1x"),
              ~H("xxxxxxx0") + ~H("xxxxxx0x"));
    EXPECT_EQ(H("xxxxxxx1") + H("xxxxxx1x"),
              ~(H("xxxxxxx0") & H("xxxxxx0x")));
    EXPECT_EQ(H("xxxxxx11"), ~(H("xxxxxxx0") + H("xxxxxx0x")));
    EXPECT_EQ(H("xxxxxxx0"), ~(whole() - H("xxxxxxx0")));
}

TEST_F(HeaderSpaceTest, ChangerCreationTest)
{
    auto identity_header = identity();
    EXPECT_EQ(T("xxxxxxxx"), identity_header);
    EXPECT_EQ(identity(), T(identity_header));
    EXPECT_EQ(identity(), T(std::move(identity_header)));
    EXPECT_EQ(T("xx0011xx"), T("xx0011xx"));
}

TEST_F(HeaderSpaceTest, ChangerOperationTest)
{
    EXPECT_EQ(identity(), identity() *= identity());
    EXPECT_EQ(T("xx0011xx"), T("xx0011xx") *= identity());

    EXPECT_EQ(zeros(), T("xxxx0000").apply(H("000011xx")));
    EXPECT_EQ(ones(), T("xx1111xx").apply(H("1100xx11")));

    auto transfer = T("xx00xxxx") *= T("00xxxxxx") *= T("xxxx0000");
    EXPECT_EQ(zeros(), transfer.apply(H("11111111")));
}

TEST_F(HeaderSpaceTest, NetworkSpaceTest)
{
    EXPECT_EQ(N(1, whole()), N(1, whole()));
    EXPECT_NE(N(1, whole()), N(2, whole()));
    EXPECT_NE(N(1, whole()), N(1, zeros()));
    EXPECT_NE(N(1, whole()), N(1, empty()));

    EXPECT_EQ(N(1, zeros()), N(whole()) & N(1, zeros()));
    EXPECT_EQ(N(1, zeros()), N(1, whole()) & N(1, zeros()));
    EXPECT_EQ(N::emptySpace(), N(1, whole()) & N(2, whole()));
    EXPECT_EQ(N::emptySpace(), N(whole()) & N(1, empty()));
    EXPECT_EQ(N::emptySpace(), N(zeros()) & N(1, ones()));
}

TEST_F(HeaderSpaceTest, BitVectorTest)
{
    auto bit_vector = BitMask::wholeSpace(header_length_);
    for (int i = 0; i < header_length_; i++) {
        EXPECT_EQ(BitValue::ANY, bit_vector.getBit(i));
    }
    bit_vector.setBit(0, BitValue::ZERO);
    bit_vector.setBit(1, BitValue::ZERO);
    bit_vector.setBit(2, BitValue::ONE);
    bit_vector.setBit(3, BitValue::ONE);
    EXPECT_EQ(H("0011xxxx"), H(std::move(bit_vector)));

    auto diff_header = whole() - H("00xxxxxx");
    diff_header.computeDifference();
    auto bit_space = diff_header.getBitSpace();
    ASSERT_EQ(2u, bit_space.size());
    auto bit_vector0 = bit_space[0].mask;
    auto bit_vector1 = bit_space[1].mask;
    auto header0 = H(bit_vector0);
    auto header1 = H(bit_vector1);
    if (H("1xxxxxxx") == header0) {
        EXPECT_EQ(H("x1xxxxxx"), header1);
    }
    else {
        EXPECT_EQ(H("1xxxxxxx"), header1);
        EXPECT_EQ(H("x1xxxxxx"), header0);
    }
}
