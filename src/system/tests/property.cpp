#include "common.hpp"

#include <erebus/system/property.hxx>


TEST(Property, ConstructionAssignment)
{
    // empty
    {
        Er::Property v;
        EXPECT_EQ(v.type(), Er::PropertyType::Empty);
        EXPECT_TRUE(v.empty());

        // copy-construct
        Er::Property vc1(v);
        EXPECT_EQ(v.type(), Er::PropertyType::Empty);
        EXPECT_TRUE(v.empty());
        EXPECT_EQ(vc1.type(), Er::PropertyType::Empty);
        EXPECT_TRUE(vc1.empty());

        // move-construct
        Er::Property vc2(std::move(v));
        EXPECT_EQ(v.type(), Er::PropertyType::Empty);
        EXPECT_TRUE(v.empty());
        EXPECT_EQ(vc2.type(), Er::PropertyType::Empty);
        EXPECT_TRUE(vc2.empty());

        // copy-assign
        v = vc1;
        EXPECT_EQ(v.type(), Er::PropertyType::Empty);
        EXPECT_TRUE(v.empty());
        EXPECT_EQ(vc1.type(), Er::PropertyType::Empty);
        EXPECT_TRUE(vc1.empty());

        // move-assign
        v = std::move(vc2);
        EXPECT_EQ(v.type(), Er::PropertyType::Empty);
        EXPECT_TRUE(v.empty());
        EXPECT_EQ(vc2.type(), Er::PropertyType::Empty);
        EXPECT_TRUE(vc2.empty());
    }

    // bool
    {
        Er::Property v1(Er::True, Er::Unspecified::Bool);
        EXPECT_EQ(v1.type(), Er::PropertyType::Bool);
        EXPECT_FALSE(v1.empty());
        EXPECT_EQ(v1.getBool(), Er::True);

        Er::Property v2(Er::False, Er::Unspecified::Bool);
        EXPECT_EQ(v2.type(), Er::PropertyType::Bool);
        EXPECT_FALSE(v2.empty());
        EXPECT_EQ(v2.getBool(), Er::False);

        // copy-construct
        Er::Property vc1(v1);
        EXPECT_EQ(v1.type(), Er::PropertyType::Bool);
        EXPECT_FALSE(v1.empty());
        EXPECT_EQ(vc1.type(), Er::PropertyType::Bool);
        EXPECT_FALSE(vc1.empty());
        EXPECT_EQ(vc1.getBool(), Er::True);

        // move-construct
        Er::Property vc2(std::move(v1));
        EXPECT_EQ(v1.type(), Er::PropertyType::Empty);
        EXPECT_TRUE(v1.empty());
        EXPECT_EQ(vc2.type(), Er::PropertyType::Bool);
        EXPECT_FALSE(vc2.empty());
        EXPECT_EQ(vc2.getBool(), Er::True);

        // copy-assign
        v1 = vc2;
        EXPECT_EQ(v1.type(), Er::PropertyType::Bool);
        EXPECT_FALSE(v1.empty());
        EXPECT_EQ(v1.getBool(), Er::True);
        EXPECT_EQ(vc2.type(), Er::PropertyType::Bool);
        EXPECT_FALSE(vc2.empty());
        EXPECT_EQ(vc2.getBool(), Er::True);

        // move-assign
        v2 = std::move(v1);
        EXPECT_EQ(v2.type(), Er::PropertyType::Bool);
        EXPECT_FALSE(v2.empty());
        EXPECT_EQ(v2.getBool(), Er::True);
        EXPECT_EQ(v1.type(), Er::PropertyType::Empty);
        EXPECT_TRUE(v1.empty());
    }

    // int32
    {
        Er::Property v(int32_t(-10), Er::Unspecified::Int32);
        EXPECT_EQ(v.type(), Er::PropertyType::Int32);
        EXPECT_FALSE(v.empty());
        EXPECT_EQ(v.getInt32(), -10);

        // copy-construct
        Er::Property vc1(v);
        EXPECT_EQ(v.type(), Er::PropertyType::Int32);
        EXPECT_FALSE(v.empty());
        EXPECT_EQ(vc1.type(), Er::PropertyType::Int32);
        EXPECT_FALSE(vc1.empty());
        EXPECT_EQ(vc1.getInt32(), -10);

        // move-construct
        Er::Property vc2(std::move(v));
        EXPECT_EQ(v.type(), Er::PropertyType::Empty);
        EXPECT_TRUE(v.empty());
        EXPECT_EQ(vc2.type(), Er::PropertyType::Int32);
        EXPECT_FALSE(vc2.empty());
        EXPECT_EQ(vc2.getInt32(), -10);

        // copy-assign
        v = vc2;
        EXPECT_EQ(v.type(), Er::PropertyType::Int32);
        EXPECT_FALSE(v.empty());
        EXPECT_EQ(v.getInt32(), -10);
        EXPECT_EQ(vc2.type(), Er::PropertyType::Int32);
        EXPECT_FALSE(vc2.empty());

        // move-assign
        v = std::move(vc2);
        EXPECT_EQ(v.type(), Er::PropertyType::Int32);
        EXPECT_FALSE(v.empty());
        EXPECT_EQ(v.getInt32(), -10);
        EXPECT_EQ(vc2.type(), Er::PropertyType::Empty);
        EXPECT_TRUE(vc2.empty());
    }

    // uint32
    {
        Er::Property v(uint32_t(20), Er::Unspecified::UInt32);
        EXPECT_EQ(v.type(), Er::PropertyType::UInt32);
        EXPECT_FALSE(v.empty());
        EXPECT_EQ(v.getUInt32(), 20);

        // copy-construct
        Er::Property vc1(v);
        EXPECT_EQ(v.type(), Er::PropertyType::UInt32);
        EXPECT_FALSE(v.empty());
        EXPECT_EQ(vc1.type(), Er::PropertyType::UInt32);
        EXPECT_FALSE(vc1.empty());
        EXPECT_EQ(vc1.getUInt32(), 20);

        // move-construct
        Er::Property vc2(std::move(v));
        EXPECT_EQ(v.type(), Er::PropertyType::Empty);
        EXPECT_TRUE(v.empty());
        EXPECT_EQ(vc2.type(), Er::PropertyType::UInt32);
        EXPECT_FALSE(vc2.empty());
        EXPECT_EQ(vc2.getUInt32(), 20);

        // copy-assign
        v = vc2;
        EXPECT_EQ(v.type(), Er::PropertyType::UInt32);
        EXPECT_FALSE(v.empty());
        EXPECT_EQ(v.getUInt32(), 20);
        EXPECT_EQ(vc2.type(), Er::PropertyType::UInt32);
        EXPECT_FALSE(vc2.empty());

        // move-assign
        v = std::move(vc2);
        EXPECT_EQ(v.type(), Er::PropertyType::UInt32);
        EXPECT_FALSE(v.empty());
        EXPECT_EQ(v.getUInt32(), 20);
        EXPECT_EQ(vc2.type(), Er::PropertyType::Empty);
        EXPECT_TRUE(vc2.empty());
    }

    // int64
    {
        Er::Property v(int64_t(-9223372036854775803LL), Er::Unspecified::Int64);
        EXPECT_EQ(v.type(), Er::PropertyType::Int64);
        EXPECT_FALSE(v.empty());
        EXPECT_EQ(v.getInt64(), int64_t(-9223372036854775803LL));

        // copy-construct
        Er::Property vc1(v);
        EXPECT_EQ(v.type(), Er::PropertyType::Int64);
        EXPECT_FALSE(v.empty());
        EXPECT_EQ(vc1.type(), Er::PropertyType::Int64);
        EXPECT_FALSE(vc1.empty());
        EXPECT_EQ(vc1.getInt64(), int64_t(-9223372036854775803LL));

        // move-construct
        Er::Property vc2(std::move(v));
        EXPECT_EQ(v.type(), Er::PropertyType::Empty);
        EXPECT_TRUE(v.empty());
        EXPECT_EQ(vc2.type(), Er::PropertyType::Int64);
        EXPECT_FALSE(vc2.empty());
        EXPECT_EQ(vc2.getInt64(), int64_t(-9223372036854775803LL));

        // copy-assign
        v = vc2;
        EXPECT_EQ(v.type(), Er::PropertyType::Int64);
        EXPECT_FALSE(v.empty());
        EXPECT_EQ(v.getInt64(), int64_t(-9223372036854775803LL));
        EXPECT_EQ(vc2.type(), Er::PropertyType::Int64);
        EXPECT_FALSE(vc2.empty());

        // move-assign
        v = std::move(vc2);
        EXPECT_EQ(v.type(), Er::PropertyType::Int64);
        EXPECT_FALSE(v.empty());
        EXPECT_EQ(v.getInt64(), int64_t(-9223372036854775803LL));
        EXPECT_EQ(vc2.type(), Er::PropertyType::Empty);
        EXPECT_TRUE(vc2.empty());
    }

    // uint64
    {
        Er::Property v(uint64_t(0x8000000000000005ULL), Er::Unspecified::UInt64);
        EXPECT_EQ(v.type(), Er::PropertyType::UInt64);
        EXPECT_FALSE(v.empty());
        EXPECT_EQ(v.getUInt64(), uint64_t(0x8000000000000005ULL));

        // copy-construct
        Er::Property vc1(v);
        EXPECT_EQ(v.type(), Er::PropertyType::UInt64);
        EXPECT_FALSE(v.empty());
        EXPECT_EQ(vc1.type(), Er::PropertyType::UInt64);
        EXPECT_FALSE(vc1.empty());
        EXPECT_EQ(vc1.getUInt64(), uint64_t(0x8000000000000005ULL));

        // move-construct
        Er::Property vc2(std::move(v));
        EXPECT_EQ(v.type(), Er::PropertyType::Empty);
        EXPECT_TRUE(v.empty());
        EXPECT_EQ(vc2.type(), Er::PropertyType::UInt64);
        EXPECT_FALSE(vc2.empty());
        EXPECT_EQ(vc2.getUInt64(), uint64_t(0x8000000000000005ULL));

        // copy-assign
        v = vc2;
        EXPECT_EQ(v.type(), Er::PropertyType::UInt64);
        EXPECT_FALSE(v.empty());
        EXPECT_EQ(v.getUInt64(), uint64_t(0x8000000000000005ULL));
        EXPECT_EQ(vc2.type(), Er::PropertyType::UInt64);
        EXPECT_FALSE(vc2.empty());

        // move-assign
        v = std::move(vc2);
        EXPECT_EQ(v.type(), Er::PropertyType::UInt64);
        EXPECT_FALSE(v.empty());
        EXPECT_EQ(v.getUInt64(), uint64_t(0x8000000000000005ULL));
        EXPECT_EQ(vc2.type(), Er::PropertyType::Empty);
        EXPECT_TRUE(vc2.empty());
    }

    // double
    {
        Er::Property v(3.0, Er::Unspecified::Double);
        EXPECT_EQ(v.type(), Er::PropertyType::Double);
        EXPECT_FALSE(v.empty());
        EXPECT_EQ(v.getDouble(), 3.0);

        // copy-construct
        Er::Property vc1(v);
        EXPECT_EQ(v.type(), Er::PropertyType::Double);
        EXPECT_FALSE(v.empty());
        EXPECT_EQ(vc1.type(), Er::PropertyType::Double);
        EXPECT_FALSE(vc1.empty());
        EXPECT_EQ(vc1.getDouble(), 3.0);

        // move-construct
        Er::Property vc2(std::move(v));
        EXPECT_EQ(v.type(), Er::PropertyType::Empty);
        EXPECT_TRUE(v.empty());
        EXPECT_EQ(vc2.type(), Er::PropertyType::Double);
        EXPECT_FALSE(vc2.empty());
        EXPECT_EQ(vc2.getDouble(), 3.0);

        // copy-assign
        v = vc2;
        EXPECT_EQ(v.type(), Er::PropertyType::Double);
        EXPECT_FALSE(v.empty());
        EXPECT_EQ(v.getDouble(), 3.0);
        EXPECT_EQ(vc2.type(), Er::PropertyType::Double);
        EXPECT_FALSE(vc2.empty());

        // move-assign
        v = std::move(vc2);
        EXPECT_EQ(v.type(), Er::PropertyType::Double);
        EXPECT_FALSE(v.empty());
        EXPECT_EQ(v.getDouble(), 3.0);
        EXPECT_EQ(vc2.type(), Er::PropertyType::Empty);
        EXPECT_TRUE(vc2.empty());
    }

    // string
    {
        std::string s("Test");
        
        Er::Property v1(s, Er::Unspecified::String);
        EXPECT_FALSE(s.empty());
        EXPECT_EQ(v1.type(), Er::PropertyType::String);
        EXPECT_FALSE(v1.empty());
        EXPECT_STREQ(v1.getString().c_str(), "Test");

        Er::Property v2(std::move(s), Er::Unspecified::String);
        EXPECT_TRUE(s.empty());
        EXPECT_EQ(v2.type(), Er::PropertyType::String);
        EXPECT_FALSE(v2.empty());
        EXPECT_STREQ(v2.getString().c_str(), "Test");

        // copy-construct
        Er::Property vc1(v2);
        EXPECT_EQ(v2.type(), Er::PropertyType::String);
        EXPECT_FALSE(v2.empty());
        EXPECT_EQ(vc1.type(), Er::PropertyType::String);
        EXPECT_FALSE(vc1.empty());
        EXPECT_STREQ(vc1.getString().c_str(), "Test");

        // move-construct
        Er::Property vc2(std::move(v2));
        EXPECT_EQ(v2.type(), Er::PropertyType::Empty);
        EXPECT_TRUE(v2.empty());
        EXPECT_EQ(vc2.type(), Er::PropertyType::String);
        EXPECT_FALSE(vc2.empty());
        EXPECT_STREQ(vc2.getString().c_str(), "Test");

        // copy-assign
        Er::Property vc3;
        vc3 = vc2;
        EXPECT_EQ(vc2.type(), Er::PropertyType::String);
        EXPECT_FALSE(vc2.empty());
        EXPECT_STREQ(vc2.getString().c_str(), "Test");
        EXPECT_EQ(vc3.type(), Er::PropertyType::String);
        EXPECT_FALSE(vc3.empty());
        EXPECT_STREQ(vc3.getString().c_str(), "Test");

        // move-assign
        Er::Property vc4;
        vc4 = std::move(vc2);
        EXPECT_EQ(vc2.type(), Er::PropertyType::Empty);
        EXPECT_TRUE(vc2.empty());
        EXPECT_EQ(vc4.type(), Er::PropertyType::String);
        EXPECT_FALSE(vc4.empty());
        EXPECT_STREQ(vc4.getString().c_str(), "Test");
    }

    // binary
    {
        Er::Binary b(std::string_view("Test"));

        Er::Property v1(b, Er::Unspecified::Binary);
        EXPECT_FALSE(b.empty());
        EXPECT_EQ(v1.type(), Er::PropertyType::Binary);
        EXPECT_FALSE(v1.empty());
        EXPECT_STREQ(v1.getBinary().data(), "Test");

        Er::Property v2(std::move(b), Er::Unspecified::Binary);
        EXPECT_TRUE(b.empty());
        EXPECT_EQ(v2.type(), Er::PropertyType::Binary);
        EXPECT_FALSE(v2.empty());
        EXPECT_STREQ(v2.getBinary().data(), "Test");

        // copy-construct
        Er::Property vc1(v2);
        EXPECT_EQ(v2.type(), Er::PropertyType::Binary);
        EXPECT_FALSE(v2.empty());
        EXPECT_EQ(vc1.type(), Er::PropertyType::Binary);
        EXPECT_FALSE(vc1.empty());
        EXPECT_STREQ(vc1.getBinary().data(), "Test");

        // move-construct
        Er::Property vc2(std::move(v2));
        EXPECT_EQ(v2.type(), Er::PropertyType::Empty);
        EXPECT_TRUE(v2.empty());
        EXPECT_EQ(vc2.type(), Er::PropertyType::Binary);
        EXPECT_FALSE(vc2.empty());
        EXPECT_STREQ(vc2.getBinary().data(), "Test");

        // copy-assign
        Er::Property vc3;
        vc3 = vc2;
        EXPECT_EQ(vc2.type(), Er::PropertyType::Binary);
        EXPECT_FALSE(vc2.empty());
        EXPECT_STREQ(vc2.getBinary().data(), "Test");
        EXPECT_EQ(vc3.type(), Er::PropertyType::Binary);
        EXPECT_FALSE(vc3.empty());
        EXPECT_STREQ(vc3.getBinary().data(), "Test");

        // move-assign
        Er::Property vc4;
        vc4 = std::move(vc2);
        EXPECT_EQ(vc2.type(), Er::PropertyType::Empty);
        EXPECT_TRUE(vc2.empty());
        EXPECT_EQ(vc4.type(), Er::PropertyType::Binary);
        EXPECT_FALSE(vc4.empty());
        EXPECT_STREQ(vc4.getBinary().data(), "Test");
    }

    // bool vector
    {
        std::vector<Er::Bool> a{ Er::True, Er::False, Er::True };
        
        Er::Property v1(a, Er::Unspecified::Bools);
        EXPECT_FALSE(a.empty());
        EXPECT_EQ(v1.type(), Er::PropertyType::Bools);
        EXPECT_FALSE(v1.empty());
        auto& r1 = v1.getBools();
        ASSERT_EQ(r1.size(), 3);
        EXPECT_EQ(r1[0], Er::True);
        EXPECT_EQ(r1[1], Er::False);
        EXPECT_EQ(r1[2], Er::True);

        Er::Property v2(std::move(a), Er::Unspecified::Bools);
        EXPECT_TRUE(a.empty());
        EXPECT_EQ(v2.type(), Er::PropertyType::Bools);
        EXPECT_FALSE(v2.empty());
        auto& r2 = v2.getBools();
        ASSERT_EQ(r2.size(), 3);
        EXPECT_EQ(r2[0], Er::True);
        EXPECT_EQ(r2[1], Er::False);
        EXPECT_EQ(r2[2], Er::True);

        // copy-construct
        Er::Property vc1(v2);
        EXPECT_EQ(v2.type(), Er::PropertyType::Bools);
        EXPECT_FALSE(v2.empty());
        EXPECT_EQ(vc1.type(), Er::PropertyType::Bools);
        EXPECT_FALSE(vc1.empty());
        auto& rc1 = vc1.getBools();
        ASSERT_EQ(rc1.size(), 3);
        EXPECT_EQ(rc1[0], Er::True);
        EXPECT_EQ(rc1[1], Er::False);
        EXPECT_EQ(rc1[2], Er::True);

        // move-construct
        Er::Property vc2(std::move(v2));
        EXPECT_EQ(v2.type(), Er::PropertyType::Empty);
        EXPECT_TRUE(v2.empty());
        EXPECT_EQ(vc2.type(), Er::PropertyType::Bools);
        EXPECT_FALSE(vc2.empty());
        auto& rc2 = vc2.getBools();
        ASSERT_EQ(rc2.size(), 3);
        EXPECT_EQ(rc2[0], Er::True);
        EXPECT_EQ(rc2[1], Er::False);
        EXPECT_EQ(rc2[2], Er::True);

        // copy-assign
        Er::Property vc3;
        vc3 = vc2;
        EXPECT_EQ(vc2.type(), Er::PropertyType::Bools);
        EXPECT_FALSE(vc2.empty());
        auto& rc3 = vc3.getBools();
        ASSERT_EQ(rc3.size(), 3);
        EXPECT_EQ(rc3[0], Er::True);
        EXPECT_EQ(rc3[1], Er::False);
        EXPECT_EQ(rc3[2], Er::True);

        // move-assign
        Er::Property vc4;
        vc4 = std::move(vc2);
        EXPECT_EQ(vc2.type(), Er::PropertyType::Empty);
        EXPECT_TRUE(vc2.empty());
        auto& rc4 = vc4.getBools();
        ASSERT_EQ(rc4.size(), 3);
        EXPECT_EQ(rc4[0], Er::True);
        EXPECT_EQ(rc4[1], Er::False);
        EXPECT_EQ(rc4[2], Er::True);
    }

    // int32 vector
    {
        std::vector<int32_t> a{ -1, 2, -3 };

        Er::Property v1(a, Er::Unspecified::Int32s);
        EXPECT_FALSE(a.empty());
        EXPECT_EQ(v1.type(), Er::PropertyType::Int32s);
        EXPECT_FALSE(v1.empty());
        auto& r1 = v1.getInt32s();
        ASSERT_EQ(r1.size(), 3);
        EXPECT_EQ(r1[0], -1);
        EXPECT_EQ(r1[1], 2);
        EXPECT_EQ(r1[2], -3);

        Er::Property v2(std::move(a), Er::Unspecified::Int32s);
        EXPECT_TRUE(a.empty());
        EXPECT_EQ(v2.type(), Er::PropertyType::Int32s);
        EXPECT_FALSE(v2.empty());
        auto& r2 = v2.getInt32s();
        ASSERT_EQ(r2.size(), 3);
        EXPECT_EQ(r2[0], -1);
        EXPECT_EQ(r2[1], 2);
        EXPECT_EQ(r2[2], -3);

        // copy-construct
        Er::Property vc1(v2);
        EXPECT_EQ(v2.type(), Er::PropertyType::Int32s);
        EXPECT_FALSE(v2.empty());
        EXPECT_EQ(vc1.type(), Er::PropertyType::Int32s);
        EXPECT_FALSE(vc1.empty());
        auto& rc1 = vc1.getInt32s();
        ASSERT_EQ(rc1.size(), 3);
        EXPECT_EQ(rc1[0], -1);
        EXPECT_EQ(rc1[1], 2);
        EXPECT_EQ(rc1[2], -3);

        // move-construct
        Er::Property vc2(std::move(v2));
        EXPECT_EQ(v2.type(), Er::PropertyType::Empty);
        EXPECT_TRUE(v2.empty());
        EXPECT_EQ(vc2.type(), Er::PropertyType::Int32s);
        EXPECT_FALSE(vc2.empty());
        auto& rc2 = vc2.getInt32s();
        ASSERT_EQ(rc2.size(), 3);
        EXPECT_EQ(rc2[0], -1);
        EXPECT_EQ(rc2[1], 2);
        EXPECT_EQ(rc2[2], -3);

        // copy-assign
        Er::Property vc3;
        vc3 = vc2;
        EXPECT_EQ(vc2.type(), Er::PropertyType::Int32s);
        EXPECT_FALSE(vc2.empty());
        auto& rc3 = vc3.getInt32s();
        ASSERT_EQ(rc3.size(), 3);
        EXPECT_EQ(rc3[0], -1);
        EXPECT_EQ(rc3[1], 2);
        EXPECT_EQ(rc3[2], -3);

        // move-assign
        Er::Property vc4;
        vc4 = std::move(vc2);
        EXPECT_EQ(vc2.type(), Er::PropertyType::Empty);
        EXPECT_TRUE(vc2.empty());
        auto& rc4 = vc4.getInt32s();
        ASSERT_EQ(rc4.size(), 3);
        EXPECT_EQ(rc4[0], -1);
        EXPECT_EQ(rc4[1], 2);
        EXPECT_EQ(rc4[2], -3);
    }

    // uint32 vector
    {
        std::vector<uint32_t> a{ 1, 2, 3 };

        Er::Property v1(a, Er::Unspecified::UInt32s);
        EXPECT_FALSE(a.empty());
        EXPECT_EQ(v1.type(), Er::PropertyType::UInt32s);
        EXPECT_FALSE(v1.empty());
        auto& r1 = v1.getUInt32s();
        ASSERT_EQ(r1.size(), 3);
        EXPECT_EQ(r1[0], 1);
        EXPECT_EQ(r1[1], 2);
        EXPECT_EQ(r1[2], 3);

        Er::Property v2(std::move(a), Er::Unspecified::UInt32s);
        EXPECT_TRUE(a.empty());
        EXPECT_EQ(v2.type(), Er::PropertyType::UInt32s);
        EXPECT_FALSE(v2.empty());
        auto& r2 = v2.getUInt32s();
        ASSERT_EQ(r2.size(), 3);
        EXPECT_EQ(r2[0], 1);
        EXPECT_EQ(r2[1], 2);
        EXPECT_EQ(r2[2], 3);

        // copy-construct
        Er::Property vc1(v2);
        EXPECT_EQ(v2.type(), Er::PropertyType::UInt32s);
        EXPECT_FALSE(v2.empty());
        EXPECT_EQ(vc1.type(), Er::PropertyType::UInt32s);
        EXPECT_FALSE(vc1.empty());
        auto& rc1 = vc1.getUInt32s();
        ASSERT_EQ(rc1.size(), 3);
        EXPECT_EQ(rc1[0], 1);
        EXPECT_EQ(rc1[1], 2);
        EXPECT_EQ(rc1[2], 3);

        // move-construct
        Er::Property vc2(std::move(v2));
        EXPECT_EQ(v2.type(), Er::PropertyType::Empty);
        EXPECT_TRUE(v2.empty());
        EXPECT_EQ(vc2.type(), Er::PropertyType::UInt32s);
        EXPECT_FALSE(vc2.empty());
        auto& rc2 = vc2.getUInt32s();
        ASSERT_EQ(rc2.size(), 3);
        EXPECT_EQ(rc2[0], 1);
        EXPECT_EQ(rc2[1], 2);
        EXPECT_EQ(rc2[2], 3);

        // copy-assign
        Er::Property vc3;
        vc3 = vc2;
        EXPECT_EQ(vc2.type(), Er::PropertyType::UInt32s);
        EXPECT_FALSE(vc2.empty());
        EXPECT_EQ(vc2.type(), Er::PropertyType::UInt32s);
        EXPECT_FALSE(vc2.empty());
        auto& rc3 = vc3.getUInt32s();
        ASSERT_EQ(rc3.size(), 3);
        EXPECT_EQ(rc3[0], 1);
        EXPECT_EQ(rc3[1], 2);
        EXPECT_EQ(rc3[2], 3);

        // move-assign
        Er::Property vc4;
        vc4 = std::move(vc2);
        EXPECT_EQ(vc2.type(), Er::PropertyType::Empty);
        EXPECT_TRUE(vc2.empty());
        EXPECT_EQ(vc4.type(), Er::PropertyType::UInt32s);
        EXPECT_FALSE(vc4.empty());
        auto& rc4 = vc4.getUInt32s();
        ASSERT_EQ(rc4.size(), 3);
        EXPECT_EQ(rc4[0], 1);
        EXPECT_EQ(rc4[1], 2);
        EXPECT_EQ(rc4[2], 3);
    }

    // int64 vector
    {
        std::vector<int64_t> a{ -1, int64_t(-9223372036854775803LL), -3 };

        Er::Property v1(a, Er::Unspecified::Int64s);
        EXPECT_FALSE(a.empty());
        EXPECT_EQ(v1.type(), Er::PropertyType::Int64s);
        EXPECT_FALSE(v1.empty());
        auto& r1 = v1.getInt64s();
        ASSERT_EQ(r1.size(), 3);
        EXPECT_EQ(r1[0], -1);
        EXPECT_EQ(r1[1], int64_t(-9223372036854775803LL));
        EXPECT_EQ(r1[2], -3);

        Er::Property v2(std::move(a), Er::Unspecified::Int64s);
        EXPECT_TRUE(a.empty());
        EXPECT_EQ(v2.type(), Er::PropertyType::Int64s);
        EXPECT_FALSE(v2.empty());
        auto& r2 = v2.getInt64s();
        ASSERT_EQ(r2.size(), 3);
        EXPECT_EQ(r2[0], -1);
        EXPECT_EQ(r2[1], int64_t(-9223372036854775803LL));
        EXPECT_EQ(r2[2], -3);

        // copy-construct
        Er::Property vc1(v2);
        EXPECT_EQ(v2.type(), Er::PropertyType::Int64s);
        EXPECT_FALSE(v2.empty());
        EXPECT_EQ(vc1.type(), Er::PropertyType::Int64s);
        EXPECT_FALSE(vc1.empty());
        auto& rc1 = vc1.getInt64s();
        ASSERT_EQ(rc1.size(), 3);
        EXPECT_EQ(rc1[0], -1);
        EXPECT_EQ(rc1[1], int64_t(-9223372036854775803LL));
        EXPECT_EQ(rc1[2], -3);

        // move-construct
        Er::Property vc2(std::move(v2));
        EXPECT_EQ(v2.type(), Er::PropertyType::Empty);
        EXPECT_TRUE(v2.empty());
        EXPECT_EQ(vc2.type(), Er::PropertyType::Int64s);
        EXPECT_FALSE(vc2.empty());
        auto& rc2 = vc2.getInt64s();
        ASSERT_EQ(rc2.size(), 3);
        EXPECT_EQ(rc2[0], -1);
        EXPECT_EQ(rc2[1], int64_t(-9223372036854775803LL));
        EXPECT_EQ(rc2[2], -3);

        // copy-assign
        Er::Property vc3;
        vc3 = vc2;
        EXPECT_EQ(vc2.type(), Er::PropertyType::Int64s);
        EXPECT_FALSE(vc2.empty());
        EXPECT_EQ(vc3.type(), Er::PropertyType::Int64s);
        EXPECT_FALSE(vc3.empty());
        auto& rc3 = vc3.getInt64s();
        ASSERT_EQ(rc3.size(), 3);
        EXPECT_EQ(rc3[0], -1);
        EXPECT_EQ(rc3[1], int64_t(-9223372036854775803LL));
        EXPECT_EQ(rc3[2], -3);

        // move-assign
        Er::Property vc4;
        vc4 = std::move(vc2);
        EXPECT_EQ(vc2.type(), Er::PropertyType::Empty);
        EXPECT_TRUE(vc2.empty());
        EXPECT_EQ(vc3.type(), Er::PropertyType::Int64s);
        EXPECT_FALSE(vc3.empty());
        auto& rc4 = vc4.getInt64s();
        ASSERT_EQ(rc4.size(), 3);
        EXPECT_EQ(rc4[0], -1);
        EXPECT_EQ(rc4[1], int64_t(-9223372036854775803LL));
        EXPECT_EQ(rc4[2], -3);
    }

    // uint64 vector
    {
        std::vector<uint64_t> a{ 1, uint64_t(0x8000000000000005ULL), 3 };

        Er::Property v1(a, Er::Unspecified::UInt64s);
        EXPECT_FALSE(a.empty());
        EXPECT_EQ(v1.type(), Er::PropertyType::UInt64s);
        EXPECT_FALSE(v1.empty());
        auto& r1 = v1.getUInt64s();
        ASSERT_EQ(r1.size(), 3);
        EXPECT_EQ(r1[0], 1);
        EXPECT_EQ(r1[1], uint64_t(0x8000000000000005ULL));
        EXPECT_EQ(r1[2], 3);

        Er::Property v2(std::move(a), Er::Unspecified::UInt64s);
        EXPECT_TRUE(a.empty());
        EXPECT_EQ(v2.type(), Er::PropertyType::UInt64s);
        EXPECT_FALSE(v2.empty());
        auto& r2 = v2.getUInt64s();
        ASSERT_EQ(r2.size(), 3);
        EXPECT_EQ(r2[0], 1);
        EXPECT_EQ(r2[1], uint64_t(0x8000000000000005ULL));
        EXPECT_EQ(r2[2], 3);

        // copy-construct
        Er::Property vc1(v2);
        EXPECT_EQ(v2.type(), Er::PropertyType::UInt64s);
        EXPECT_FALSE(v2.empty());
        EXPECT_EQ(vc1.type(), Er::PropertyType::UInt64s);
        EXPECT_FALSE(vc1.empty());
        auto& rc1 = vc1.getUInt64s();
        ASSERT_EQ(rc1.size(), 3);
        EXPECT_EQ(rc1[0], 1);
        EXPECT_EQ(rc1[1], uint64_t(0x8000000000000005ULL));
        EXPECT_EQ(rc1[2], 3);

        // move-construct
        Er::Property vc2(std::move(v2));
        EXPECT_EQ(v2.type(), Er::PropertyType::Empty);
        EXPECT_TRUE(v2.empty());
        EXPECT_EQ(vc2.type(), Er::PropertyType::UInt64s);
        EXPECT_FALSE(vc2.empty());
        auto& rc2 = vc2.getUInt64s();
        ASSERT_EQ(rc2.size(), 3);
        EXPECT_EQ(rc2[0], 1);
        EXPECT_EQ(rc2[1], uint64_t(0x8000000000000005ULL));
        EXPECT_EQ(rc2[2], 3);

        // copy-assign
        Er::Property vc3;
        vc3 = vc2;
        EXPECT_EQ(vc2.type(), Er::PropertyType::UInt64s);
        EXPECT_FALSE(vc2.empty());
        EXPECT_EQ(vc3.type(), Er::PropertyType::UInt64s);
        EXPECT_FALSE(vc3.empty());
        auto& rc3 = vc3.getUInt64s();
        ASSERT_EQ(rc3.size(), 3);
        EXPECT_EQ(rc3[0], 1);
        EXPECT_EQ(rc3[1], uint64_t(0x8000000000000005ULL));
        EXPECT_EQ(rc3[2], 3);

        // move-assign
        Er::Property vc4;
        vc4 = std::move(vc2);
        EXPECT_EQ(vc2.type(), Er::PropertyType::Empty);
        EXPECT_TRUE(vc2.empty());
        EXPECT_EQ(vc3.type(), Er::PropertyType::UInt64s);
        EXPECT_FALSE(vc3.empty());
        auto& rc4 = vc4.getUInt64s();
        ASSERT_EQ(rc4.size(), 3);
        EXPECT_EQ(rc4[0], 1);
        EXPECT_EQ(rc4[1], uint64_t(0x8000000000000005ULL));
        EXPECT_EQ(rc4[2], 3);
    }

    // string vector
    {
        std::vector<std::string> a{ "true", "false", "true" };

        Er::Property v1(a, Er::Unspecified::Strings);
        EXPECT_FALSE(a.empty());
        EXPECT_EQ(v1.type(), Er::PropertyType::Strings);
        EXPECT_FALSE(v1.empty());
        auto& r1 = v1.getStrings();
        ASSERT_EQ(r1.size(), 3);
        EXPECT_STREQ(r1[0].c_str(), "true");
        EXPECT_STREQ(r1[1].c_str(), "false");
        EXPECT_STREQ(r1[2].c_str(), "true");

        Er::Property v2(std::move(a), Er::Unspecified::Strings);
        EXPECT_TRUE(a.empty());
        EXPECT_EQ(v2.type(), Er::PropertyType::Strings);
        EXPECT_FALSE(v2.empty());
        auto& r2 = v2.getStrings();
        ASSERT_EQ(r2.size(), 3);
        EXPECT_STREQ(r1[0].c_str(), "true");
        EXPECT_STREQ(r1[1].c_str(), "false");
        EXPECT_STREQ(r1[2].c_str(), "true");

        // copy-construct
        Er::Property vc1(v2);
        EXPECT_EQ(v2.type(), Er::PropertyType::Strings);
        EXPECT_FALSE(v2.empty());
        EXPECT_EQ(vc1.type(), Er::PropertyType::Strings);
        EXPECT_FALSE(vc1.empty());
        auto& rc1 = vc1.getStrings();
        ASSERT_EQ(rc1.size(), 3);
        EXPECT_STREQ(rc1[0].c_str(), "true");
        EXPECT_STREQ(rc1[1].c_str(), "false");
        EXPECT_STREQ(rc1[2].c_str(), "true");

        // move-construct
        Er::Property vc2(std::move(v2));
        EXPECT_EQ(v2.type(), Er::PropertyType::Empty);
        EXPECT_TRUE(v2.empty());
        EXPECT_EQ(vc2.type(), Er::PropertyType::Strings);
        EXPECT_FALSE(vc2.empty());
        auto& rc2 = vc2.getStrings();
        ASSERT_EQ(rc2.size(), 3);
        EXPECT_STREQ(rc2[0].c_str(), "true");
        EXPECT_STREQ(rc2[1].c_str(), "false");
        EXPECT_STREQ(rc2[2].c_str(), "true");

        // copy-assign
        Er::Property vc3 = vc2;
        EXPECT_EQ(vc2.type(), Er::PropertyType::Strings);
        EXPECT_FALSE(vc2.empty());
        EXPECT_EQ(vc3.type(), Er::PropertyType::Strings);
        EXPECT_FALSE(vc3.empty());
        auto& rc3 = vc3.getStrings();
        ASSERT_EQ(rc3.size(), 3);
        EXPECT_STREQ(rc3[0].c_str(), "true");
        EXPECT_STREQ(rc3[1].c_str(), "false");
        EXPECT_STREQ(rc3[2].c_str(), "true");

        // copy-assign
        Er::Property vc4 = std::move(vc2);
        EXPECT_EQ(vc2.type(), Er::PropertyType::Empty);
        EXPECT_TRUE(vc2.empty());
        EXPECT_EQ(vc4.type(), Er::PropertyType::Strings);
        EXPECT_FALSE(vc4.empty());
        auto& rc4 = vc4.getStrings();
        ASSERT_EQ(rc4.size(), 3);
        EXPECT_STREQ(rc4[0].c_str(), "true");
        EXPECT_STREQ(rc4[1].c_str(), "false");
        EXPECT_STREQ(rc4[2].c_str(), "true");
    }

    // binaries vector
    {
        std::vector<Er::Binary> a{ Er::Binary(std::string("aaa")), Er::Binary(std::string("bbb")), Er::Binary(std::string("ccc")) };

        Er::Property v1(a, Er::Unspecified::Binaries);
        EXPECT_FALSE(a.empty());
        EXPECT_EQ(v1.type(), Er::PropertyType::Binaries);
        EXPECT_FALSE(v1.empty());
        auto& r1 = v1.getBinaries();
        ASSERT_EQ(r1.size(), 3);
        EXPECT_STREQ(r1[0].data(), "aaa");
        EXPECT_STREQ(r1[1].data(), "bbb");
        EXPECT_STREQ(r1[2].data(), "ccc");

        Er::Property v2(std::move(a), Er::Unspecified::Binaries);
        EXPECT_TRUE(a.empty());
        EXPECT_EQ(v2.type(), Er::PropertyType::Binaries);
        EXPECT_FALSE(v2.empty());
        auto& r2 = v2.getBinaries();
        ASSERT_EQ(r2.size(), 3);
        EXPECT_STREQ(r1[0].data(), "aaa");
        EXPECT_STREQ(r1[1].data(), "bbb");
        EXPECT_STREQ(r1[2].data(), "ccc");

        // copy-construct
        Er::Property vc1(v2);
        EXPECT_EQ(v2.type(), Er::PropertyType::Binaries);
        EXPECT_FALSE(v2.empty());
        EXPECT_EQ(vc1.type(), Er::PropertyType::Binaries);
        EXPECT_FALSE(vc1.empty());
        auto& rc1 = vc1.getBinaries();
        ASSERT_EQ(rc1.size(), 3);
        EXPECT_STREQ(rc1[0].data(), "aaa");
        EXPECT_STREQ(rc1[1].data(), "bbb");
        EXPECT_STREQ(rc1[2].data(), "ccc");

        // move-construct
        Er::Property vc2(std::move(v2));
        EXPECT_EQ(v2.type(), Er::PropertyType::Empty);
        EXPECT_TRUE(v2.empty());
        EXPECT_EQ(vc2.type(), Er::PropertyType::Binaries);
        EXPECT_FALSE(vc2.empty());
        auto& rc2 = vc2.getBinaries();
        ASSERT_EQ(rc2.size(), 3);
        EXPECT_STREQ(rc2[0].data(), "aaa");
        EXPECT_STREQ(rc2[1].data(), "bbb");
        EXPECT_STREQ(rc2[2].data(), "ccc");

        // copy-assign
        Er::Property vc3;
        vc3 = vc2;
        EXPECT_EQ(vc2.type(), Er::PropertyType::Binaries);
        EXPECT_FALSE(vc2.empty());
        EXPECT_EQ(vc3.type(), Er::PropertyType::Binaries);
        EXPECT_FALSE(vc3.empty());
        auto& rc3 = vc3.getBinaries();
        ASSERT_EQ(rc3.size(), 3);
        EXPECT_STREQ(rc3[0].data(), "aaa");
        EXPECT_STREQ(rc3[1].data(), "bbb");
        EXPECT_STREQ(rc3[2].data(), "ccc");

        // move-assign
        Er::Property vc4;
        vc4 = std::move(vc2);
        EXPECT_EQ(vc2.type(), Er::PropertyType::Empty);
        EXPECT_TRUE(vc2.empty());
        EXPECT_EQ(vc4.type(), Er::PropertyType::Binaries);
        EXPECT_FALSE(vc4.empty());
        auto& rc4 = vc4.getBinaries();
        ASSERT_EQ(rc4.size(), 3);
        EXPECT_STREQ(rc4[0].data(), "aaa");
        EXPECT_STREQ(rc4[1].data(), "bbb");
        EXPECT_STREQ(rc4[2].data(), "ccc");
    }
}

