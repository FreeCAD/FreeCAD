// SPDX-License-Identifier: LGPL-2.1-or-later

#include "App/MappedName.h"
#include "gtest/gtest.h"

#include <App/StringHasher.h>
#include <App/StringHasherPy.h>
#include <App/StringIDPy.h>

#include <QCryptographicHash>
#include <array>

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

TEST_F(StringIDTest, stringIDManualConstructionNoFlags)  // NOLINT
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

TEST_F(StringIDTest, stringIDManualConstructionWithFlag)  // NOLINT
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

TEST_F(StringIDTest, stringIDDefaultConstruction)  // NOLINT
{
    // Arrange & Act
    auto id = App::StringID();

    // Assert
    EXPECT_EQ(0, id.value());
}

TEST_F(StringIDTest, value)  // NOLINT
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

TEST_F(StringIDTest, relatedIDs)  // NOLINT
{
    // Nothing to test -- relatedIDs are storage-only in this class
}

TEST_F(StringIDTest, isBinary)  // NOLINT
{
    // Arrange
    auto flaggedID = givenFlaggedStringID(App::StringID::Flag::Binary);
    auto controlID = App::StringID {};

    // Act & Assert
    EXPECT_TRUE(flaggedID.isBinary());
    EXPECT_FALSE(controlID.isBinary());
}

TEST_F(StringIDTest, isHashed)  // NOLINT
{
    // Arrange
    auto flaggedID = givenFlaggedStringID(App::StringID::Flag::Hashed);
    auto controlID = App::StringID {};

    // Act & Assert
    EXPECT_TRUE(flaggedID.isHashed());
    EXPECT_FALSE(controlID.isHashed());
}

TEST_F(StringIDTest, isPostfixed)  // NOLINT
{
    // Arrange
    auto flaggedID = givenFlaggedStringID(App::StringID::Flag::Postfixed);
    auto controlID = App::StringID {};

    // Act & Assert
    EXPECT_TRUE(flaggedID.isPostfixed());
    EXPECT_FALSE(controlID.isPostfixed());
}

TEST_F(StringIDTest, isPostfixEncoded)  // NOLINT
{
    // Arrange
    auto flaggedID = givenFlaggedStringID(App::StringID::Flag::PostfixEncoded);
    auto controlID = App::StringID {};

    // Act & Assert
    EXPECT_TRUE(flaggedID.isPostfixEncoded());
    EXPECT_FALSE(controlID.isPostfixEncoded());
}

TEST_F(StringIDTest, isIndexed)  // NOLINT
{
    // Arrange
    auto flaggedID = givenFlaggedStringID(App::StringID::Flag::Indexed);
    auto controlID = App::StringID {};

    // Act & Assert
    EXPECT_TRUE(flaggedID.isIndexed());
    EXPECT_FALSE(controlID.isIndexed());
}

TEST_F(StringIDTest, isPrefixID)  // NOLINT
{
    // Arrange
    auto flaggedID = givenFlaggedStringID(App::StringID::Flag::PrefixID);
    auto controlID = App::StringID {};

    // Act & Assert
    EXPECT_TRUE(flaggedID.isPrefixID());
    EXPECT_FALSE(controlID.isPrefixID());
}

TEST_F(StringIDTest, isPrefixIDIndex)  // NOLINT
{
    // Arrange
    auto flaggedID = givenFlaggedStringID(App::StringID::Flag::PrefixIDIndex);
    auto controlID = App::StringID {};

    // Act & Assert
    EXPECT_TRUE(flaggedID.isPrefixIDIndex());
    EXPECT_FALSE(controlID.isPrefixIDIndex());
}

TEST_F(StringIDTest, isMarked)  // NOLINT
{
    // Arrange
    auto flaggedID = givenFlaggedStringID(App::StringID::Flag::Marked);
    auto controlID = App::StringID {};

    // Act & Assert
    EXPECT_TRUE(flaggedID.isMarked());
    EXPECT_FALSE(controlID.isMarked());
}

TEST_F(StringIDTest, isPersistent)  // NOLINT
{
    // Arrange
    auto flaggedID = givenFlaggedStringID(App::StringID::Flag::Persistent);
    auto controlID = App::StringID {};

    // Act & Assert
    EXPECT_TRUE(flaggedID.isPersistent());
    EXPECT_FALSE(controlID.isPersistent());
}

TEST_F(StringIDTest, isFromSameHasher)  // NOLINT
{
    // Nothing to test except when used by StringHasher
}

TEST_F(StringIDTest, getHasher)  // NOLINT
{
    // Nothing to test except when used by StringHasher
}

TEST_F(StringIDTest, data)  // NOLINT
{
    // Arrange
    QByteArray expectedData {"data", 4};
    auto id = App::StringID(1, expectedData);

    // Act
    auto data = id.data();

    // Assert
    EXPECT_EQ(expectedData, data);
}

TEST_F(StringIDTest, postfix)  // NOLINT
{
    // Nothing to test except when used by StringHasher
}

TEST_F(StringIDTest, getPyObject)  // NOLINT
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

TEST_F(StringIDTest, getPyObjectWithIndex)  // NOLINT
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

TEST_F(StringIDTest, toStringWithoutIndex)  // NOLINT
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
    EXPECT_EQ(std::string("#fcad10"), resultB);  // Make sure result is in hex
}

