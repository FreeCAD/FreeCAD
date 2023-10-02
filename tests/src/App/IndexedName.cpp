// SPDX-License-Identifier: LGPL-2.1-or-later

#include "gtest/gtest.h"

#include "App/IndexedName.h"

#include <sstream>

// NOLINTBEGIN(readability-magic-numbers)

class IndexedNameTest: public ::testing::Test
{
protected:
    // void SetUp() override {}

    // void TearDown() override {}

    // Create and return a list of invalid IndexedNames
    static std::vector<Data::IndexedName> givenInvalidIndexedNames()
    {
        return std::vector<Data::IndexedName> {Data::IndexedName(),
                                               Data::IndexedName("", 1),
                                               Data::IndexedName("INVALID42NAME", 1),
                                               Data::IndexedName(".EDGE", 1)};
    }

    // Create and return a list of valid IndexedNames
    static std::vector<Data::IndexedName> givenValidIndexedNames()
    {
        return std::vector<Data::IndexedName> {Data::IndexedName("NAME"),
                                               Data::IndexedName("NAME1"),
                                               Data::IndexedName("NAME", 1),
                                               Data::IndexedName("NAME_WITH_UNDERSCORES12345")};
    }

    // An arbitrary list of C strings used for testing some types of construction
    // NOLINTNEXTLINE cppcoreguidelines-non-private-member-variables-in-classes
    std::vector<const char*> allowedTypes {"VERTEX", "EDGE", "FACE", "WIRE"};
};

TEST_F(IndexedNameTest, defaultConstruction)
{
    // Act
    auto indexedName = Data::IndexedName();

    // Assert
    EXPECT_STREQ(indexedName.getType(), "");
    EXPECT_EQ(indexedName.getIndex(), 0);
}

TEST_F(IndexedNameTest, nameOnlyConstruction)
{
    // Act
    auto indexedName = Data::IndexedName("TestName");

    // Assert
    EXPECT_STREQ(indexedName.getType(), "TestName");
    EXPECT_EQ(indexedName.getIndex(), 0);
}

TEST_F(IndexedNameTest, nameAndIndexConstruction)
{
    // Arrange
    const int testIndex {42};

    // Act
    auto indexedName = Data::IndexedName("TestName", testIndex);

    // Assert
    EXPECT_STREQ(indexedName.getType(), "TestName");
    EXPECT_EQ(indexedName.getIndex(), testIndex);
}

TEST_F(IndexedNameTest, nameAndIndexConstructionWithOverride)
{
    // Arrange
    const int testIndex {42};

    // Act
    auto indexedName = Data::IndexedName("TestName17", testIndex);

    // Assert
    EXPECT_STREQ(indexedName.getType(), "TestName");
    EXPECT_EQ(indexedName.getIndex(), testIndex);
}

// Names must only contain ASCII letters and underscores (but may end with a number)
TEST_F(IndexedNameTest, constructionInvalidCharInName)
{
    // Arrange
    constexpr int lastASCIICode {127};
    std::vector<char> illegalCharacters = {};
    for (int code = 1; code <= lastASCIICode; ++code) {
        if ((std::isalnum(code) == 0) && code != '_') {
            illegalCharacters.push_back(char(code));
        }
    }
    for (auto illegalChar : illegalCharacters) {
        std::string testName {"TestName"};
        testName += illegalChar;

        // Act
        auto indexedName = Data::IndexedName(testName.c_str(), 1);

        // Assert
        EXPECT_STREQ(indexedName.getType(), "") << "Expected empty name when given " << testName;
    }
}

// Names must not contain numbers in the middle:
TEST_F(IndexedNameTest, constructionNumberInName)
{
    // Arrange
    const int testIndex {42};
    std::string testName;
    testName += "Test" + std::to_string(testIndex) + "Name";

    // Act
    auto indexedName = Data::IndexedName(testName.c_str(), testIndex);

    // Assert
    EXPECT_STREQ(indexedName.getType(), "");
}

TEST_F(IndexedNameTest, nameAndTypeListConstructionWithoutAllowOthers)
{
    // Act
    auto indexedName = Data::IndexedName("EDGE19", allowedTypes, false);

    // Assert
    EXPECT_STREQ(indexedName.getType(), "EDGE");
    EXPECT_EQ(indexedName.getIndex(), 19);

    // Act
    indexedName = Data::IndexedName("EDGES_ARE_REALLY_GREAT19", allowedTypes, false);

    // Assert
    EXPECT_STREQ(indexedName.getType(), "");
    EXPECT_EQ(indexedName.getIndex(), 19);

    // Act
    indexedName = Data::IndexedName("NOT_IN_THE_LIST42", allowedTypes, false);

    // Assert
    EXPECT_STREQ(indexedName.getType(), "");
}

