// SPDX-License-Identifier: LGPL-2.1-or-later

#include "gtest/gtest.h"

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
