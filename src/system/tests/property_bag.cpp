#include "common.hpp"

#include <erebus/system/property_bag.hxx>


TEST(Er_PropertyBag, simple)
{
    Er::PropertyBag bag;

   auto pb = Er::get<Er::Bool>(bag, Er::Unspecified::Bool);
   ASSERT_FALSE(pb);

   auto u = Er::update<Er::Bool>(bag, 0, Er::Property(Er::True, Er::Unspecified::Bool));
   ASSERT_TRUE(u);
   pb = Er::get<Er::Bool>(bag, Er::Unspecified::Bool);
   ASSERT_TRUE(pb);
   EXPECT_EQ(*pb, Er::True);

   auto ps = Er::get<std::string>(bag, Er::Unspecified::String);
   EXPECT_FALSE(ps);
   u = Er::update<std::string>(bag, 1, Er::Property(std::string("test 1"), Er::Unspecified::String));
   ASSERT_TRUE(u);
   ps = Er::get<std::string>(bag, Er::Unspecified::String);
   ASSERT_TRUE(ps);
   EXPECT_STREQ(ps->c_str(), "test 1");
  
   // update the existing prop
   u = Er::update<std::string>(bag, 1, Er::Property(std::string("test 2"), Er::Unspecified::String));
   EXPECT_TRUE(u);
   ps = Er::get<std::string>(bag, Er::Unspecified::String);
   ASSERT_TRUE(ps);
   EXPECT_STREQ(ps->c_str(), "test 2");

   // don't update the existing prop
   u = Er::update<std::string>(bag, 1, Er::Property(std::string("test 2"), Er::Unspecified::String));
   EXPECT_FALSE(u);
   ps = Er::get<std::string>(bag, Er::Unspecified::String);
   ASSERT_TRUE(ps);
   EXPECT_STREQ(ps->c_str(), "test 2");

}