TEST_F(StringIDTest, toStringWithIndex)  // NOLINT
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

TEST_F(StringIDTest, fromStringWithEOFAndLengthGood)  // NOLINT
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

TEST_F(StringIDTest, fromStringExtraData)  // NOLINT
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

TEST_F(StringIDTest, fromStringLengthUnspecified)  // NOLINT
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

TEST_F(StringIDTest, fromStringShorterLength)  // NOLINT
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

TEST_F(StringIDTest, fromStringNoHashtag)  // NOLINT
{
    // Arrange
    const std::string testString {"1:fcad"};

    // Act
    auto result = App::StringID::fromString(testString.c_str(), true);

    // Assert
    EXPECT_EQ(result.id, -1);
}

TEST_F(StringIDTest, fromStringNotHex)  // NOLINT
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

TEST_F(StringIDTest, fromStringQByteArray)  // NOLINT
{
    // Arrange
    const QByteArray testString {"#1:fcad", 7};

    // Act
    auto result = App::StringID::fromString(testString, true);

    // Assert
    EXPECT_EQ(result.id, 1);
    EXPECT_EQ(result.index, 0xfcad);
}

TEST_F(StringIDTest, dataToTextHashed)  // NOLINT
{
    // Arrange
    QByteArray buffer {"120ca87015d849dbea060eaf2295fcc4ee981427", 40};  // NOLINT
    auto id = App::StringID(1, buffer, App::StringID::Flag::Hashed);

    // Act
    auto result = id.dataToText(0);

    // Assert
    EXPECT_EQ(result, buffer.toBase64().constData());
}

TEST_F(StringIDTest, dataToTextBinary)  // NOLINT
{
    // Arrange
    QByteArray buffer {"120ca87015d849dbea060eaf2295fcc4ee981427", 40};  // NOLINT
    auto id = App::StringID(1, buffer, App::StringID::Flag::Binary);

    // Act
    auto result = id.dataToText(0);

    // Assert
    EXPECT_EQ(result, buffer.toBase64().constData());
}

TEST_F(StringIDTest, dataToTextNoIndex)  // NOLINT
{
    // Arrange
    QByteArray data {"data", 4};
    auto id = App::StringID(1, data);

    // Act
    auto result = id.dataToText(0);

    // Assert
    EXPECT_EQ(result, "data");
}

TEST_F(StringIDTest, dataToTextWithIndex)  // NOLINT
{
    // Arrange
    QByteArray data {"data", 4};
    auto id = App::StringID(1, data);

    // Act
    auto resultA = id.dataToText(1);
    auto resultB = id.dataToText(1024);  // NOLINT

    // Assert
    EXPECT_EQ(resultA, "data1");
    EXPECT_EQ(resultB, "data1024");  // Not hex!
}

TEST_F(StringIDTest, dataToTextWithPostfix)  // NOLINT
{
    // Arrange
    QByteArray data {"data", 4};
    QByteArray postfix {"postfix", 7};  // NOLINT
    auto id = App::StringID(1, data);
    id.setPostfix(postfix);

    // Act
    auto result = id.dataToText(1);

    // Assert
    EXPECT_EQ(result, "data1postfix");
}

TEST_F(StringIDTest, dataToBytesNoIndex)  // NOLINT
{
    // Arrange
    QByteArray data {"data", 4};
    auto id = App::StringID(1, data);

    // Act
    auto result = id.dataToBytes();

    // Assert
    EXPECT_EQ(data, result);
}

TEST_F(StringIDTest, dataToBytesWithIndex)  // NOLINT
{
    // Arrange
    QByteArray data {"data", 4};
    const int index {1234};
    auto id = App::StringID(1, data);

    // Act
    auto result = id.dataToBytes(index);

    // Assert
    EXPECT_EQ(data + QByteArray::number(index), result);
}

TEST_F(StringIDTest, dataToBytesWithPostfix)  // NOLINT
{
    // Arrange
    QByteArray data {"data", 4};
    QByteArray postfix {"postfix", 7};  // NOLINT
    auto id = App::StringID(1, data);
    id.setPostfix(postfix);

    // Act
    auto result = id.dataToBytes();

    // Assert
    EXPECT_EQ(data + postfix, result);
}

TEST_F(StringIDTest, dataToBytesWithIndexAndPostfix)  // NOLINT
{
    // Arrange
    QByteArray data {"data", 4};
    QByteArray postfix {"postfix", 7};  // NOLINT
    const int index {1234};
    auto id = App::StringID(1, data);
    id.setPostfix(postfix);

    // Act
    auto result = id.dataToBytes(index);

    // Assert
    EXPECT_EQ(data + QByteArray::number(index) + postfix, result);
}

TEST_F(StringIDTest, mark)  // NOLINT
{
    // Arrange
    QByteArray data {"data", 4};
    auto id = App::StringID(1, data);
    ASSERT_FALSE(id.isMarked());

    // Act
    id.mark();

    // Assert
    EXPECT_TRUE(id.isMarked());
}

TEST_F(StringIDTest, setPersistent)  // NOLINT
{
    // Arrange
    QByteArray data {"data", 4};
    auto id = App::StringID(1, data);
    ASSERT_FALSE(id.isPersistent());

    // Act
    id.setPersistent(true);

    // Assert
    EXPECT_TRUE(id.isPersistent());
}

TEST_F(StringIDTest, operatorLessThan)  // NOLINT
{
    // Can't test without a _hasher
}

