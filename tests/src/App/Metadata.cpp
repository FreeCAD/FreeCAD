/**************************************************************************
 *                                                                         *
 *   Copyright (c) 2021-2023 FreeCAD Project Association                   *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 ***************************************************************************/

// NOLINTNEXTLINE
#include "gtest/gtest.h"

#include "App/Metadata.h"

// NOLINTBEGIN(readability-named-parameter)

TEST(ContactTest, ContactDefaultConstruction)
{
    auto contact = App::Meta::Contact();
    ASSERT_EQ(contact.name, "");
    ASSERT_EQ(contact.email, "");
}

TEST(ContactTest, ContactParameterConstruction)
{
    auto contact = App::Meta::Contact("Name", "email@some.com");
    ASSERT_EQ(contact.name, "Name");
    ASSERT_EQ(contact.email, "email@some.com");
}

TEST(ContactTest, ContactNullDomNodeConstruction)
{
    auto contact = App::Meta::Contact(nullptr);
    ASSERT_EQ(contact.name, "");
    ASSERT_EQ(contact.email, "");
}

TEST(ContactTest, ContactOperatorEquality)
{
    auto contact1 = App::Meta::Contact("Name", "Email");
    auto contact2 = App::Meta::Contact("Name", "Email");
    auto contact3 = App::Meta::Contact("NotName", "Email");
    auto contact4 = App::Meta::Contact("Name", "NotEmail");
    ASSERT_TRUE(contact1 == contact2);
    ASSERT_FALSE(contact1 == contact3);
    ASSERT_FALSE(contact1 == contact4);
}

TEST(LicenseTest, LicenseDefaultConstruction)
{
    auto contact = App::Meta::License();
    ASSERT_EQ(contact.name, "");
    ASSERT_EQ(contact.file, "");
}

TEST(LicenseTest, LicenseParameterConstruction)
{
    auto contact = App::Meta::License("Name", "path/to/file");
    ASSERT_EQ(contact.name, "Name");
    ASSERT_EQ(contact.file, "path/to/file");
}

TEST(LicenseTest, LicenseNullDomNodeConstruction)
{
    auto contact = App::Meta::License(nullptr);
    ASSERT_EQ(contact.name, "");
    ASSERT_EQ(contact.file, "");
}

TEST(LicenseTest, LicenseOperatorEquality)
{
    auto license1 = App::Meta::License("Name", "path/to/file");
    auto license2 = App::Meta::License("Name", "path/to/file");
    auto license3 = App::Meta::License("NotName", "path/to/file");
    auto license4 = App::Meta::License("Name", "not/path/to/file");
    ASSERT_TRUE(license1 == license2);
    ASSERT_FALSE(license1 == license3);
    ASSERT_FALSE(license1 == license4);
}

TEST(UrlTest, UrlDefaultConstruction)
{
    auto url = App::Meta::Url();
    ASSERT_EQ(url.location, "");
    ASSERT_EQ(url.type, App::Meta::UrlType::website);
}

TEST(UrlTest, UrlParameterConstruction)
{
    auto url = App::Meta::Url("https://some.url", App::Meta::UrlType::documentation);
    ASSERT_EQ(url.location, "https://some.url");
    ASSERT_EQ(url.type, App::Meta::UrlType::documentation);
}

TEST(UrlTest, UrlNullDomNodeConstruction)
{
    auto url = App::Meta::Url(nullptr);
    ASSERT_EQ(url.location, "");
}

TEST(UrlTest, UrlOperatorEquality)
{
    auto url1 = App::Meta::Url("https://some.url", App::Meta::UrlType::documentation);
    auto url2 = App::Meta::Url("https://some.url", App::Meta::UrlType::documentation);
    auto url3 = App::Meta::Url("https://not.some.url", App::Meta::UrlType::documentation);
    auto url4 = App::Meta::Url("https://some.url", App::Meta::UrlType::website);
    ASSERT_TRUE(url1 == url2);
    ASSERT_FALSE(url1 == url3);
    ASSERT_FALSE(url1 == url4);
}

TEST(VersionTest, VersionDefaultConstruction)
{
    auto version = App::Meta::Version();
    ASSERT_EQ(version.major, 0);
    ASSERT_EQ(version.minor, 0);
    ASSERT_EQ(version.patch, 0);
    ASSERT_EQ(version.suffix, "");
}

