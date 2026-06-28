#include <gtest/gtest.h>

#include <Base/Uuid.h>

#include <regex>
#include <string>
#include <unordered_set>

TEST(Uuid, CreateUuidFormatAndBits)
{
    const std::string uuid = Base::Uuid::createUuid();

    // Canonical RFC4122: 8-4-4-4-12 lowercase hex.
    static const std::regex re("^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$");
    EXPECT_TRUE(std::regex_match(uuid, re));

    // Version nibble is at index 14 (xxxxxxxx-xxxx-Mxxx-....)
    ASSERT_GE(uuid.size(), static_cast<std::size_t>(15));
    EXPECT_EQ(uuid[14], '4');

    // Variant nibble is at index 19 (....-Nxxx-....) => 8,9,a,b
    ASSERT_GE(uuid.size(), static_cast<std::size_t>(20));
    EXPECT_TRUE(uuid[19] == '8' || uuid[19] == '9' || uuid[19] == 'a' || uuid[19] == 'b');
}

TEST(Uuid, SetValueAcceptsBracesAndNormalizes)
{
    const std::string input = "{550E8400-E29B-41D4-A716-446655440000}";

    Base::Uuid id;
    id.setValue(input.c_str());

    EXPECT_EQ(id.getValue(), "550e8400-e29b-41d4-a716-446655440000");
}

TEST(Uuid, SetValueRejectsInvalidStrings)
{
    Base::Uuid id;

    EXPECT_THROW(id.setValue(""), std::runtime_error);
    EXPECT_THROW(id.setValue("not-a-uuid"), std::runtime_error);
    EXPECT_THROW(id.setValue("550e8400e29b41d4a716446655440000"), std::runtime_error);  // no dashes
    EXPECT_THROW(id.setValue("{550e8400-e29b-41d4-a716-446655440000"), std::runtime_error);  // missing }
    EXPECT_THROW(id.setValue("550e8400-e29b-41d4-a716-44665544000z"), std::runtime_error);  // bad hex
}

TEST(Uuid, CreateUuidUniquenessSmoke)
{
    std::unordered_set<std::string> ids;
    ids.reserve(512);

    for (int i = 0; i < 512; ++i) {
        ids.insert(Base::Uuid::createUuid());
    }

    EXPECT_EQ(ids.size(), 512u);
}