TEST_F(StringIDTest, compare)  // NOLINT
{
    // Can't test without a _hasher
}

TEST_F(StringIDTest, IndexIDBooleanConversion)  // NOLINT
{
    // Arrange
    const long id {42};
    const int index {123};
    App::StringID::IndexID indexIdTrue {id, index};
    App::StringID::IndexID indexIdFalse {0, index};

    // Act & Assert
    EXPECT_TRUE(indexIdTrue);
    EXPECT_FALSE(indexIdFalse);
}

TEST_F(StringIDTest, IndexIDStreamInsertionOperator)  // NOLINT
{
    // Arrange
    const long id {42};
    const int index {123};
    App::StringID::IndexID indexIdNonZero {id, index};
    App::StringID::IndexID indexIdZero {id, 0};
    std::ostringstream stream;

    // Act
    stream << indexIdNonZero << " " << indexIdZero;

    // Assert
    EXPECT_EQ("42:123 42", stream.str());
}


class StringIDRefTest: public ::testing::Test
{
protected:
    // void SetUp() override {}
    // void TearDown() override {}

    App::StringID* createStringID() const
    {
        return new App::StringID {_id, _data};
    }

private:
    QByteArray _data {"data", 4};
    int _id {1};
};


TEST_F(StringIDRefTest, defaultConstructor)  // NOLINT
{
    // Arrange & Act
    auto idRef = App::StringIDRef();

    // Assert
    EXPECT_FALSE(idRef);
}

TEST_F(StringIDRefTest, constructFromNewStringID)  // NOLINT
{
    // Arrange & Act
    auto idRef = App::StringIDRef(createStringID());

    // Assert
    EXPECT_TRUE(idRef);
    EXPECT_EQ(1, idRef.getRefCount());

    // NOTE: the dynamically-allocated StringID is automatically deallocated by the StringIDRef
    // when its destructor is called (upon exit from this test function).
}

TEST_F(StringIDRefTest, constructFromStringIDAndIndex)  // NOLINT
{
    // Arrange
    const int index {42};

    // Act
    auto idRef = App::StringIDRef(createStringID(), index);

    // Assert
    EXPECT_TRUE(idRef);
    EXPECT_EQ(1, idRef.getRefCount());
    EXPECT_EQ(index, idRef.getIndex());

    // NOTE: the dynamically-allocated StringID is automatically deallocated by the StringIDRef
    // when its destructor is called (upon exit from this test function).
}

TEST_F(StringIDRefTest, copyConstructor)  // NOLINT
{
    // Arrange
    const int index {42};
    auto idRef = App::StringIDRef(createStringID(), index);

    // Act
    auto newIdRef = App::StringIDRef(idRef);

    // Assert
    EXPECT_TRUE(newIdRef);
    EXPECT_EQ(2, newIdRef.getRefCount());
    EXPECT_EQ(index, idRef.getIndex());
    EXPECT_EQ(index, newIdRef.getIndex());
}

TEST_F(StringIDRefTest, copyConstructorWithIndex)  // NOLINT
{
    // Arrange
    const int index {42};
    const int otherIndex {12345};
    auto idRef = App::StringIDRef(createStringID(), index);

    // Act
    auto newIdRef = App::StringIDRef(idRef, otherIndex);

    // Assert
    EXPECT_TRUE(newIdRef);
    EXPECT_EQ(2, newIdRef.getRefCount());
    EXPECT_EQ(index, idRef.getIndex());
    EXPECT_EQ(otherIndex, newIdRef.getIndex());
}

TEST_F(StringIDRefTest, moveConstructor)  // NOLINT
{
    // Arrange
    auto idRef = App::StringIDRef(createStringID());

    // Act
    auto newIdRef = App::StringIDRef(std::move(idRef));

    // Assert
    EXPECT_EQ(1, newIdRef.getRefCount());
}

TEST_F(StringIDRefTest, destructor)  // NOLINT
{
    // Arrange
    auto idRef = App::StringIDRef(createStringID());

    {
        auto newIdRef = App::StringIDRef(idRef);
        ASSERT_EQ(2, idRef.getRefCount());  // Verify the test setup

        // Act
        // The scope ends, causing newIdRef destructor execution
    }

    // Assert
    EXPECT_EQ(1, idRef.getRefCount());
}

TEST_F(StringIDRefTest, reset)  // NOLINT
{
    // Arrange
    auto idRef = App::StringIDRef(createStringID());

    // Act
    idRef.reset();

    // Assert
    EXPECT_FALSE(idRef);
}

TEST_F(StringIDRefTest, resetWithStringID)  // NOLINT
{
    // Arrange
    const int index {42};
    auto idRef = App::StringIDRef(createStringID(), index);

    // Act
    idRef.reset(createStringID());

    // Assert
    EXPECT_TRUE(idRef);
    EXPECT_NE(index, idRef.getIndex());
}

TEST_F(StringIDRefTest, resetWithStringIDAndIndex)  // NOLINT
{
    // Arrange
    const int indexA {42};
    const int indexB {12345};
    auto idRef = App::StringIDRef(createStringID(), indexA);

    // Act
    idRef.reset(createStringID(), indexB);

    // Assert
    EXPECT_TRUE(idRef);
    EXPECT_EQ(indexB, idRef.getIndex());
}

