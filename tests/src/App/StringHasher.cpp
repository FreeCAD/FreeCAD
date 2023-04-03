// SPDX-License-Identifier: LGPL-2.1-or-later

#include "gtest/gtest.h"

#include <App/StringHasher.h>
#include <App/StringIDPy.h>

#include <QCryptographicHash>

class StringIDTest: public ::testing::Test
{
protected:
    // void SetUp() override {}
    // void TearDown() override {}

    static App::StringID givenFlaggedStringID(App::StringID::Flag flag)
    {
        const long value {42};
        const QByteArray data {"data", 4};
        return App::StringID {value, data, flag};
    }
};

TEST_F(StringIDTest, stringIDManualConstructionNoFlags)
{
    // Arrange
    const long expectedValue {42};
    const QByteArray expectedData {"data", 4};

    // Act
    auto id = App::StringID(expectedValue, expectedData);

    // Assert
    EXPECT_EQ(expectedValue, id.value());
    EXPECT_EQ(expectedData, id.data());
    EXPECT_FALSE(id.isBinary());
}

TEST_F(StringIDTest, stringIDManualConstructionWithFlag)
{
    // Arrange
    const long expectedValue {42};
    const QByteArray expectedData {"data", 4};
    const App::StringID::Flags expectedFlags {App::StringID::Flag::Binary};

    // Act
    auto id = App::StringID(expectedValue, expectedData, expectedFlags);

    // Assert
    EXPECT_EQ(expectedValue, id.value());
    EXPECT_EQ(expectedData, id.data());
    EXPECT_TRUE(id.isBinary());
}

TEST_F(StringIDTest, stringIDDefaultConstruction)
{
    // Arrange & Act
    auto id = App::StringID();

    // Assert
    EXPECT_EQ(0, id.value());
}

TEST_F(StringIDTest, value)
{
    // Arrange
    const long expectedValueA {0};
    auto idA = App::StringID(expectedValueA, nullptr);
    const long expectedValueB {42};
    auto idB = App::StringID(expectedValueB, nullptr);
    const long expectedValueC {314159};
    auto idC = App::StringID(expectedValueC, nullptr);

    // Act
    auto valueA = idA.value();
    auto valueB = idB.value();
    auto valueC = idC.value();

    // Assert
    EXPECT_EQ(expectedValueA, valueA);
    EXPECT_EQ(expectedValueB, valueB);
    EXPECT_EQ(expectedValueC, valueC);
}

TEST_F(StringIDTest, relatedIDs)
{
    // Nothing to test -- relatedIDs are storage-only in this class
}

TEST_F(StringIDTest, isBinary)
{
    // Arrange
    auto flaggedID = givenFlaggedStringID(App::StringID::Flag::Binary);
    auto controlID = App::StringID {};

    // Act & Assert
    EXPECT_TRUE(flaggedID.isBinary());
    EXPECT_FALSE(controlID.isBinary());
}

TEST_F(StringIDTest, isHashed)
{
    // Arrange
    auto flaggedID = givenFlaggedStringID(App::StringID::Flag::Hashed);
    auto controlID = App::StringID {};

    // Act & Assert
    EXPECT_TRUE(flaggedID.isHashed());
    EXPECT_FALSE(controlID.isHashed());
}

TEST_F(StringIDTest, isPostfixed)
{
    // Arrange
    auto flaggedID = givenFlaggedStringID(App::StringID::Flag::Postfixed);
    auto controlID = App::StringID {};

    // Act & Assert
    EXPECT_TRUE(flaggedID.isPostfixed());
    EXPECT_FALSE(controlID.isPostfixed());
}

TEST_F(StringIDTest, isPostfixEncoded)
{
    // Arrange
    auto flaggedID = givenFlaggedStringID(App::StringID::Flag::PostfixEncoded);
    auto controlID = App::StringID {};

    // Act & Assert
    EXPECT_TRUE(flaggedID.isPostfixEncoded());
    EXPECT_FALSE(controlID.isPostfixEncoded());
}

TEST_F(StringIDTest, isIndexed)
{
    // Arrange
    auto flaggedID = givenFlaggedStringID(App::StringID::Flag::Indexed);
    auto controlID = App::StringID {};

    // Act & Assert
    EXPECT_TRUE(flaggedID.isIndexed());
    EXPECT_FALSE(controlID.isIndexed());
}

TEST_F(StringIDTest, isPrefixID)
{
    // Arrange
    auto flaggedID = givenFlaggedStringID(App::StringID::Flag::PrefixID);
    auto controlID = App::StringID {};

    // Act & Assert
    EXPECT_TRUE(flaggedID.isPrefixID());
    EXPECT_FALSE(controlID.isPrefixID());
}

TEST_F(StringIDTest, isPrefixIDIndex)
{
    // Arrange
    auto flaggedID = givenFlaggedStringID(App::StringID::Flag::PrefixIDIndex);
    auto controlID = App::StringID {};

    // Act & Assert
    EXPECT_TRUE(flaggedID.isPrefixIDIndex());
    EXPECT_FALSE(controlID.isPrefixIDIndex());
}