TEST_F(IndexedNameTest, nameAndTypeListConstructionWithAllowOthers)
{
    // Act
    auto indexedName = Data::IndexedName("NOT_IN_THE_LIST42", allowedTypes, true);

    // Assert
    EXPECT_STREQ(indexedName.getType(), "NOT_IN_THE_LIST");
    EXPECT_EQ(indexedName.getIndex(), 42);
}

// Check that the same memory location is used for two names that are not in the allowedTypes list
TEST_F(IndexedNameTest, nameAndTypeListConstructionReusedMemoryCheck)
{
    // Arrange
    auto indexedName1 = Data::IndexedName("NOT_IN_THE_LIST42", allowedTypes, true);
    auto indexedName2 = Data::IndexedName("NOT_IN_THE_LIST43", allowedTypes, true);

    // Act & Assert
    EXPECT_EQ(indexedName1.getType(), indexedName2.getType());
}

TEST_F(IndexedNameTest, byteArrayConstruction)
{
    // Arrange
    QByteArray qba {"EDGE42"};

    // Act
    auto indexedName = Data::IndexedName(qba);

    // Assert
    EXPECT_STREQ(indexedName.getType(), "EDGE");
    EXPECT_EQ(indexedName.getIndex(), 42);
}

TEST_F(IndexedNameTest, copyConstruction)
{
    // Arrange
    auto indexedName = Data::IndexedName("EDGE42");

    // Act
    auto indexedNameCopy {indexedName};

    // Assert
    EXPECT_EQ(indexedName, indexedNameCopy);
}

TEST_F(IndexedNameTest, streamInsertionOperator)
{
    // Arrange
    auto indexedName = Data::IndexedName("EDGE42");
    std::stringstream ss;

    // Act
    ss << indexedName;

    // Assert
    EXPECT_EQ(ss.str(), std::string {"EDGE42"});
}

TEST_F(IndexedNameTest, compoundAssignmentOperator)
{
    // NOTE: Only += is defined for this class

    // Arrange
    constexpr int base {42};
    constexpr int offset {10};
    auto indexedName = Data::IndexedName("EDGE", base);

    // Act
    indexedName += offset;

    // Assert
    EXPECT_EQ(indexedName.getIndex(), 52);
}

TEST_F(IndexedNameTest, preincrementOperator)
{
    // Arrange
    auto indexedName = Data::IndexedName("EDGE42");

    // Act
    ++indexedName;

    // Assert
    EXPECT_EQ(indexedName.getIndex(), 43);
}

TEST_F(IndexedNameTest, predecrementOperator)
{
    // Arrange
    auto indexedName = Data::IndexedName("EDGE42");

    // Act
    --indexedName;

    // Assert
    EXPECT_EQ(indexedName.getIndex(), 41);
}

TEST_F(IndexedNameTest, comparisonOperators)
{
    // Arrange
    auto indexedName1 = Data::IndexedName("EDGE42");
    auto indexedName2 = Data::IndexedName("EDGE42");

    // Act & Assert
    EXPECT_EQ(indexedName1.compare(indexedName2), 0);
    EXPECT_TRUE(indexedName1 == indexedName2);
    EXPECT_FALSE(indexedName1 != indexedName2);
    EXPECT_FALSE(indexedName1 < indexedName2);

    // Arrange
    auto indexedName3 = Data::IndexedName("EDGE42");
    auto indexedName4 = Data::IndexedName("FACE42");

    // Act & Assert
    EXPECT_LT(indexedName3.compare(indexedName4), 0);
    EXPECT_FALSE(indexedName3 == indexedName4);
    EXPECT_TRUE(indexedName3 != indexedName4);
    EXPECT_TRUE(indexedName3 < indexedName4);

    // Arrange
    auto indexedName5 = Data::IndexedName("FACE42");
    auto indexedName6 = Data::IndexedName("EDGE42");

    // Act & Assert
    EXPECT_GT(indexedName5.compare(indexedName6), 0);
    EXPECT_FALSE(indexedName5 == indexedName6);
    EXPECT_TRUE(indexedName5 != indexedName6);
    EXPECT_FALSE(indexedName5 < indexedName6);

    // Arrange
    auto indexedName7 = Data::IndexedName("EDGE41");
    auto indexedName8 = Data::IndexedName("EDGE42");

    // Act & Assert
    EXPECT_LT(indexedName7.compare(indexedName8), 0);
    EXPECT_FALSE(indexedName7 == indexedName8);
    EXPECT_TRUE(indexedName7 != indexedName8);
    EXPECT_TRUE(indexedName7 < indexedName8);

    // Arrange
    auto indexedName9 = Data::IndexedName("EDGE43");
    auto indexedName10 = Data::IndexedName("EDGE42");

    // Act & Assert
    EXPECT_GT(indexedName9.compare(indexedName10), 0);
    EXPECT_FALSE(indexedName9 == indexedName10);
    EXPECT_TRUE(indexedName9 != indexedName10);
    EXPECT_FALSE(indexedName9 < indexedName10);

    // Arrange
    auto indexedName11 = Data::IndexedName("EDGE2");
    auto indexedName12 = Data::IndexedName("EDGE12");

    // Act & Assert
    EXPECT_LT(indexedName11.compare(indexedName12), 0);
    EXPECT_FALSE(indexedName11 == indexedName12);
    EXPECT_TRUE(indexedName11 != indexedName12);
    EXPECT_TRUE(indexedName11 < indexedName12);
}