TEST_F(StringIDRefTest, swap)  // NOLINT
{
    // Arrange
    const int indexA {42};
    const int indexB {12345};
    auto idRefA = App::StringIDRef(createStringID(), indexA);
    auto idRefB = App::StringIDRef(createStringID(), indexB);

    // Act
    idRefA.swap(idRefB);

    // Assert
    EXPECT_EQ(indexB, idRefA.getIndex());
    EXPECT_EQ(indexA, idRefB.getIndex());
}

TEST_F(StringIDRefTest, assignmentFromSelf)  // NOLINT
{
    // Arrange
    auto idRef = App::StringIDRef(createStringID());

    // Act
    idRef = idRef;

    // Assert
    EXPECT_EQ(1, idRef.getRefCount());
}

TEST_F(StringIDRefTest, assignmentToEmptyFromStringID)  // NOLINT
{
    // Arrange
    Py_Initialize();
    auto idRef = App::StringIDRef();
    ASSERT_FALSE(idRef);  // Verify setup

    // Act
    idRef = createStringID();

    // Assert
    EXPECT_TRUE(idRef);
}

TEST_F(StringIDRefTest, assignmentFromStringIDRef)  // NOLINT
{
    // Arrange
    auto firstIdRef = App::StringIDRef(createStringID());
    auto firstIdRefExtra = firstIdRef;
    auto secondIdRef = App::StringIDRef(createStringID());

    // Act
    firstIdRef = secondIdRef;

    // Assert
    EXPECT_EQ(2, secondIdRef.getRefCount());
    EXPECT_EQ(2, firstIdRef.getRefCount());
    EXPECT_EQ(1, firstIdRefExtra.getRefCount());
}

TEST_F(StringIDRefTest, moveAssignmentFromStringIDRef)  // NOLINT
{
    auto emptyIdRef = App::StringIDRef();
    auto goodIdRef = App::StringIDRef(createStringID());
    ASSERT_FALSE(emptyIdRef);  // Verify setup

    // Act
    emptyIdRef = std::move(goodIdRef);

    // Assert
    EXPECT_TRUE(emptyIdRef);
    EXPECT_EQ(1, emptyIdRef.getRefCount());
}

TEST_F(StringIDRefTest, operatorLess)  // NOLINT
{
    // Arrange
    auto emptySIDA = App::StringIDRef();
    auto emptySIDB = App::StringIDRef();
    auto lowID = App::StringIDRef(new App::StringID {1, nullptr});
    auto highID = App::StringIDRef(new App::StringID {2, nullptr});

    // Act & Assert
    EXPECT_FALSE(emptySIDA < emptySIDB);
    EXPECT_FALSE(emptySIDB < emptySIDA);
    EXPECT_TRUE(emptySIDA < lowID);
    EXPECT_TRUE(emptySIDA < highID);
    EXPECT_TRUE(lowID < highID);
    EXPECT_FALSE(highID < lowID);

    // NOTE: Cannot test the impact of hasher without a StringHasher
}

TEST_F(StringIDRefTest, operatorEquality)  // NOLINT
{
    // Arrange
    auto emptySIDA = App::StringIDRef();
    auto emptySIDB = App::StringIDRef();
    auto nonEmptyA = App::StringIDRef(new App::StringID {1, nullptr});
    auto nonEmptyB = App::StringIDRef(new App::StringID {1, nullptr});
    auto nonEmptyOther = App::StringIDRef(new App::StringID {2, nullptr});

    // Act & Assert
    EXPECT_TRUE(emptySIDA == emptySIDB);
    EXPECT_TRUE(nonEmptyA == nonEmptyB);
    EXPECT_FALSE(emptySIDA == nonEmptyA);
    EXPECT_FALSE(nonEmptyA == nonEmptyOther);
}

TEST_F(StringIDRefTest, operatorInequality)  // NOLINT
{
    // Arrange
    auto emptySIDA = App::StringIDRef();
    auto emptySIDB = App::StringIDRef();
    auto nonEmptyA = App::StringIDRef(new App::StringID {1, nullptr});
    auto nonEmptyB = App::StringIDRef(new App::StringID {1, nullptr});
    auto nonEmptyOther = App::StringIDRef(new App::StringID {2, nullptr});

    // Act & Assert
    EXPECT_FALSE(emptySIDA != emptySIDB);
    EXPECT_FALSE(nonEmptyA != nonEmptyB);
    EXPECT_TRUE(emptySIDA != nonEmptyA);
    EXPECT_TRUE(nonEmptyA != nonEmptyOther);
}

TEST_F(StringIDRefTest, booleanConversion)  // NOLINT
{
    // Arrange
    auto emptySID = App::StringIDRef();
    auto nonEmpty = App::StringIDRef(new App::StringID {1, nullptr});

    // Act & Assert
    EXPECT_FALSE(emptySID);
    EXPECT_TRUE(nonEmpty);
}

TEST_F(StringIDRefTest, getRefCount)  // NOLINT
{
    // Arrange
    auto stringID = createStringID();
    auto stringIDRef = App::StringIDRef(stringID);

    // Act
    auto firstCount = stringIDRef.getRefCount();
    auto stringIDRef2 = App::StringIDRef(stringID);
    auto secondCount = stringIDRef.getRefCount();

    // Assert
    EXPECT_EQ(1, firstCount);
    EXPECT_EQ(2, secondCount);
}

