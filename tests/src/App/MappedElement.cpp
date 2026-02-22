// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileNotice: Part of the FreeCAD project.

#include <gtest/gtest.h>
#include <utility>
#include <map>
#include <string>
#include <vector>


#include "App/IndexedName.h"
#include "App/MappedElement.h"

class MappedElementTest: public ::testing::Test
{
protected:
    // void SetUp() override {}

    // void TearDown() override {}

    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    static Data::MappedElement givenMappedElement(const char* index, const char* name)
    {
        Data::IndexedName indexedName {index};
        Data::MappedName mappedName {name};
        return {indexedName, mappedName};
    }
};

TEST_F(MappedElementTest, constructFromNameAndIndex)
{
    // Arrange
    Data::IndexedName indexedName {"EDGE1"};
    Data::MappedName mappedName {"OTHER_NAME"};

    // Act
    Data::MappedElement mappedElement {indexedName, mappedName};

    // Assert
    EXPECT_EQ(mappedElement.index, indexedName);
    EXPECT_EQ(mappedElement.name, mappedName);
}

TEST_F(MappedElementTest, moveConstructor)
{
    // Arrange
    auto originalMappedElement = givenMappedElement("EDGE1", "OTHER_NAME");
    auto originalName = originalMappedElement.name;
    auto originalIndex = originalMappedElement.index;

    // Act
    Data::MappedElement newMappedElement {std::move(originalMappedElement)};

    // Assert
    EXPECT_EQ(originalName, newMappedElement.name);
    EXPECT_EQ(originalIndex, newMappedElement.index);
}

TEST_F(MappedElementTest, assignmentOperator)
{
    // Arrange
    auto mappedElementA = givenMappedElement("EDGE1", "OTHER_NAME");
    auto mappedElementB = givenMappedElement("EDGE2", "ANOTHER_NAME");
    EXPECT_NE(mappedElementA, mappedElementB);  // Verify test setup

    // Act
    mappedElementA = mappedElementB;

    // Assert
    EXPECT_EQ(mappedElementA, mappedElementB);
}

TEST_F(MappedElementTest, moveAssignmentOperator)
{
    // Arrange
    auto mappedElementA = givenMappedElement("EDGE1", "OTHER_NAME");
    auto mappedElementB = givenMappedElement("EDGE2", "ANOTHER_NAME");
    EXPECT_NE(mappedElementA, mappedElementB);  // Verify test setup

    // Act
    mappedElementA = std::move(mappedElementB);

    // Assert
    EXPECT_EQ(mappedElementA.name.toString(), "ANOTHER_NAME");
    EXPECT_EQ(mappedElementA.index.toString(), "EDGE2");
}

TEST_F(MappedElementTest, equalityOperatorsWhenNotEqual)
{
    // Arrange
    auto mappedElementA = givenMappedElement("EDGE1", "OTHER_NAME");
    auto mappedElementB = givenMappedElement("EDGE2", "ANOTHER_NAME");

    // Act
    bool aAndBAreEqual = mappedElementA == mappedElementB;
    bool aAndBAreNotEqual = mappedElementA != mappedElementB;

    // Assert
    EXPECT_TRUE(aAndBAreNotEqual);
    EXPECT_FALSE(aAndBAreEqual);
}

TEST_F(MappedElementTest, equalityOperatorsWhenEqual)
{
    // Arrange
    auto mappedElementA = givenMappedElement("EDGE1", "OTHER_NAME");
    auto mappedElementB = givenMappedElement("EDGE1", "OTHER_NAME");

    // Act
    bool aAndBAreEqual = mappedElementA == mappedElementB;
    bool aAndBAreNotEqual = mappedElementA != mappedElementB;

    // Assert
    EXPECT_FALSE(aAndBAreNotEqual);
    EXPECT_TRUE(aAndBAreEqual);
}

