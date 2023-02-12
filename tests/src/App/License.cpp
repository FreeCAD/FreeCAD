#include "gtest/gtest.h"

#include "App/License.h"

TEST(License, AllRightsReserved)
{
    auto lic = App::License{App::License::Type::AllRightsReserved};
    ASSERT_EQ(lic.getType(), App::License::Type::AllRightsReserved);
    ASSERT_EQ(lic.getLicense(), "All rights reserved");
    ASSERT_EQ(lic.getUrl(), "https://en.wikipedia.org/wiki/All_rights_reserved");
}

TEST(License, CC_BY_40)
{
    auto lic = App::License{App::License::Type::CC_BY_40};
    ASSERT_EQ(lic.getType(), App::License::Type::CC_BY_40);
    ASSERT_EQ(lic.getLicense(), "Creative Commons Attribution");
    ASSERT_EQ(lic.getUrl(), "https://creativecommons.org/licenses/by/4.0/");
}

TEST(License, CC_BY_SA_40)
{
    auto lic = App::License{App::License::Type::CC_BY_SA_40};
    ASSERT_EQ(lic.getType(), App::License::Type::CC_BY_SA_40);
    ASSERT_EQ(lic.getLicense(), "Creative Commons Attribution-ShareAlike");
    ASSERT_EQ(lic.getUrl(), "https://creativecommons.org/licenses/by-sa/4.0/");
}

TEST(License, PublicDomain)
{
    auto lic = App::License{App::License::Type::PublicDomain};
    ASSERT_EQ(lic.getType(), App::License::Type::PublicDomain);
    ASSERT_EQ(lic.getLicense(), "Public Domain");
    ASSERT_EQ(lic.getUrl(), "https://en.wikipedia.org/wiki/Public_domain");
}

TEST(License, FreeArt)
{
    auto lic = App::License{App::License::Type::FreeArt};
    ASSERT_EQ(lic.getType(), App::License::Type::FreeArt);
    ASSERT_EQ(lic.getLicense(), "FreeArt");
    ASSERT_EQ(lic.getUrl(), "https://artlibre.org/licence/lal");
}

TEST(License, CERN_OHS_S)
{
    auto lic = App::License{App::License::Type::CERN_OHS_S};
    ASSERT_EQ(lic.getType(), App::License::Type::CERN_OHS_S);
    ASSERT_EQ(lic.getLicense(), "CERN Open Hardware Licence strongly-reciprocal");
    ASSERT_EQ(lic.getUrl(), "https://cern-ohl.web.cern.ch/");
}

TEST(License, Other)
{
    auto lic = App::License{App::License::Type::Other};
    ASSERT_EQ(lic.getType(), App::License::Type::Other);
    ASSERT_EQ(lic.getLicense(), "Other");
    ASSERT_EQ(lic.getUrl(), "");
}

TEST(License, CompareTypeWithInt)
{
    auto lic1 = App::License{App::License::Type::Other};
    auto lic2 = App::License{static_cast<int>(App::License::Type::Other)};
    ASSERT_EQ(lic1.getType(), lic2.getType());
}

TEST(License, CompareTypeWithLong)
{
    auto lic1 = App::License{App::License::Type::CC_BY_NC_ND_40};
    auto lic2 = App::License{static_cast<long>(App::License::Type::CC_BY_NC_ND_40)};
    ASSERT_EQ(lic1.getType(), lic2.getType());
}

TEST(License, All)
{
    std::vector<std::string> all = App::License::getLicenses();
    int num = static_cast<int>(all.size());
    for (int index = 0; index < num; index++) {
        auto lic = App::License{index};
        auto text = all.at(index);
        ASSERT_EQ(lic.getLicense(), text);
    }
}