TEST_F(StringIDRefTest, toString)  // NOLINT
{
    // Arrange
    auto emptySID = App::StringIDRef();
    auto nonEmpty = App::StringIDRef(createStringID());

    // Act
    auto empty = emptySID.toString();
    auto nonempty = nonEmpty.toString();

    // Assert
    // Only confirm that the function call is passed along: the real test is in the StringID class
    EXPECT_TRUE(empty.empty());
    EXPECT_FALSE(nonempty.empty());
}

TEST_F(StringIDRefTest, dataToText)  // NOLINT
{
    // Arrange
    auto emptySID = App::StringIDRef();
    auto nonEmpty = App::StringIDRef(createStringID());

    // Act
    auto empty = emptySID.dataToText();
    auto nonempty = nonEmpty.dataToText();

    // Assert
    // Only confirm that the function call is passed along: the real test is in the StringID class
    EXPECT_TRUE(empty.empty());
    EXPECT_FALSE(nonempty.empty());
}

TEST_F(StringIDRefTest, constData)  // NOLINT
{
    // Arrange
    auto sid = App::StringIDRef(createStringID());

    // Act
    auto constData = sid.constData();

    // Assert
    ASSERT_NE(constData, nullptr);
    EXPECT_STREQ(constData, "data");
}

TEST_F(StringIDRefTest, deref)  // NOLINT
{
    // Arrange
    auto sid = createStringID();
    auto ref = App::StringIDRef(sid);

    // Act & Assert
    EXPECT_EQ(sid, &(ref.deref()));
}

TEST_F(StringIDRefTest, value)  // NOLINT
{
    // Arrange
    auto empty = App::StringIDRef();
    auto nonEmpty = App::StringIDRef(createStringID());

    // Act
    auto emptyValue = empty.value();
    auto nonEmptyValue = nonEmpty.value();

    // Assert
    EXPECT_EQ(0, emptyValue);
    EXPECT_NE(0, nonEmptyValue);
}

TEST_F(StringIDRefTest, relatedIDs)  // NOLINT
{
    // Nothing to test without a StringHasher
}

TEST_F(StringIDRefTest, isBinary)  // NOLINT
{
    // Arrange
    auto nothing = App::StringIDRef();
    auto binary = App::StringIDRef(new App::StringID {1, nullptr, App::StringID::Flag::Binary});
    auto nonBinary = App::StringIDRef(new App::StringID {1, nullptr, App::StringID::Flag::None});

    // Act & Assert
    EXPECT_FALSE(nothing.isBinary());
    EXPECT_TRUE(binary.isBinary());
    EXPECT_FALSE(nonBinary.isBinary());
}

TEST_F(StringIDRefTest, isHashed)  // NOLINT
{
    // Arrange
    auto nothing = App::StringIDRef();
    auto hashed = App::StringIDRef(new App::StringID {1, nullptr, App::StringID::Flag::Hashed});
    auto nonHashed = App::StringIDRef(new App::StringID {1, nullptr, App::StringID::Flag::None});

    // Act & Assert
    EXPECT_FALSE(nothing.isHashed());
    EXPECT_TRUE(hashed.isHashed());
    EXPECT_FALSE(nonHashed.isHashed());
}

TEST_F(StringIDRefTest, toBytes)  // NOLINT
{
    // Arrange
    QByteArray byteStorage;
    auto ref = App::StringIDRef(createStringID());

    // Act
    ref.toBytes(byteStorage);

    // Assert
    EXPECT_FALSE(byteStorage.isNull());
}

TEST_F(StringIDRefTest, getPyObject)  // NOLINT
{
    // Arrange
    auto ref = App::StringIDRef(createStringID());
    auto empty = App::StringIDRef();

    // Act
    Py::Object pyObject(ref.getPyObject(), true);
    Py::Object none(empty.getPyObject(), true);

    // Assert
    EXPECT_TRUE(PyObject_TypeCheck(pyObject.ptr(), &App::StringIDPy::Type));
    EXPECT_EQ(none.ptr(), Py_None);
}

TEST_F(StringIDRefTest, mark)  // NOLINT
{
    // Arrange
    auto ref = App::StringIDRef(createStringID());
    ASSERT_FALSE(ref.isMarked());

    // Act
    ref.mark();

    // Assert
    EXPECT_TRUE(ref.isMarked());
}

TEST_F(StringIDRefTest, isMarked)  // NOLINT
{
    // Arrange
    auto marked = App::StringIDRef(new App::StringID(1, nullptr, App::StringID::Flag::Marked));
    auto notMarked = App::StringIDRef(createStringID());

    // Act & Assert
    EXPECT_TRUE(marked.isMarked());
    EXPECT_FALSE(notMarked.isMarked());
}

TEST_F(StringIDRefTest, isFromSameHasher)  // NOLINT
{
    // Nothing to test, requires a StringHasher
}

TEST_F(StringIDRefTest, getHasher)  // NOLINT
{
    // Nothing to test, requires a StringHasher
}

TEST_F(StringIDRefTest, setPersistent)  // NOLINT
{
    // Arrange
    auto persistent = App::StringIDRef(createStringID());
    ASSERT_FALSE(persistent.deref().isPersistent());

    // Act
    persistent.setPersistent(true);

    // Assert
    ASSERT_TRUE(persistent.deref().isPersistent());
}


