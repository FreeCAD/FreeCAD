#include "gtest/gtest.h"

#include "App/License.h"

TEST(License, isLicenseYesStr)
{
    EXPECT_EQ(App::findLicense("CC_BY_40"), 1);
}

TEST(License, UnknownIdentifier)
{
    int index {App::findLicense("junk")};
    EXPECT_EQ(index, -1);
}

TEST(License, direct)
{
    int posn {App::findLicense("CC_BY_40")};
    App::TLicenseArr tt {"CC_BY_40",
                         "Creative Commons Attribution 4.0",
                         "https://creativecommons.org/licenses/by/4.0/"};
    EXPECT_EQ(App::licenseItems.at(posn), tt);
}

TEST(License, findLicenseByIdent)
{
    App::TLicenseArr arr {App::licenseItems.at(App::findLicense("CC_BY_40"))};

    EXPECT_STREQ(arr.at(App::posnOfIdentifier), "CC_BY_40");
    EXPECT_STREQ(arr.at(App::posnOfFullName), "Creative Commons Attribution 4.0");
    EXPECT_STREQ(arr.at(App::posnOfUrl), "https://creativecommons.org/licenses/by/4.0/");
}