TEST_F(MappedElementTest, lessThanOperator)
{
    // Index is compared first, then mappedName

    // Arrange
    auto mappedElement1A = givenMappedElement("EDGE1", "A");
    auto mappedElement1B = givenMappedElement("EDGE1", "B");
    auto mappedElement2A = givenMappedElement("EDGE2", "A");
    auto mappedElement2B = givenMappedElement("EDGE2", "B");
    auto mappedElement2BDuplicate = givenMappedElement("EDGE2", "B");

    // Act & Assert
    EXPECT_TRUE(mappedElement1A < mappedElement1B);
    EXPECT_TRUE(mappedElement1A < mappedElement2A);
    EXPECT_TRUE(mappedElement1B < mappedElement2A);
    EXPECT_FALSE(mappedElement2A < mappedElement1B);
    EXPECT_FALSE(mappedElement2B < mappedElement2BDuplicate);
}

TEST_F(MappedElementTest, comparatorBothAreZeroSize)
{
    // Arrange
    Data::MappedName mappedName1 {""};
    Data::MappedName mappedName2 {""};
    auto comp = Data::ElementNameComparator();

    // Act & Assert
    EXPECT_FALSE(comp(mappedName1, mappedName2));
}

TEST_F(MappedElementTest, comparatorOneIsZeroSize)
{
    // Arrange
    Data::MappedName mappedName1 {""};
    Data::MappedName mappedName2 {"#12345"};
    auto comp = Data::ElementNameComparator();

    // Act & Assert
    EXPECT_TRUE(comp(mappedName1, mappedName2));
}

TEST_F(MappedElementTest, comparatorBothStartWithHexDigitsThatDiffer)
{
    // Arrange
    Data::MappedName mappedName1 {"#fed;B"};
    Data::MappedName mappedName2 {"#abcdef;A"};
    auto comp = Data::ElementNameComparator();

    // Act & Assert
    EXPECT_TRUE(comp(mappedName1, mappedName2));
}

TEST_F(MappedElementTest, comparatorBothStartWithTheSameHexDigits)
{
    // Arrange
    Data::MappedName mappedName1 {"#12345;B"};
    Data::MappedName mappedName2 {"#12345;A"};
    auto comp = Data::ElementNameComparator();

    // Act & Assert
    EXPECT_FALSE(comp(mappedName1, mappedName2));
}

TEST_F(MappedElementTest, comparatorHexWithoutTerminator)
{
    // Arrange
    Data::MappedName mappedName1 {"#fed"};
    Data::MappedName mappedName2 {"#abcdef"};
    auto comp = Data::ElementNameComparator();

    // Act & Assert
    EXPECT_TRUE(comp(mappedName1, mappedName2));
}

TEST_F(MappedElementTest, comparatorHexWithoutTerminatorSameLength)
{
    // Arrange
    Data::MappedName mappedName1 {"#fed"};
    Data::MappedName mappedName2 {"#abc"};
    auto comp = Data::ElementNameComparator();

    // Act & Assert
    EXPECT_FALSE(comp(mappedName1, mappedName2));
}

TEST_F(MappedElementTest, comparatorNoHexDigitsLexicalCompare)
{
    // Arrange
    Data::MappedName mappedName1 {"A"};
    Data::MappedName mappedName2 {"B"};
    auto comp = Data::ElementNameComparator();

    // Act & Assert
    EXPECT_TRUE(comp(mappedName1, mappedName2));
}

TEST_F(MappedElementTest, comparatorNoHexDigitsSameStringNumericCompare)
{
    // Arrange
    Data::MappedName mappedName1 {"Edge123456;"};
    Data::MappedName mappedName2 {"Edge321;"};
    auto comp = Data::ElementNameComparator();

    // Act & Assert
    EXPECT_FALSE(comp(mappedName1, mappedName2));
}

TEST_F(MappedElementTest, comparatorIntegerWithoutTerminatorIsBroken)
{
    // Arrange
    Data::MappedName mappedName1 {"Edge123456"};
    Data::MappedName mappedName2 {"Edge321"};
    auto comp = Data::ElementNameComparator();

    // Act & Assert
    EXPECT_FALSE(comp(mappedName1, mappedName2));
}