class StringHasherTest: public ::testing::Test
{
protected:
    void SetUp() override
    {
        Py_Initialize();
        _hasher = Base::Reference<App::StringHasher>(new App::StringHasher);
    }

    void TearDown() override
    {
        _hasher->clear();
    }

    Base::Reference<App::StringHasher> Hasher()
    {
        return _hasher;
    }

    static Data::MappedName givenMappedName(const char* name, const char* postfix = nullptr)
    {
        QByteArray expectedPrefix {name, static_cast<int>(std::strlen(name))};
        Data::MappedName mappedName(expectedPrefix);
        if (postfix) {
            QByteArray expectedPostfix {postfix, static_cast<int>(std::strlen(postfix))};
            Data::MappedName mappedNameA(mappedName, expectedPostfix.data());
            return mappedNameA;
        }
        return mappedName;
    }

    /// Put a couple of things into the hash table: at the end of this call the size of the table
    /// is 2, one postfix string and one prefix+postfix combination.
    App::StringIDRef givenSomeHashedValues()
    {
        const std::string prefix {"Test1"};
        const std::string postfix {";:M;FUS;:Hb:7,F"};
        auto mappedName = givenMappedName(prefix.c_str(), postfix.c_str());
        QVector<App::StringIDRef> sids;
        return Hasher()->getID(mappedName, sids);
    }

private:
    Base::Reference<App::StringHasher> _hasher;
};

TEST_F(StringHasherTest, defaultConstructor)  // NOLINT
{
    // Arrange
    // Done in Setup()

    // Act
    // Done in Setup()

    // Assert
    EXPECT_EQ(0, Hasher()->size());
}

TEST_F(StringHasherTest, getMemSize)  // NOLINT
{
    // Arrange
    givenSomeHashedValues();

    // Act
    auto result = Hasher()->getMemSize();

    // Assert
    // getMemSize is advisory only, so the only thing we can confidently assert is that it is larger
    // than the number of values in the hash table.
    EXPECT_LT(Hasher()->size(), result);
}

TEST_F(StringHasherTest, Save)  // NOLINT
{
    // Arrange
    // Act
    // Assert
}

TEST_F(StringHasherTest, Restore)  // NOLINT
{
    // Arrange
    // Act
    // Assert
}

TEST_F(StringHasherTest, SaveDocFile)  // NOLINT
{
    // Arrange
    // Act
    // Assert
}

TEST_F(StringHasherTest, RestoreDocFile)  // NOLINT
{
    // Arrange
    // Act
    // Assert
}

TEST_F(StringHasherTest, setPersistenceFileName)  // NOLINT
{
    // Arrange
    // Act
    // Assert
}

TEST_F(StringHasherTest, getPersistenceFileName)  // NOLINT
{
    // Arrange
    // Act
    // Assert
}

TEST_F(StringHasherTest, getIDFromQByteArrayShort)  // NOLINT
{
    // Arrange
    const std::array<char, 5> string {"data"};
    QByteArray qba(string.data(), string.size());
    Hasher()->setThreshold(string.size() + 1);

    // Act
    auto id = Hasher()->getID(qba, App::StringHasher::Option::Hashable);

    // Assert
    EXPECT_STREQ(string.data(), id.constData());
    EXPECT_FALSE(id.isHashed());
    EXPECT_NE(qba.constData(), id.constData());  // A copy was made, the pointers differ
    EXPECT_EQ(2, id.getRefCount());
}

TEST_F(StringHasherTest, getIDFromQByteArrayLongHashable)  // NOLINT
{
    // Arrange
    const std::array<char, 47> string {"data that is longer than our hasher threshold"};
    QByteArray qba(string.data(), string.size());
    Hasher()->setThreshold(string.size() - 1);

    // Act
    auto id = Hasher()->getID(qba, App::StringHasher::Option::Hashable);

    // Assert
    EXPECT_STRNE(string.data(), id.constData());
    EXPECT_TRUE(id.isHashed());
    EXPECT_NE(qba.constData(), id.constData());  // A copy was made, the pointers differ
}

TEST_F(StringHasherTest, getIDFromQByteArrayLongUnhashable)  // NOLINT
{
    // Arrange
    const std::array<char, 47> string {"data that is longer than our hasher threshold"};
    QByteArray qba(string.data(), string.size());
    Hasher()->setThreshold(string.size() - 1);

    // Act
    auto id = Hasher()->getID(qba, App::StringHasher::Option::None);

    // Assert
    EXPECT_STREQ(string.data(), id.constData());
    EXPECT_FALSE(id.isHashed());
    EXPECT_NE(qba.constData(), id.constData());  // A copy was made, the pointers differ
}

TEST_F(StringHasherTest, getIDFromQByteArrayNoCopy)  // NOLINT
{
    // Arrange
    const std::array<char, 5> string {"data"};
    QByteArray qba(string.data(), string.size());
    Hasher()->setThreshold(string.size() + 1);

    // Act
    auto id = Hasher()->getID(qba, App::StringHasher::Option::NoCopy);

    // Assert
    EXPECT_STREQ(string.data(), id.constData());
    EXPECT_EQ(qba.constData(), id.constData());  // No copy was made, the pointers are the same
}

