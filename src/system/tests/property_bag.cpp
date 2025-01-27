#include "common.hpp"

#include <erebus/system/property_bag.hxx>


TEST(Er_PropertyBag, simple)
{
    Er::PropertyBag bag;

   auto pb = Er::get<Er::Bool>(bag, Er::Unspecified::Bool);
   ASSERT_FALSE(pb);

   auto u = Er::update(bag, 0, Er::Property(Er::True, Er::Unspecified::Bool));
   ASSERT_TRUE(u);
   pb = Er::get<Er::Bool>(bag, Er::Unspecified::Bool);
   ASSERT_TRUE(pb);
   EXPECT_EQ(*pb, Er::True);
}