TEST(Property, Compare)
{
    // empty
    {
        Er::Property v1;
        Er::Property v2;

        EXPECT_TRUE(v1 == v2);
        EXPECT_FALSE(v1 != v2);
    }

    // bool
    {
        Er::Property v1(true, Er::Unspecified::Bool);
        Er::Property v2(false, Er::Unspecified::Bool);
        Er::Property v3(true, Er::Unspecified::Bool);
        EXPECT_FALSE(v1 == v2);
        EXPECT_TRUE(v1 != v2);
        EXPECT_TRUE(v1 == v3);
        EXPECT_FALSE(v1 != v3);
    }

    // int32
    {
        Er::Property v1(int32_t(-10), Er::Unspecified::Int32);
        Er::Property v2(int32_t(30), Er::Unspecified::Int32);
        Er::Property v3(int32_t(-10), Er::Unspecified::Int32);
        EXPECT_FALSE(v1 == v2);
        EXPECT_TRUE(v1 != v2);
        EXPECT_TRUE(v1 == v3);
        EXPECT_FALSE(v1 != v3);
    }

    // uint32
    {
        Er::Property v1(uint32_t(10), Er::Unspecified::UInt32);
        Er::Property v2(uint32_t(30), Er::Unspecified::UInt32);
        Er::Property v3(uint32_t(10), Er::Unspecified::UInt32);
        EXPECT_FALSE(v1 == v2);
        EXPECT_TRUE(v1 != v2);
        EXPECT_TRUE(v1 == v3);
        EXPECT_FALSE(v1 != v3);
    }

    // int64
    {
        Er::Property v1(int64_t(-9223372036854775803LL), Er::Unspecified::Int64);
        Er::Property v2(int64_t(22LL), Er::Unspecified::Int64);
        Er::Property v3(int64_t(-9223372036854775803LL), Er::Unspecified::Int64);
        EXPECT_FALSE(v1 == v2);
        EXPECT_TRUE(v1 != v2);
        EXPECT_TRUE(v1 == v3);
        EXPECT_FALSE(v1 != v3);
    }

    // uint64
    {
        Er::Property v1(uint64_t(0x8000000000000005ULL), Er::Unspecified::UInt64);
        Er::Property v2(uint64_t(5ULL), Er::Unspecified::UInt64);
        Er::Property v3(uint64_t(0x8000000000000005ULL), Er::Unspecified::UInt64);
        EXPECT_FALSE(v1 == v2);
        EXPECT_TRUE(v1 != v2);
        EXPECT_TRUE(v1 == v3);
        EXPECT_FALSE(v1 != v3);
    }

    // double
    {
        Er::Property v1(3.0, Er::Unspecified::Double);
        Er::Property v2(-3.0, Er::Unspecified::Double);
        Er::Property v3(3.0, Er::Unspecified::Double);
        EXPECT_FALSE(v1 == v2);
        EXPECT_TRUE(v1 != v2);
        EXPECT_TRUE(v1 == v3);
        EXPECT_FALSE(v1 != v3);
    }

    // string
    {
        Er::Property v1(std::string("Test"), Er::Unspecified::String);
        Er::Property v2("Test1", Er::Unspecified::String);
        Er::Property v3("Test", Er::Unspecified::String);
        EXPECT_FALSE(v1 == v2);
        EXPECT_TRUE(v1 != v2);
        EXPECT_TRUE(v1 == v3);
        EXPECT_FALSE(v1 != v3);
    }

    // binary
    {
        Er::Property v1(Er::Binary("Test"), Er::Unspecified::Binary);
        Er::Property v2(Er::Binary("Test1"), Er::Unspecified::Binary);
        Er::Property v3(Er::Binary("Test"), Er::Unspecified::Binary);
        EXPECT_FALSE(v1 == v2);
        EXPECT_TRUE(v1 != v2);
        EXPECT_TRUE(v1 == v3);
        EXPECT_FALSE(v1 != v3);
    }

    // bool vector
    {
        std::vector<Er::Bool> a1{ Er::True, Er::False, Er::True };
        Er::Property v1(std::move(a1), Er::Unspecified::Bools);
        std::vector<Er::Bool> a2{ Er::False, Er::False, Er::True };
        Er::Property v2(std::move(a2), Er::Unspecified::Bools);
        std::vector<Er::Bool> a3{ Er::True, Er::False, Er::True };
        Er::Property v3(std::move(a3), Er::Unspecified::Bools);
        EXPECT_FALSE(v1 == v2);
        EXPECT_TRUE(v1 != v2);
        EXPECT_TRUE(v1 == v3);
        EXPECT_FALSE(v1 != v3);
    }

    // int32 vector
    {
        std::vector<int32_t> a1{ -1, 2, -3 };
        Er::Property v1(std::move(a1), Er::Unspecified::Int32s);
        std::vector<int32_t> a2{ -1, -2, -3 };
        Er::Property v2(std::move(a2), Er::Unspecified::Int32s);
        std::vector<int32_t> a3{ -1, 2, -3 };
        Er::Property v3(std::move(a3), Er::Unspecified::Int32s);
        EXPECT_FALSE(v1 == v2);
        EXPECT_TRUE(v1 != v2);
        EXPECT_TRUE(v1 == v3);
        EXPECT_FALSE(v1 != v3);
    }

    // uint32 vector
    {
        std::vector<uint32_t> a1{ 1, 2, 3 };
        Er::Property v1(std::move(a1), Er::Unspecified::UInt32s);
        std::vector<uint32_t> a2{ 1, 32, 3 };
        Er::Property v2(std::move(a2), Er::Unspecified::UInt32s);
        std::vector<uint32_t> a3{ 1, 2, 3 };
        Er::Property v3(std::move(a3), Er::Unspecified::UInt32s);
        EXPECT_FALSE(v1 == v2);
        EXPECT_TRUE(v1 != v2);
        EXPECT_TRUE(v1 == v3);
        EXPECT_FALSE(v1 != v3);
    }

    // int64 vector
    {
        std::vector<int64_t> a1{ -1, int64_t(-9223372036854775803LL), -3 };
        Er::Property v1(std::move(a1), Er::Unspecified::Int64s);
        std::vector<int64_t> a2{ -1, 2, -3 };
        Er::Property v2(std::move(a2), Er::Unspecified::Int64s);
        std::vector<int64_t> a3{ -1, int64_t(-9223372036854775803LL), -3 };
        Er::Property v3(std::move(a3), Er::Unspecified::Int64s);
        EXPECT_FALSE(v1 == v2);
        EXPECT_TRUE(v1 != v2);
        EXPECT_TRUE(v1 == v3);
        EXPECT_FALSE(v1 != v3);
    }

    // uint64 vector
    {
        std::vector<uint64_t> a1{ 1, uint64_t(0x8000000000000005ULL), 3 };
        Er::Property v1(std::move(a1), Er::Unspecified::UInt64s);
        std::vector<uint64_t> a2{ 1, 2, 3 };
        Er::Property v2(std::move(a2), Er::Unspecified::UInt64s);
        std::vector<uint64_t> a3{ 1, uint64_t(0x8000000000000005ULL), 3 };
        Er::Property v3(std::move(a3), Er::Unspecified::UInt64s);
        EXPECT_FALSE(v1 == v2);
        EXPECT_TRUE(v1 != v2);
        EXPECT_TRUE(v1 == v3);
        EXPECT_FALSE(v1 != v3);
    }

    // string vector
    {
        std::vector<std::string> a1{ "true", "false", "true" };
        Er::Property v1(std::move(a1), Er::Unspecified::Strings);
        std::vector<std::string> a2{ "true", "false", "false" };
        Er::Property v2(std::move(a2), Er::Unspecified::Strings);
        std::vector<std::string> a3{ "true", "false", "true" };
        Er::Property v3(std::move(a3), Er::Unspecified::Strings);
        EXPECT_FALSE(v1 == v2);
        EXPECT_TRUE(v1 != v2);
        EXPECT_TRUE(v1 == v3);
        EXPECT_FALSE(v1 != v3);
    }

    // binaries vector
    {
        std::vector<Er::Binary> a1{ Er::Binary(std::string_view("aaa")), Er::Binary(std::string_view("bbb")), Er::Binary(std::string_view("ccc")) };
        Er::Property v1(std::move(a1), Er::Unspecified::Binaries);
        std::vector<Er::Binary> a2{ Er::Binary(std::string_view("aaa")), Er::Binary(std::string_view("bbb")), Er::Binary(std::string_view("eee")) };
        Er::Property v2(std::move(a2), Er::Unspecified::Binaries);
        std::vector<Er::Binary> a3{ Er::Binary(std::string_view("aaa")), Er::Binary(std::string_view("bbb")), Er::Binary(std::string_view("ccc")) };
        Er::Property v3(std::move(a3), Er::Unspecified::Binaries);
        EXPECT_FALSE(v1 == v2);
        EXPECT_TRUE(v1 != v2);
        EXPECT_TRUE(v1 == v3);
        EXPECT_FALSE(v1 != v3);
    }
}