TEST_F(IndexedNameTest, subscriptOperator)
{
    // Arrange
    auto indexedName = Data::IndexedName("EDGE42");

    // Act & Assert
    EXPECT_EQ(indexedName[0], 'E');
    EXPECT_EQ(indexedName[1], 'D');
    EXPECT_EQ(indexedName[2], 'G');
    EXPECT_EQ(indexedName[3], 'E');
}

TEST_F(IndexedNameTest, getType)
{
    // Arrange
    auto indexedName = Data::IndexedName("EDGE42");

    // Act & Assert
    EXPECT_STREQ(indexedName.getType(), "EDGE");
}

TEST_F(IndexedNameTest, setIndex)
{
    // Arrange
    auto indexedName = Data::IndexedName("EDGE42");
    EXPECT_EQ(indexedName.getIndex(), 42);

    // Act
    indexedName.setIndex(1);

    // Assert
    EXPECT_EQ(indexedName.getIndex(), 1);
}

TEST_F(IndexedNameTest, isNullTrue)
{
    // Arrange
    auto invalidNames = givenInvalidIndexedNames();
    for (const auto& name : invalidNames) {

        // Act & Assert
        EXPECT_TRUE(name.isNull());
    }
}

TEST_F(IndexedNameTest, isNullFalse)
{
    // Arrange
    auto validNames = givenValidIndexedNames();
    for (const auto& name : validNames) {

        // Act & Assert
        EXPECT_FALSE(name.isNull());
    }
}

TEST_F(IndexedNameTest, booleanConversionFalse)
{
    // Arrange
    auto invalidNames = givenInvalidIndexedNames();
    for (const auto& name : invalidNames) {

        // Act & Assert
        EXPECT_FALSE(static_cast<bool>(name));
    }

    // Usage example:
    auto indexedName = Data::IndexedName(".EDGE", 1);  // Invalid name
    if (indexedName) {
        FAIL() << "indexedName as a boolean should have been false for an invalid name";
    }
}

TEST_F(IndexedNameTest, booleanConversionTrue)
{
    // Arrange
    auto validNames = givenValidIndexedNames();
    for (const auto& name : validNames) {

        // Act & Assert
        EXPECT_TRUE(static_cast<bool>(name));
    }
}

TEST_F(IndexedNameTest, fromConst)
{
    // Arrange
    const int testIndex {42};

    // Act
    auto indexedName = Data::IndexedName::fromConst("TestName", testIndex);

    // Assert
    EXPECT_STREQ(indexedName.getType(), "TestName");
    EXPECT_EQ(indexedName.getIndex(), testIndex);
}

TEST_F(IndexedNameTest, appendToStringBufferEmptyBuffer)
{
    // Arrange
    std::string bufferStartedEmpty;
    Data::IndexedName testName("TEST_NAME", 1);

    // Act
    testName.appendToStringBuffer(bufferStartedEmpty);

    // Assert
    EXPECT_EQ(bufferStartedEmpty, "TEST_NAME1");
}

TEST_F(IndexedNameTest, appendToStringBufferNonEmptyBuffer)
{
    // Arrange
    std::string bufferWithData {"DATA"};
    Data::IndexedName testName("TEST_NAME", 1);

    // Act
    testName.appendToStringBuffer(bufferWithData);

    // Assert
    EXPECT_EQ(bufferWithData, "DATATEST_NAME1");
}

TEST_F(IndexedNameTest, appendToStringBufferZeroIndex)
{
    // Arrange
    std::string bufferStartedEmpty;
    Data::IndexedName testName("TEST_NAME", 0);

    // Act
    testName.appendToStringBuffer(bufferStartedEmpty);

    // Assert
    EXPECT_EQ(bufferStartedEmpty, "TEST_NAME");
}