TEST_F(StringIDTest, isMarked)
{
    // Arrange
    auto flaggedID = givenFlaggedStringID(App::StringID::Flag::Marked);
    auto controlID = App::StringID {};

    // Act & Assert
    EXPECT_TRUE(flaggedID.isMarked());
    EXPECT_FALSE(controlID.isMarked());
}

TEST_F(StringIDTest, isPersistent)
{
    // Arrange
    auto flaggedID = givenFlaggedStringID(App::StringID::Flag::Persistent);
    auto controlID = App::StringID {};

    // Act & Assert
    EXPECT_TRUE(flaggedID.isPersistent());
    EXPECT_FALSE(controlID.isPersistent());
}

TEST_F(StringIDTest, isFromSameHasher)
{
    // Nothing to test except when used by StringHasher
}

TEST_F(StringIDTest, getHasher)
{
    // Nothing to test except when used by StringHasher
}

TEST_F(StringIDTest, data)
{
    // Arrange
    QByteArray expectedData {"data", 4};
    auto id = App::StringID(1, expectedData);

    // Act
    auto data = id.data();

    // Assert
    EXPECT_EQ(expectedData, data);
}

TEST_F(StringIDTest, postfix)
{
    // Nothing to test except when used by StringHasher
}

TEST_F(StringIDTest, getPyObject)
{
    // Arrange
    Py_Initialize();
    auto id = new App::StringID(1, nullptr);
    id->ref();

    // Act
    Py::Object py(id->getPyObject(), true);
    id->unref();

    // Assert
    EXPECT_TRUE(PyObject_TypeCheck(py.ptr(), &App::StringIDPy::Type));
}

TEST_F(StringIDTest, getPyObjectWithIndex)
{
    // Arrange
    Py_Initialize();
    auto id = new App::StringID(1, nullptr);
    id->ref();

    // Act
    Py::Object py(id->getPyObjectWithIndex(2), true);
    id->unref();

    // Assert
    ASSERT_TRUE(PyObject_TypeCheck(py.ptr(), &App::StringIDPy::Type));
}

TEST_F(StringIDTest, toStringWithoutIndex)
{
    // Arrange
    const long bigHex = 0xfcad10;
    auto idA = App::StringID(1, QByteArray {"data", 4});
    auto idB = App::StringID(bigHex, QByteArray {"data", 4});

    // Act
    auto resultA = idA.toString();
    auto resultB = idB.toString();

    // Assert
    EXPECT_EQ(std::string("#1"), resultA);
    EXPECT_EQ(std::string("#fcad10"), resultB);// Make sure result is in hex
}

TEST_F(StringIDTest, toStringWithIndex)
{
    // Arrange
    const long bigHex = 0xfcad10;
    auto id = App::StringID(1, QByteArray {"data", 4});

    // Act
    auto resultA = id.toString(bigHex);
    auto resultB = id.toString(0);

    // Assert
    EXPECT_EQ(std::string("#1:fcad10"), resultA);
    EXPECT_EQ(std::string("#1"), resultB);
}

TEST_F(StringIDTest, fromStringWithEOFAndLengthGood)
{
    // Arrange
    const std::string testString {"#1:fcad"};

    // Act
    auto result =
        App::StringID::fromString(testString.c_str(), true, static_cast<int>(testString.length()));

    // Assert
    EXPECT_EQ(result.id, 1);
    EXPECT_EQ(result.index, 0xfcad);
}

TEST_F(StringIDTest, fromStringExtraData)
{
    // Arrange
    const std::string testString {"#1:fcad#2:bad"};

    // Act
    auto trueResult =
        App::StringID::fromString(testString.c_str(), true, static_cast<int>(testString.length()));
    auto falseResult =
        App::StringID::fromString(testString.c_str(), false, static_cast<int>(testString.length()));

    // Assert
    EXPECT_EQ(trueResult.id, -1);
    EXPECT_EQ(falseResult.id, 1);
}

TEST_F(StringIDTest, fromStringLengthUnspecified)
{
    // Arrange
    const std::string testString {"#1:fcad#2:bad"};

    // Act
    auto trueResult = App::StringID::fromString(testString.c_str(), true);
    auto falseResult = App::StringID::fromString(testString.c_str(), false);

    // Assert
    EXPECT_EQ(trueResult.id, -1);
    EXPECT_EQ(falseResult.id, 1);
}

TEST_F(StringIDTest, fromStringShorterLength)
{
    // Arrange
    const int dataLength {7};
    const std::string testString {"#1:fcad#2:bad"};

    // Act
    auto trueResult = App::StringID::fromString(testString.c_str(), true, dataLength);
    auto falseResult = App::StringID::fromString(testString.c_str(), false, dataLength);

    // Assert
    EXPECT_EQ(trueResult.id, 1);
    EXPECT_EQ(falseResult.id, 1);
}

TEST_F(StringIDTest, fromStringNoHashtag)
{
    // Arrange
    const std::string testString {"1:fcad"};

    // Act
    auto result = App::StringID::fromString(testString.c_str(), true);

    // Assert
    EXPECT_EQ(result.id, -1);
}

