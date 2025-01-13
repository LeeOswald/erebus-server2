#include "common.hpp"

#include <erebus/system/flags.hxx>

struct F
{
    static constexpr std::size_t _0 = 0;
    static constexpr std::size_t _1 = 1;
    static constexpr std::size_t _20 = 20;
    static constexpr std::size_t _69 = 69;
};

using FF = Er::FlagsPack<70, F>;


struct F16
{
    static constexpr std::size_t _0 = 0;
    static constexpr std::size_t _15 = 15;
};

using FF16 = Er::FlagsPack<16, F16>;

TEST(FlagsPack, Simple)
{
    // default constructor
    {
        FF f;
        EXPECT_EQ(f.size(), 70);
        EXPECT_FALSE(f.any());
        EXPECT_TRUE(f.none());
        EXPECT_EQ(f.count(), 0);
        EXPECT_FALSE(f[F::_0]);
        EXPECT_FALSE(f[F::_1]);
        EXPECT_FALSE(f[F::_20]);
        EXPECT_FALSE(f[F::_69]);
        EXPECT_TRUE(f == FF());
        EXPECT_TRUE(f != FF({ FF::_0, FF::_1 }));
    }

    //construct from initializer_list
    {
        FF f({ FF::_0, FF::_1 });
        EXPECT_EQ(f.size(), 70);
        EXPECT_TRUE(f.any());
        EXPECT_FALSE(f.none());
        EXPECT_EQ(f.count(), 2);
        EXPECT_TRUE(f[F::_0]);
        EXPECT_TRUE(f[F::_1]);
        EXPECT_FALSE(f[F::_20]);
        EXPECT_FALSE(f[F::_69]);
        EXPECT_TRUE(f != FF());
        EXPECT_TRUE(f == FF({ FF::_0, FF::_1 }));
    }

    // construct from uint32_t
    {
        FF f(uint32_t(0x80000001), FF::FromBits);
        EXPECT_EQ(f.size(), 70);
        EXPECT_TRUE(f.any());
        EXPECT_FALSE(f.none());
        EXPECT_EQ(f.count(), 2);
        EXPECT_TRUE(f[F::_0]);
        EXPECT_TRUE(f[31]);
        EXPECT_FALSE(f[F::_1]);
        EXPECT_FALSE(f[F::_20]);
        EXPECT_FALSE(f[F::_69]);
        EXPECT_FALSE(f == FF());
        EXPECT_TRUE(f == FF({ FF::_0, 31 }));
    }

    // construct from uint64_t
    {
        FF f(uint64_t(0x8000000080000001), FF::FromBits);
        EXPECT_EQ(f.size(), 70);
        EXPECT_TRUE(f.any());
        EXPECT_FALSE(f.none());
        EXPECT_EQ(f.count(), 3);
        EXPECT_TRUE(f[F::_0]);
        EXPECT_TRUE(f[31]);
        EXPECT_TRUE(f[63]);
        EXPECT_FALSE(f[F::_1]);
        EXPECT_FALSE(f[F::_20]);
        EXPECT_FALSE(f[F::_69]);
        EXPECT_FALSE(f == FF());
        EXPECT_TRUE(f == FF({ FF::_0, 31, 63 }));
    }

    // construct from flags
    {
        FF f{ F::_0, F::_20};
        EXPECT_EQ(f.size(), 70);
        EXPECT_TRUE(f.any());
        EXPECT_FALSE(f.none());
        EXPECT_EQ(f.count(), 2);
        EXPECT_TRUE(f[F::_0]);
        EXPECT_FALSE(f[F::_1]);
        EXPECT_TRUE(f[F::_20]);
        EXPECT_FALSE(f[F::_69]);
        EXPECT_FALSE(f == FF());
        EXPECT_TRUE(f == FF({ FF::_0, FF::_20 }));
    }

    // construct from string
    {
        FF f(std::string_view("100000000000000000010"));
        EXPECT_EQ(f.size(), 70);
        EXPECT_TRUE(f.any());
        EXPECT_FALSE(f.none());
        EXPECT_EQ(f.count(), 2);
        EXPECT_FALSE(f[F::_0]);
        EXPECT_TRUE(f[F::_1]);
        EXPECT_TRUE(f[F::_20]);
        EXPECT_FALSE(f[F::_69]);
        EXPECT_FALSE(f == FF());
        EXPECT_TRUE(f == FF({ FF::_1, FF::_20 }));
    }

    // construct from empty string
    {
        FF f(std::string_view(""));
        EXPECT_EQ(f.size(), 70);
        EXPECT_FALSE(f.any());
        EXPECT_TRUE(f.none());
        EXPECT_EQ(f.count(), 0);
        EXPECT_FALSE(f[F::_0]);
        EXPECT_FALSE(f[F::_1]);
        EXPECT_FALSE(f[F::_20]);
        EXPECT_FALSE(f[F::_69]);
        EXPECT_TRUE(f == FF());
        EXPECT_TRUE(f != FF({ FF::_0, FF::_1 }));
    }

    // convert to string
    {
        std::string source("0000000000000000000000000000000000000000000000000100000000000000000010");
        FF f{ std::string_view(source) };
        auto dest = f.to_string();
        EXPECT_STREQ(source.c_str(), dest.c_str());
    }

    // pack
    {
        FF16 f({ FF16::_0, FF16::_15 });
        EXPECT_EQ(f.size(), 16);
        EXPECT_TRUE(f.any());
        EXPECT_FALSE(f.none());
        EXPECT_EQ(f.count(), 2);
        EXPECT_TRUE(f[FF16::_0]);
        EXPECT_TRUE(f[FF16::_15]);
        EXPECT_FALSE(f[5]);
        EXPECT_FALSE(f[7]);
        EXPECT_TRUE(f != FF16());
        EXPECT_TRUE(f == FF16({ FF16::_0, FF16::_15 }));
        EXPECT_EQ(f.pack<uint32_t>(), 0x00008001);
        EXPECT_EQ(f.pack<uint64_t>(), 0x0000000000008001);
    }

}