TEST_F(IndexedNameTest, toString)
{
    // Arrange
    Data::IndexedName testName("TEST_NAME", 1);

    // Act
    auto result = testName.toString();

    // Assert
    EXPECT_EQ(result, "TEST_NAME1");
}

TEST_F(IndexedNameTest, toStringNoIndex)
{
    // Arrange
    Data::IndexedName testName("TEST_NAME", 0);

    // Act
    auto result = testName.toString();

    // Assert
    EXPECT_EQ(result, "TEST_NAME");
}

TEST_F(IndexedNameTest, assignmentOperator)
{
    // Arrange
    const int testIndex1 {42};
    const int testIndex2 {24};
    auto indexedName1 = Data::IndexedName::fromConst("TestName", testIndex1);
    auto indexedName2 = Data::IndexedName::fromConst("TestName2", testIndex2);
    EXPECT_NE(indexedName1, indexedName2);  // Ensure the test is set up correctly

    // Act
    indexedName1 = indexedName2;

    // Assert
    EXPECT_EQ(indexedName1, indexedName2);
}


class ByteArrayTest: public ::testing::Test
{
protected:
    // void SetUp() override {}

    // void TearDown() override {}
};

TEST_F(ByteArrayTest, QByteArrayConstruction)
{
    // Arrange
    QByteArray testQBA("Data in a QByteArray");

    // Act
    Data::ByteArray testByteArray(testQBA);

    // Assert
    EXPECT_EQ(testQBA, testByteArray.bytes);
}

TEST_F(ByteArrayTest, CopyConstruction)
{
    // Arrange
    QByteArray testQBA("Data in a QByteArray");
    Data::ByteArray originalByteArray(testQBA);

    // Act
    // NOLINTNEXTLINE performance-unnecessary-copy-initialization
    Data::ByteArray copiedByteArray(originalByteArray);

    // Assert
    EXPECT_EQ(originalByteArray, copiedByteArray);
}

TEST_F(ByteArrayTest, MoveConstruction)
{
    // Arrange
    QByteArray testQBA("Data in a QByteArray");
    Data::ByteArray originalByteArray(testQBA);
    const auto* originalDataLocation = originalByteArray.bytes.constData();

    // Act
    Data::ByteArray copiedByteArray(std::move(originalByteArray));

    // Assert
    EXPECT_EQ(testQBA, copiedByteArray.bytes);
    EXPECT_EQ(originalDataLocation, copiedByteArray.bytes.constData());
}

TEST_F(ByteArrayTest, ensureUnshared)
{
    // Arrange
    QByteArray testQBA("Data in a QByteArray");
    Data::ByteArray originalByteArray(testQBA);
    const auto* originalDataLocation = originalByteArray.bytes.constData();
    Data::ByteArray copiedByteArray(originalByteArray);

    // Act
    copiedByteArray.ensureUnshared();

    // Assert
    EXPECT_EQ(testQBA, copiedByteArray.bytes);
    EXPECT_NE(originalDataLocation, copiedByteArray.bytes.constData());
}

TEST_F(ByteArrayTest, equalityOperator)
{
    // Arrange
    QByteArray testQBA1("Data in a QByteArray");
    QByteArray testQBA2("Data in a QByteArray");
    QByteArray testQBA3("Not the same data in a QByteArray");
    Data::ByteArray byteArray1(testQBA1);
    Data::ByteArray byteArray2(testQBA2);
    Data::ByteArray byteArray3(testQBA3);

    // Act & Assert
    EXPECT_TRUE(byteArray1 == byteArray2);
    EXPECT_FALSE(byteArray1 == byteArray3);
}

TEST_F(ByteArrayTest, assignmentOperator)
{
    // Arrange
    QByteArray testQBA1("Data in a QByteArray");
    QByteArray testQBA2("Different data in a QByteArray");
    Data::ByteArray originalByteArray(testQBA1);
    Data::ByteArray newByteArray(testQBA2);
    ASSERT_FALSE(originalByteArray == newByteArray);

    // Act
    newByteArray = originalByteArray;

    // Assert
    EXPECT_TRUE(originalByteArray == newByteArray);
}

TEST_F(ByteArrayTest, moveAssignmentOperator)
{
    // Arrange
    QByteArray testQBA1("Data in a QByteArray");
    QByteArray testQBA2("Different data in a QByteArray");
    Data::ByteArray originalByteArray(testQBA1);
    const auto* originalByteArrayLocation = originalByteArray.bytes.constData();
    Data::ByteArray newByteArray(testQBA2);
    ASSERT_FALSE(originalByteArray == newByteArray);

    // Act
    newByteArray = std::move(originalByteArray);

    // Assert
    EXPECT_EQ(originalByteArrayLocation, newByteArray.bytes.constData());
}

// NOLINTEND(readability-magic-numbers)