TEST(VersionTest, VersionParameterConstruction)
{
    auto version = App::Meta::Version(1, 2, 3, "delta");
    ASSERT_EQ(version.major, 1);
    ASSERT_EQ(version.minor, 2);
    ASSERT_EQ(version.patch, 3);
    ASSERT_EQ(version.suffix, "delta");
}

TEST(VersionTest, VersionStringConstruction)
{
    auto version = App::Meta::Version("1.2.3delta");
    ASSERT_EQ(version.major, 1);
    ASSERT_EQ(version.minor, 2);
    ASSERT_EQ(version.patch, 3);
    ASSERT_EQ(version.suffix, "delta");
}

TEST(VersionTest, VersionOperatorEqual)
{
    auto version1 = App::Meta::Version(1, 2, 3, "delta");
    auto version2 = App::Meta::Version(1, 2, 3, "delta");
    auto version3 = App::Meta::Version(1, 3, 3, "delta");
    auto version4 = App::Meta::Version(1, 2, 4, "delta");
    auto version5 = App::Meta::Version(1, 2, 3, "gamma");
    ASSERT_EQ(version1, version2);
    ASSERT_NE(version1, version3);
    ASSERT_NE(version1, version4);
    ASSERT_NE(version1, version5);
}

TEST(VersionTest, VersionOperatorComparison)
{
    auto version_2_3_4_delta = App::Meta::Version(2, 3, 4, "delta");
    auto version_1_3_4_delta = App::Meta::Version(1, 3, 4, "delta");
    auto version_3_3_4_delta = App::Meta::Version(3, 3, 4, "delta");
    auto version_2_2_4_delta = App::Meta::Version(2, 2, 4, "delta");
    auto version_2_4_4_delta = App::Meta::Version(2, 4, 4, "delta");
    auto version_2_3_3_delta = App::Meta::Version(2, 3, 3, "delta");
    // NOLINTNEXTLINE Five is not really a "magic number" in this test
    auto version_2_3_5_delta = App::Meta::Version(2, 3, 5, "delta");
    auto version_2_3_4_epsilon = App::Meta::Version(2, 3, 4, "epsilon");
    auto version_2_3_4_beta = App::Meta::Version(2, 3, 4, "beta");
    ASSERT_GT(version_2_3_4_delta, version_1_3_4_delta);
    ASSERT_LT(version_2_3_4_delta, version_3_3_4_delta);
    ASSERT_GT(version_2_3_4_delta, version_2_2_4_delta);
    ASSERT_LT(version_2_3_4_delta, version_2_4_4_delta);
    ASSERT_GT(version_2_3_4_delta, version_2_3_3_delta);
    ASSERT_LT(version_2_3_4_delta, version_2_3_5_delta);
    ASSERT_GT(version_2_3_4_delta, version_2_3_4_beta);
    ASSERT_LT(version_2_3_4_delta, version_2_3_4_epsilon);
}

// TODO: Test Dependency
// TODO: Test GenericMetadata

class MetadataTest: public ::testing::Test
{
protected:
    void SetUp() override
    {
        xercesc_3_2::XMLPlatformUtils::Initialize();
    }
    void TearDown() override
    {
        xercesc_3_2::XMLPlatformUtils::Terminate();
    }
    std::string GivenSimpleMetadataXMLString()
    {
        std::ostringstream stream;
        stream << "<?xml version=\"1.0\" encoding=\"utf-8\" standalone=\"no\" ?>\n"
               << "<package format=\"1\" xmlns=\"https://wiki.freecad.org/Package_Metadata\">\n"
               << "  <name>" << _name << "</name>\n"
               << "  <description>" << _description << "</description>\n"
               << "  <version>" << _version << "</version>\n"
               << "  <date>2022-01-07</date>\n"
               << "  <url type=\"repository\" "
                  "branch=\"main\">https://github.com/FreeCAD/FreeCAD</url>\n"
               << "</package>";
        return stream.str();
    }

    void AssertMetadataMatches(App::Metadata& testObject)
    {
        ASSERT_EQ(testObject.name(), _name);
        ASSERT_EQ(testObject.description(), _description);
        ASSERT_EQ(testObject.version(), App::Meta::Version(_version));
    }

private:
    std::string _name = "TestAddon";
    std::string _description = "A package.xml file for unit testing.";
    std::string _version = "1.2.3beta";
};

TEST_F(MetadataTest, MetadataInMemoryConstruction)
{
    auto xml = GivenSimpleMetadataXMLString();
    auto testObject = App::Metadata(std::string {xml});
    AssertMetadataMatches(testObject);
}

// NOLINTEND(readability-named-parameter)
