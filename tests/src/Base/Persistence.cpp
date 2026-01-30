#include <gtest/gtest.h>

#include <Base/Persistence.h>

#include <string>

TEST(Persistence, ValidateXmlStringReplacesDisallowed)
{
    // U+0001 is not allowed in XML 1.0.
    std::string s = "A";
    s.push_back(static_cast<char>(0x01));
    s += "B";

    const std::string out = Base::Persistence::validateXMLString(s);
    EXPECT_EQ(out, "A_B");
    EXPECT_EQ(out.find('\0'), std::string::npos);
}

TEST(Persistence, ValidateXmlStringPreservesValidNonBmp)
{
    // U+1F600 (grinning face) is valid XML 1.0 (>= 0x10000).
    const std::string s = "x\xf0\x9f\x98\x80y";
    const std::string out = Base::Persistence::validateXMLString(s);
    EXPECT_EQ(out, s);
    EXPECT_EQ(out.find('\0'), std::string::npos);
}

TEST(Persistence, ValidateXmlStringReplacesDiscouraged)
{
    // U+FDD0 is in the discouraged range.
    const std::string s = "x\xef\xb7\x90y";  // UTF-8 for U+FDD0
    const std::string out = Base::Persistence::validateXMLString(s);
    EXPECT_EQ(out, "x_y");
}
