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

}

TEST(Property, Compare)
{
    // empty
    {
        Er::Property v1;
        Er::Property v2;
        Er::Property v3(false, Er::Unspecified::Bool); // different type

        EXPECT_TRUE(v1 == v2);
        EXPECT_FALSE(v1 != v2);
        EXPECT_FALSE(v1 == v3);
    }

    // bool
    {
        Er::Property v1(true, Er::Unspecified::Bool);
        Er::Property v2(false, Er::Unspecified::Bool);
        Er::Property v3(true, Er::Unspecified::Bool);
        Er::Property v4(int32_t(1), Er::Unspecified::Int32);
        EXPECT_FALSE(v1 == v2);
        EXPECT_TRUE(v1 != v2);
        EXPECT_TRUE(v1 == v3);
        EXPECT_FALSE(v1 != v3);
        EXPECT_FALSE(v1 == v4);
    }

    // int32
    {
        Er::Property v1(int32_t(-10), Er::Unspecified::Int32);
        Er::Property v2(int32_t(10), Er::Unspecified::Int32);
        Er::Property v3(int32_t(-10), Er::Unspecified::Int32);
        Er::Property v4(uint32_t(10), Er::Unspecified::UInt32);
        EXPECT_FALSE(v1 == v2);
        EXPECT_TRUE(v1 != v2);
        EXPECT_TRUE(v1 == v3);
        EXPECT_FALSE(v1 != v3);
        EXPECT_FALSE(v2 == v4); // same value, different types
    }

    // uint32
    {
        Er::Property v1(uint32_t(10), Er::Unspecified::UInt32);
        Er::Property v2(uint32_t(30), Er::Unspecified::UInt32);
        Er::Property v3(uint32_t(10), Er::Unspecified::UInt32);
        Er::Property v4(int64_t(30), Er::Unspecified::Int64);
        EXPECT_FALSE(v1 == v2);
        EXPECT_TRUE(v1 != v2);
        EXPECT_TRUE(v1 == v3);
        EXPECT_FALSE(v1 != v3);
        EXPECT_FALSE(v2 == v4); // same value, different types
    }

    // int64
    {
        Er::Property v1(int64_t(-9223372036854775803LL), Er::Unspecified::Int64);
        Er::Property v2(int64_t(22LL), Er::Unspecified::Int64);
        Er::Property v3(int64_t(-9223372036854775803LL), Er::Unspecified::Int64);
        Er::Property v4(uint64_t(22ULL), Er::Unspecified::UInt64);
        EXPECT_FALSE(v1 == v2);
        EXPECT_TRUE(v1 != v2);
        EXPECT_TRUE(v1 == v3);
        EXPECT_FALSE(v1 != v3);
        EXPECT_FALSE(v2 == v4); // same value, different types
    }

    // uint64
    {
        Er::Property v1(uint64_t(0x8000000000000005ULL), Er::Unspecified::UInt64);
        Er::Property v2(uint64_t(5ULL), Er::Unspecified::UInt64);
        Er::Property v3(uint64_t(0x8000000000000005ULL), Er::Unspecified::UInt64);
        Er::Property v4(5.0, Er::Unspecified::Double);
        EXPECT_FALSE(v1 == v2);
        EXPECT_TRUE(v1 != v2);
        EXPECT_TRUE(v1 == v3);
        EXPECT_FALSE(v1 != v3);
        EXPECT_FALSE(v2 == v4); // same value, different types
    }

    // double
    {
        Er::Property v1(3.0, Er::Unspecified::Double);
        Er::Property v2(-3.0, Er::Unspecified::Double);
        Er::Property v3(3.0, Er::Unspecified::Double);
        Er::Property v4("-3.0", Er::Unspecified::String);
        EXPECT_FALSE(v1 == v2);
        EXPECT_TRUE(v1 != v2);
        EXPECT_TRUE(v1 == v3);
        EXPECT_FALSE(v1 != v3);
        EXPECT_FALSE(v2 == v4); // same value, different types
    }

    // string
    {
        Er::Property v1(std::string("Test"), Er::Unspecified::String);
        Er::Property v2("Test1", Er::Unspecified::String);
        Er::Property v3("Test", Er::Unspecified::String);
        Er::Property v4(Er::Binary("Test1"), Er::Unspecified::Binary);
        EXPECT_FALSE(v1 == v2);
        EXPECT_TRUE(v1 != v2);
        EXPECT_TRUE(v1 == v3);
        EXPECT_FALSE(v1 != v3);
        EXPECT_FALSE(v2 == v4); // same value, different types
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

}