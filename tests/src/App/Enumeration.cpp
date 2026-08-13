// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include <App/Enumeration.h>


TEST(Enumeration, PreservesIndexWhenLabelsChange)
{
    App::Enumeration enumeration;
    enumeration.setEnums(std::vector<std::string> {"Small", "Large"});
    enumeration.setValue(1);

    enumeration.setEnums(std::vector<std::string> {"Small: 10 mm", "Large: 20 mm"});

    EXPECT_EQ(enumeration.getInt(), 1);
    EXPECT_STREQ(enumeration.getCStr(), "Large: 20 mm");
}

TEST(Enumeration, PreservesIndexWhenStaticLabelsChange)
{
    const char* initial[] = {"Small", "Large", nullptr};
    const char* relabeled[] = {"Small: 10 mm", "Large: 20 mm", nullptr};
    App::Enumeration enumeration;
    enumeration.setEnums(initial);
    enumeration.setValue(1);

    enumeration.setEnums(relabeled);

    EXPECT_EQ(enumeration.getInt(), 1);
    EXPECT_STREQ(enumeration.getCStr(), "Large: 20 mm");
}

TEST(Enumeration, PreservesStringWhenItemsMove)
{
    App::Enumeration enumeration;
    enumeration.setEnums(std::vector<std::string> {"Small", "Large"});
    enumeration.setValue("Large");

    enumeration.setEnums(std::vector<std::string> {"Large", "Small"});

    EXPECT_EQ(enumeration.getInt(), 0);
    EXPECT_STREQ(enumeration.getCStr(), "Large");
}

TEST(Enumeration, ResetsWhenOldIndexIsUnavailable)
{
    App::Enumeration enumeration;
    enumeration.setEnums(std::vector<std::string> {"Small", "Medium", "Large"});
    enumeration.setValue(2);

    enumeration.setEnums(std::vector<std::string> {"Small: 10 mm", "Medium: 15 mm"});

    EXPECT_EQ(enumeration.getInt(), 0);
    EXPECT_STREQ(enumeration.getCStr(), "Small: 10 mm");
}