TEST_F(StringHasherTest, getIDFromQByteArrayTwoDifferentStrings)  // NOLINT
{
    // Arrange
    const std::array<char, 6> stringA {"dataA"};
    QByteArray qbaA(stringA.data(), stringA.size());
    const std::array<char, 6> stringB {"dataB"};
    QByteArray qbaB(stringB.data(), stringB.size());

    // Act
    auto idA = Hasher()->getID(qbaA);
    auto idB = Hasher()->getID(qbaB);

    // Assert
    EXPECT_NE(idA.dataToText(), idB.dataToText());
}

TEST_F(StringHasherTest, getIDFromQByteArrayTwoIdenticalStrings)  // NOLINT
{
    // Arrange
    const std::array<char, 5> stringA {"data"};
    QByteArray qbaA(stringA.data(), stringA.size());
    const std::array<char, 5> stringB {"data"};
    QByteArray qbaB(stringB.data(), stringB.size());

    // Act
    auto idA = Hasher()->getID(qbaA);
    auto idB = Hasher()->getID(qbaB);

    // Assert
    EXPECT_EQ(idA.dataToText(), idB.dataToText());
}

TEST_F(StringHasherTest, getIDFromQByteArrayBinaryFlag)  // NOLINT
{
    // Arrange
    const std::array<char, 5> string {"data"};
    QByteArray qba(string.data(), string.size());

    // Act
    auto id = Hasher()->getID(qba, App::StringHasher::Option::Binary);

    // Assert
    EXPECT_TRUE(id.isBinary());
}

TEST_F(StringHasherTest, getIDFromCString)  // NOLINT
{
    // Arrange
    // Act
    // Assert
}

/*
 * Things that have to be tested for getIDFromMappedName:
 *   1. With and without postfix (every other path must test both)
 *   2. Existing entry: short circuits
 *   3. Raw data and non-raw
 *   4. Postfix contains # and not
 *   5. Indexed name and not
 *   6. sids empty and sids with content
 *   7. sids whose hasher==this and whose hasher is something else
 *   8. If sids.size() > 10, duplicates get removed
 */

TEST_F(StringHasherTest, getIDFromMappedNameWithoutPostfixWithoutIndex)  // NOLINT
{
    // Arrange
    const char* name {"Face"};
    QByteArray expectedPrefix {name, static_cast<int>(std::strlen(name))};
    Data::MappedName mappedName1(expectedPrefix);
    QVector<App::StringIDRef> sids;

    // Act
    auto id = Hasher()->getID(mappedName1, sids);

    // Assert
    EXPECT_EQ(id.dataToText(), mappedName1.toString());
}

TEST_F(StringHasherTest, getIDFromMappedNameWithoutPostfixWithIndex)  // NOLINT
{
    // Arrange
    const char* expectedName {"Face"};
    QByteArray expectedPrefix {expectedName, static_cast<int>(std::strlen(expectedName))};
    const char* name {"Face3"};
    QByteArray prefix {name, static_cast<int>(std::strlen(name))};
    Data::MappedName mappedName1(prefix);
    QVector<App::StringIDRef> sids;

    // Act
    auto id = Hasher()->getID(mappedName1, sids);

    // Assert
    EXPECT_EQ(id.dataToText(), mappedName1.toString());
}

TEST_F(StringHasherTest, getIDFromMappedNameWithoutIndexWithPostfix)  // NOLINT
{
    // Arrange
    const char* name {"Face"};
    QByteArray expectedPrefix {name, static_cast<int>(std::strlen(name))};
    const char* postfix {";:M;FUS;:Hb:7,F"};
    QByteArray expectedPostfix {postfix, static_cast<int>(std::strlen(postfix))};
    Data::MappedName mappedName1(expectedPrefix);
    Data::MappedName mappedName2(mappedName1, expectedPostfix.data());
    QVector<App::StringIDRef> sids;

    // Act
    auto id = Hasher()->getID(mappedName2, sids);

    // Assert
    EXPECT_EQ(expectedPrefix, id.deref().data());
    EXPECT_EQ(expectedPostfix, id.deref().postfix());
}

TEST_F(StringHasherTest, getIDFromMappedNameWithIndexWithPostfix)  // NOLINT
{
    // Arrange
    const char* name {"Face3"};
    QByteArray expectedPrefix {name, static_cast<int>(std::strlen(name))};
    const char* postfix {";:M;FUS;:Hb:7,F"};
    QByteArray expectedPostfix {postfix, static_cast<int>(std::strlen(postfix))};
    Data::MappedName mappedName1(expectedPrefix);
    Data::MappedName mappedName2(mappedName1, expectedPostfix.data());
    QVector<App::StringIDRef> sids;

    // Act
    auto id = Hasher()->getID(mappedName2, sids);

    // Assert
    EXPECT_EQ(id.dataToText(), mappedName2.toString());
}

TEST_F(StringHasherTest, getIDFromMappedNameExistingNameNoIndex)  // NOLINT
{
    // Arrange
    Data::MappedName mappedName1 = givenMappedName("SomeTestName");
    QVector<App::StringIDRef> sids;
    auto firstIDInserted = Hasher()->getID(mappedName1, sids);
    ASSERT_EQ(1, Hasher()->size());

    // Act
    auto secondIDInserted = Hasher()->getID(mappedName1, sids);

    // Assert
    EXPECT_EQ(secondIDInserted.dataToText(), mappedName1.toString());
}

