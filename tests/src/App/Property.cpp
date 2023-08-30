#include "gtest/gtest.h"

#include "App/PropertyLinks.h"

TEST(PropertyLink, TestSetValues)
{
    App::PropertyLinkSubList prop;
    std::vector<App::DocumentObject*> objs {nullptr, nullptr};
    std::vector<const char*> subs {"Sub1", "Sub2"};
    prop.setValues(objs, subs);
    const auto& sub = prop.getSubValues();
    EXPECT_EQ(sub.size(), 2);
    EXPECT_EQ(sub[0], "Sub1");
    EXPECT_EQ(sub[1], "Sub2");
}
