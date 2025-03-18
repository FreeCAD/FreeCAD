// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>
#include <Mod/TechDraw/App/GeometryId.h>

#include <iostream>
#include <array>
// #include "src/App/InitApplication.h"


class GeometryIdTest: public ::testing::Test
{
};

TEST_F(GeometryIdTest, TestWithValidIds)
{
    std::array<const char*, 6> validIds {"Vertex1",
                                         "Edge12",
                                         "Face165",
                                         "Vertex193429932",
                                         "Face42",
                                         "Edge5"};
    for (const char* id : validIds) {
        EXPECT_NO_THROW(TechDraw::GeometryId {id});
    }

    std::array<std::string, 6> validIds2 {"Vertex1",
                                          "Edge12",
                                          "Face165",
                                          "Vertex193429932",
                                          "Face42",
                                          "Edge5"};
    for (const std::string& id : validIds2) {
        EXPECT_NO_THROW(TechDraw::GeometryId {id});
    }
}

TEST_F(GeometryIdTest, TestWithInvalidIds)
{
    std::array<const char*, 9> invalidIds {
        "Vertex-1",    // Invalid number
        "Edge12-",     // Invalid number
        "edge13",      // Invalid type
        "Facsade165",  // Invalid type
        "Vertex",      // Missing number
        "135",         // Missing type
        "Faceasd12",   // Invalid, but contains valid type and number
        "Edge 1",      // Invalid, but contains valid type and number
        "Edge11 "      // Invalid, but contains valid type and number
    };
    for (const char* id : invalidIds) {
        EXPECT_ANY_THROW(TechDraw::GeometryId {id});
    }

    std::array<std::string, 9> invalidIds2 {
        "Vertex-1",    // Invalid number
        "Edge12-",     // Invalid number
        "edge13",      // Invalid type
        "Facsade165",  // Invalid type
        "Vertex",      // Missing number
        "135",         // Missing type
        "Faceasd12",   // Invalid, but contains valid type and number
        "Edge 1",      // Invalid, but contains valid type and number
        "Edge11 "      // Invalid, but contains valid type and number
    };
    for (const std::string& id : invalidIds2) {
        EXPECT_ANY_THROW(TechDraw::GeometryId {id});
    }
}

TEST_F(GeometryIdTest, TestParsedIds)
{
    EXPECT_EQ(TechDraw::GeometryId {"Vertex1"}.id, 1);
    EXPECT_EQ(TechDraw::GeometryId {"Vertex523"}.id, 523);
    EXPECT_EQ(TechDraw::GeometryId {"Vertex0"}.id, 0);
    EXPECT_EQ(TechDraw::GeometryId {"Edge1"}.id, 1);
    EXPECT_EQ(TechDraw::GeometryId {"Edge523"}.id, 523);
    EXPECT_EQ(TechDraw::GeometryId {"Edge0"}.id, 0);
    EXPECT_EQ(TechDraw::GeometryId {"Face1"}.id, 1);
    EXPECT_EQ(TechDraw::GeometryId {"Face523"}.id, 523);
    EXPECT_EQ(TechDraw::GeometryId {"Face0"}.id, 0);
}