TEST_F(MappedElementTest, comparatorThreeComplexHexNamesInMap)
{
    // Arrange
    Data::MappedName name1("#19c9:e;:U;FUS;:Hce4:7,E");
    Data::MappedName name2("#1dadb:11;:L#1061a;FUS;:H:d,E");
    Data::MappedName name3("#1dae6:8;:L#1dae4;FUS;:H:d,E");

    std::map<Data::MappedName, int, Data::ElementNameComparator> testMap;
    testMap[name1] = 1;
    testMap[name2] = 2;
    testMap[name3] = 3;

    // Assert: map should have 3 unique keys
    EXPECT_EQ(testMap.size(), 3);

    // Collect keys in order
    std::vector<std::string> keys;
    for (const auto& kv : testMap) {
        keys.push_back(kv.first.toString());
    }

    // Print for debug (optional)
    // for (const auto& k : keys) std::cout << k << std::endl;

    // The expected order depends on your comparator logic.
    // If you want to check the exact order, set it here:
    // (Replace with the correct expected order if needed)
    std::vector<std::string> expectedOrder = {
        "#19c9:e;:U;FUS;:Hce4:7,E",
        "#1dadb:11;:L#1061a;FUS;:H:d,E",
        "#1dae6:8;:L#1dae4;FUS;:H:d,E"
    };

    EXPECT_EQ(keys, expectedOrder);
}

TEST_F(MappedElementTest, comparatorLargerWorkedExampleWithMap)
{
    // Arrange
    Data::MappedName name0("Edge123;:U;FUS;:Hce4:7,E");
    Data::MappedName name1("#1dad:e;:U;FUS;:Hce4:7,E");
    Data::MappedName name2("#1dadb:11;:L#1061a;FUS;:H:d,E");
    Data::MappedName name3("#1dae6:8;:L#1dae4;FUS;:H:d,E");
    Data::MappedName name4("Edge999;;:L#1dae4;FUS;:H:d,E");
    Data::MappedName name5("g4v2;SKT;:H1234,F;:H5678:2,E;:G0(g1;SKT;:H9012,E);XTR;:H3456:2,F");


    std::map<Data::MappedName, int, Data::ElementNameComparator> testMap;
    testMap[name0] = 1;
    testMap[name1] = 2;
    testMap[name2] = 3;
    testMap[name3] = 4;
    testMap[name0] = 5;   // Duplicate, should not affect size
    testMap[name1] = 6;   // Duplicate, should not affect size
    testMap[name4] = 7;   // New entry
    testMap[name4] = 8;   // Duplicate, should not affect size
    testMap[name2] = 9;   // Duplicate, should not affect size
    testMap[name3] = 10;  // Duplicate, should not affect size
    testMap[name5] = 11;

    // Assert: map should have 5 unique keys
    EXPECT_EQ(testMap.size(), 6);

    // Collect keys in order
    std::vector<std::string> keys;
    for (const auto& kv : testMap) {
        keys.push_back(kv.first.toString());
    }

    // Print for debug (optional)
    // for (const auto& k : keys) std::cout << k << std::endl;

    // The expected order depends on your comparator logic.
    // If you want to check the exact order, set it here:
    // (Replace with the correct expected order if needed)
    std::vector<std::string> expectedOrder = {
        "Edge123;:U;FUS;:Hce4:7,E",
        "Edge999;;:L#1dae4;FUS;:H:d,E",
        "g4v2;SKT;:H1234,F;:H5678:2,E;:G0(g1;SKT;:H9012,E);XTR;:H3456:2,F",
        "#1dad:e;:U;FUS;:Hce4:7,E",
        "#1dadb:11;:L#1061a;FUS;:H:d,E",
        "#1dae6:8;:L#1dae4;FUS;:H:d,E"
    };

    EXPECT_EQ(keys, expectedOrder);
}