TEST_F(StringHasherTest, getIDFromMappedNameExistingNameWithIndex)  // NOLINT
{
    // Arrange
    auto mappedNameA = givenMappedName("Test1");
    auto mappedNameB = givenMappedName("Test2");
    QVector<App::StringIDRef> sids;
    auto firstIDInserted = Hasher()->getID(mappedNameA, sids);

    // Act
    auto secondIDInserted = Hasher()->getID(mappedNameB, sids);

    // Assert
    EXPECT_EQ(firstIDInserted.dataToText(), mappedNameA.toString());
    EXPECT_EQ(secondIDInserted.dataToText(), mappedNameB.toString());
}

TEST_F(StringHasherTest, getIDFromMappedNameExistingNameWithIndexAndPostfix)  // NOLINT
{
    // Arrange
    auto mappedNameA = givenMappedName("Test1", ";:M;FUS;:Hb:7,F");
    auto mappedNameB = givenMappedName("Test2", ";:M;FUS;:Hb:7,F");
    QVector<App::StringIDRef> sids;
    auto firstIDInserted = Hasher()->getID(mappedNameA, sids);

    // Act
    auto secondIDInserted = Hasher()->getID(mappedNameB, sids);

    // Assert
    EXPECT_EQ(firstIDInserted.dataToText(), mappedNameA.toString());
    EXPECT_EQ(secondIDInserted.dataToText(), mappedNameB.toString());
}

TEST_F(StringHasherTest, getIDFromMappedNameDuplicateWithEncodedPostfix)  // NOLINT
{
    // Arrange
    auto mappedNameA = givenMappedName("Test1", ";:M;FUS;:Hb:7,F");
    auto mappedNameB = givenMappedName("Test1", "#1");
    QVector<App::StringIDRef> sids;
    auto firstIDInserted = Hasher()->getID(mappedNameA, sids);

    // Act
    auto secondIDInserted = Hasher()->getID(mappedNameB, sids);

    // Assert
    EXPECT_EQ(firstIDInserted.dataToText(), mappedNameA.toString());
    EXPECT_EQ(secondIDInserted.dataToText(), mappedNameB.toString());
}

TEST_F(StringHasherTest, getIDFromIntegerIDNoSuchID)  // NOLINT
{
    // Arrange
    // Do nothing, so the hash table is empty

    // Act
    auto result = Hasher()->getID(1);

    // Assert
    EXPECT_FALSE(result);
}

TEST_F(StringHasherTest, getIDFromIntegerIDBadID)  // NOLINT
{
    // Arrange
    const std::string prefix {"Test1"};
    auto mappedName = givenMappedName(prefix.c_str());
    QVector<App::StringIDRef> sids;
    auto inserted = Hasher()->getID(mappedName, sids);
    ASSERT_EQ(1, Hasher()->size());

    // Act
    auto result = Hasher()->getID(-1);

    // Assert
    EXPECT_FALSE(result);
}


TEST_F(StringHasherTest, getIDMap)  // NOLINT
{
    // Arrange
    givenSomeHashedValues();

    // Act
    auto map = Hasher()->getIDMap();

    // Assert
    EXPECT_GT(map.size(), 0);
}

TEST_F(StringHasherTest, clear)  // NOLINT
{
    // Arrange
    givenSomeHashedValues();

    // Act
    Hasher()->clear();

    // Assert
    EXPECT_EQ(0, Hasher()->size());
}

TEST_F(StringHasherTest, size)  // NOLINT
{
    // Arrange
    givenSomeHashedValues();

    // Act
    auto result = Hasher()->size();

    // Assert
    EXPECT_GT(result, 0);
}

TEST_F(StringHasherTest, count)  // NOLINT
{
    // Arrange
    givenSomeHashedValues();

    // Act
    auto result = Hasher()->count();

    // Assert
    EXPECT_GT(result, 0);
}

TEST_F(StringHasherTest, getPyObject)  // NOLINT
{
    // Arrange - done in setUp()

    // Act
    Py::Object py(Hasher()->getPyObject(), true);

    // Assert
    EXPECT_TRUE(PyObject_TypeCheck(py.ptr(), &App::StringHasherPy::Type));
}

TEST_F(StringHasherTest, setGetSaveAll)  // NOLINT
{
    // Arrange - done by setUp()

    // Act
    Hasher()->setSaveAll(true);
    bool expectedTrue = Hasher()->getSaveAll();
    Hasher()->setSaveAll(false);
    bool expectedFalse = Hasher()->getSaveAll();

    // Assert
    EXPECT_TRUE(expectedTrue);
    EXPECT_FALSE(expectedFalse);
}

TEST_F(StringHasherTest, setGetThreshold)  // NOLINT
{
    // Arrange
    const int expectedThreshold {42};

    // Act
    Hasher()->setThreshold(expectedThreshold);
    auto foundThreshold = Hasher()->getThreshold();

    // Assert
    EXPECT_EQ(expectedThreshold, foundThreshold);
}

TEST_F(StringHasherTest, clearMarks)  // NOLINT
{
    // Arrange
    auto ref = givenSomeHashedValues();
    ref.mark();
    ASSERT_TRUE(ref.isMarked());

    // Act
    Hasher()->clearMarks();

    // Assert
    ASSERT_FALSE(ref.isMarked());
}

TEST_F(StringHasherTest, compact)  // NOLINT
{
    // Arrange
    givenSomeHashedValues();

    // Act
    Hasher()->compact();

    // Assert
    EXPECT_EQ(0, Hasher()->count());
}