TEST_F(StringIDTest, fromStringNotHex)
{
    // Arrange
    const std::string testStringA {"1:freecad"};
    const std::string testStringB {"zoink:2"};

    // Act
    auto resultA = App::StringID::fromString(testStringA.c_str(), false);
    auto resultB = App::StringID::fromString(testStringB.c_str(), false);

    // Assert
    EXPECT_EQ(resultA.id, -1);
    EXPECT_EQ(resultB.id, -1);
}

TEST_F(StringIDTest, fromStringQByteArray)
{
    // Arrange
    const QByteArray testString {"#1:fcad", 7};

    // Act
    auto result = App::StringID::fromString(testString, true);

    // Assert
    EXPECT_EQ(result.id, 1);
    EXPECT_EQ(result.index, 0xfcad);
}

TEST_F(StringIDTest, dataToTextHashed)
{
    // Arrange
    QByteArray buffer {"120ca87015d849dbea060eaf2295fcc4ee981427", 40};  // NOLINT
    auto id = App::StringID(1, buffer, App::StringID::Flag::Hashed);

    // Act
    auto result = id.dataToText(0);

    // Assert
    EXPECT_EQ(result, buffer.toBase64().constData());
}

TEST_F(StringIDTest, dataToTextBinary)
{
    // Arrange
    QByteArray buffer {"120ca87015d849dbea060eaf2295fcc4ee981427", 40};  // NOLINT
    auto id = App::StringID(1, buffer, App::StringID::Flag::Binary);

    // Act
    auto result = id.dataToText(0);

    // Assert
    EXPECT_EQ(result, buffer.toBase64().constData());
}

TEST_F(StringIDTest, dataToTextNoIndex)
{
    // Arrange
    QByteArray data{"data", 4};
    auto id = App::StringID(1, data);

    // Act
    auto result = id.dataToText(0);

    // Assert
    EXPECT_EQ(result, "data");
}

TEST_F(StringIDTest, dataToTextWithIndex)
{
    // Arrange
    QByteArray data{"data", 4};
    auto id = App::StringID(1, data);

    // Act
    auto resultA = id.dataToText(1);
    auto resultB = id.dataToText(1024);  // NOLINT

    // Assert
    EXPECT_EQ(resultA, "data1");
    EXPECT_EQ(resultB, "data1024"); // Not hex!
}

TEST_F(StringIDTest, dataToTextWithPostfix)
{
    // Arrange
    QByteArray data{"data", 4};
    QByteArray postfix{"postfix", 7};  // NOLINT
    auto id = App::StringID(1, data);
    id.setPostfix(postfix);

    // Act
    auto result = id.dataToText(1);

    // Assert
    EXPECT_EQ(result, "data1postfix");
}

TEST_F(StringIDTest, dataToBytesNoIndex)
{
    // Arrange
    QByteArray data{"data", 4};
    auto id = App::StringID(1, data);

    // Act
    auto result = id.dataToBytes();

    // Assert
    EXPECT_EQ(data, result);
}

TEST_F(StringIDTest, dataToBytesWithIndex)
{
    // Arrange
    QByteArray data{"data", 4};
    const int index {1234};
    auto id = App::StringID(1, data);

    // Act
    auto result = id.dataToBytes(index);

    // Assert
    EXPECT_EQ(data + QByteArray::number(index), result);
}

TEST_F(StringIDTest, dataToBytesWithPostfix)
{
    // Arrange
    QByteArray data{"data", 4};
    QByteArray postfix{"postfix", 7}; // NOLINT
    auto id = App::StringID(1, data);
    id.setPostfix(postfix);

    // Act
    auto result = id.dataToBytes();

    // Assert
    EXPECT_EQ(data + postfix, result);
}

TEST_F(StringIDTest, dataToBytesWithIndexAndPostfix)
{
    // Arrange
    QByteArray data{"data", 4};
    QByteArray postfix{"postfix", 7}; // NOLINT
    const int index {1234};
    auto id = App::StringID(1, data);
    id.setPostfix(postfix);

    // Act
    auto result = id.dataToBytes(index);

    // Assert
    EXPECT_EQ(data + QByteArray::number(index) + postfix, result);
}

TEST_F(StringIDTest, mark)
{
    // Arrange
    QByteArray data{"data", 4};
    auto id = App::StringID(1, data);
    ASSERT_FALSE(id.isMarked());

    // Act
    id.mark();

    // Assert
    EXPECT_TRUE(id.isMarked());
}

TEST_F(StringIDTest, setPersistent)
{
    // Arrange
    QByteArray data{"data", 4};
    auto id = App::StringID(1, data);
    ASSERT_FALSE(id.isPersistent());

    // Act
    id.setPersistent(true);

    // Assert
    EXPECT_TRUE(id.isPersistent());
}

TEST_F(StringIDTest, operatorLessThan)
{
    // Can't test without a _hasher
}

TEST_F(StringIDTest, compare)
{
    // Can't test without a _hasher
}


class StringIDRefTest: public ::testing::Test
{
protected:
    // void SetUp() override {}
    // void TearDown() override {}
};


class StringHasherTest: public ::testing::Test
{
protected:
    // void SetUp() override {}
    // void TearDown() override {}
};