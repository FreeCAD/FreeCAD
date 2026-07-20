// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include "App/ComplexGeoData.h"
#include "App/MappedName.h"

#include <Base/BytesView.h>

#include <string>

// NOLINTBEGIN(readability-magic-numbers)

static Base::BytesView bv(const char* s)
{
    return s ? Base::BytesView(s) : Base::BytesView();
}

static Base::BytesView bv(const char* s, std::size_t size)
{
    return Base::BytesView(s, size);
}

TEST(MappedName, defaultConstruction)
{
    // Act
    Data::MappedName mappedName;

    // Assert
    EXPECT_EQ(mappedName.isRaw(), false);
    EXPECT_EQ(mappedName.empty(), true);
    EXPECT_EQ(mappedName.size(), 0);
    EXPECT_EQ(mappedName.dataBytes(), Base::BytesView());
    EXPECT_EQ(mappedName.postfixBytes(), Base::BytesView());
}

TEST(MappedName, namedConstruction)
{
    // Act
    Data::MappedName mappedName("TEST");

    // Assert
    EXPECT_EQ(mappedName.isRaw(), false);
    EXPECT_EQ(mappedName.empty(), false);
    EXPECT_EQ(mappedName.size(), 4);
    EXPECT_EQ(mappedName.dataBytes(), bv("TEST"));
    EXPECT_EQ(mappedName.postfixBytes(), Base::BytesView());
}

TEST(MappedName, namedConstructionWithMaxSize)
{
    // Act
    Data::MappedName mappedName("TEST", 2);

    // Assert
    EXPECT_EQ(mappedName.isRaw(), false);
    EXPECT_EQ(mappedName.empty(), false);
    EXPECT_EQ(mappedName.size(), 2);
    EXPECT_EQ(mappedName.dataBytes(), bv("TE"));
    EXPECT_EQ(mappedName.postfixBytes(), Base::BytesView());
}

TEST(MappedName, namedConstructionDiscardPrefix)
{
    // Arrange
    std::string name = std::string(Data::ELEMENT_MAP_PREFIX) + "TEST";

    // Act
    Data::MappedName mappedName(name.c_str());

    // Assert
    EXPECT_EQ(mappedName.isRaw(), false);
    EXPECT_EQ(mappedName.empty(), false);
    EXPECT_EQ(mappedName.size(), 4);
    EXPECT_EQ(mappedName.dataBytes(), bv("TEST"));
    EXPECT_EQ(mappedName.postfixBytes(), Base::BytesView());
}

TEST(MappedName, namedConstructionWithMaxSizeDiscardPrefix)
{
    // Arrange
    std::string name = std::string(Data::ELEMENT_MAP_PREFIX) + "TESTTAIL";

    // Act
    Data::MappedName mappedName(name.c_str(), static_cast<int>(Data::ELEMENT_MAP_PREFIX_SIZE + 4));

    // Assert
    EXPECT_EQ(mappedName.isRaw(), false);
    EXPECT_EQ(mappedName.empty(), false);
    EXPECT_EQ(mappedName.size(), 4);
    EXPECT_EQ(mappedName.dataBytes(), bv("TEST"));
    EXPECT_EQ(mappedName.postfixBytes(), Base::BytesView());
}

TEST(MappedName, stringNamedConstruction)
{
    // Act
    Data::MappedName mappedName(std::string("TEST"));

    // Assert
    EXPECT_EQ(mappedName.isRaw(), false);
    EXPECT_EQ(mappedName.empty(), false);
    EXPECT_EQ(mappedName.size(), 4);
    EXPECT_EQ(mappedName.dataBytes(), bv("TEST"));
    EXPECT_EQ(mappedName.postfixBytes(), Base::BytesView());
}

TEST(MappedName, stringNamedConstructionDiscardPrefix)
{
    // Arrange
    std::string name = std::string(Data::ELEMENT_MAP_PREFIX) + "TEST";

    // Act
    Data::MappedName mappedName(name);

    // Assert
    EXPECT_EQ(mappedName.isRaw(), false);
    EXPECT_EQ(mappedName.empty(), false);
    EXPECT_EQ(mappedName.size(), 4);
    EXPECT_EQ(mappedName.dataBytes(), bv("TEST"));
    EXPECT_EQ(mappedName.postfixBytes(), Base::BytesView());
}

TEST(MappedName, constructFromIndexedNameNoIndex)
{
    // Arrange
    Data::IndexedName indexedName {"INDEXED_NAME"};

    // Act
    Data::MappedName mappedName {indexedName};

    // Assert
    EXPECT_EQ(mappedName.dataBytes().data(), indexedName.getType());  // shared memory
    EXPECT_EQ(mappedName.isRaw(), true);
}

TEST(MappedName, constructFromIndexedNameWithIndex)
{
    // Arrange
    Data::IndexedName indexedName {"INDEXED_NAME", 1};

    // Act
    Data::MappedName mappedName {indexedName};

    // Assert
    EXPECT_NE(mappedName.dataBytes().data(), indexedName.getType());  // NOT shared memory
    EXPECT_EQ(mappedName.isRaw(), false);
    EXPECT_EQ(mappedName.toString(), indexedName.toString());
}

TEST(MappedName, copyConstructor)
{
    // Arrange
    Data::MappedName temp("TEST");

    // Act
    Data::MappedName mappedName(temp);

    // Assert
    EXPECT_EQ(mappedName.isRaw(), false);
    EXPECT_EQ(mappedName.empty(), false);
    EXPECT_EQ(mappedName.size(), 4);
    EXPECT_EQ(mappedName.dataBytes(), bv("TEST"));
    EXPECT_EQ(mappedName.postfixBytes(), Base::BytesView());
}

TEST(MappedName, copyConstructorWithPostfix)
{
    // Arrange
    Data::MappedName temp("TEST");

    // Act
    Data::MappedName mappedName(temp, "POSTFIXTEST");

    // Assert
    EXPECT_EQ(mappedName.isRaw(), false);
    EXPECT_EQ(mappedName.empty(), false);
    EXPECT_EQ(mappedName.size(), 15);
    EXPECT_EQ(mappedName.dataBytes(), bv("TEST"));
    EXPECT_EQ(mappedName.postfixBytes(), bv("POSTFIXTEST"));

    // Act
    Data::MappedName mappedName2(mappedName, "ANOTHERPOSTFIX");

    // Assert
    EXPECT_EQ(mappedName2.isRaw(), false);
    EXPECT_EQ(mappedName2.empty(), false);
    EXPECT_EQ(mappedName2.size(), 29);
    EXPECT_EQ(mappedName2.dataBytes(), bv("TESTPOSTFIXTEST"));
    EXPECT_EQ(mappedName2.postfixBytes(), bv("ANOTHERPOSTFIX"));
}

TEST(MappedName, copyConstructorStartpos)
{
    // Arrange
    Data::MappedName temp(Data::MappedName("TEST"), "POSTFIXTEST");

    // Act
    Data::MappedName mappedName(temp, 2, -1);

    // Assert
    EXPECT_EQ(mappedName.isRaw(), false);
    EXPECT_EQ(mappedName.empty(), false);
    EXPECT_EQ(mappedName.size(), 13);
    EXPECT_EQ(mappedName.dataBytes(), bv("ST"));
    EXPECT_EQ(mappedName.postfixBytes(), bv("POSTFIXTEST"));
}

TEST(MappedName, copyConstructorStartposAndSize)
{
    // Arrange
    Data::MappedName temp(Data::MappedName("TEST"), "POSTFIXTEST");

    // Act
    Data::MappedName mappedName(temp, 2, 6);

    // Assert
    EXPECT_EQ(mappedName.isRaw(), false);
    EXPECT_EQ(mappedName.empty(), false);
    EXPECT_EQ(mappedName.size(), 6);
    EXPECT_EQ(mappedName.dataBytes(), bv("ST"));
    EXPECT_EQ(mappedName.postfixBytes(), bv("POST"));
}

TEST(MappedName, moveConstructor)
{
    // Arrange
    Data::MappedName temp(Data::MappedName("TEST"), "POSTFIXTEST");

    // Act
    Data::MappedName mappedName(std::move(temp));

    // Assert
    EXPECT_EQ(mappedName.isRaw(), false);
    EXPECT_EQ(mappedName.empty(), false);
    EXPECT_EQ(mappedName.size(), 15);
    EXPECT_EQ(mappedName.dataBytes(), bv("TEST"));
    EXPECT_EQ(mappedName.postfixBytes(), bv("POSTFIXTEST"));
}

TEST(MappedName, fromRawData)
{
    // Act
    Data::MappedName mappedName = Data::MappedName::fromRawData("TESTTEST", 8);

    // Assert
    EXPECT_EQ(mappedName.isRaw(), true);
    EXPECT_EQ(mappedName.empty(), false);
    EXPECT_EQ(mappedName.size(), 8);
    EXPECT_EQ(mappedName.dataBytes(), bv("TESTTEST", 8));
    EXPECT_EQ(mappedName.postfixBytes(), Base::BytesView());
}

TEST(MappedName, fromRawDataCopy)
{
    // Arrange
    const std::string testBytes("TESTTEST", 8);
    Data::MappedName temp = Data::MappedName::fromRawData(
        Base::BytesView(testBytes.data(), testBytes.size())
    );
    temp.append("TESTPOSTFIX");
    temp.compact();  // Always call compact before accessing data!

    // Act
    Data::MappedName mappedName = Data::MappedName::fromRawData(temp, 0);

    // Assert
    EXPECT_EQ(mappedName.isRaw(), true);
    EXPECT_EQ(mappedName.empty(), false);
    EXPECT_EQ(mappedName.size(), 19);
    EXPECT_EQ(mappedName.dataBytes(), bv("TESTTEST", 8));
    EXPECT_EQ(mappedName.postfixBytes(), bv("TESTPOSTFIX"));
}

TEST(MappedName, fromRawDataCopyStartposAndSize)
{
    // Arrange
    const std::string testBytes("TESTTEST", 8);
    Data::MappedName temp = Data::MappedName::fromRawData(
        Base::BytesView(testBytes.data(), testBytes.size())
    );
    temp.append("ABCDEFGHIJKLM");  // postfix
    temp.compact();                // Always call compact before accessing data!

    // Act
    Data::MappedName mappedName = Data::MappedName::fromRawData(temp, 2, 13);

    // Assert
    EXPECT_EQ(mappedName.isRaw(), true);
    EXPECT_EQ(mappedName.empty(), false);
    EXPECT_EQ(mappedName.size(), 13);
    EXPECT_EQ(mappedName.dataBytes(), bv("STTEST", 6));
    EXPECT_EQ(mappedName.postfixBytes(), bv("ABCDEFG"));
}

TEST(MappedName, assignmentOperator)
{
    // Arrange
    Data::MappedName temp(Data::MappedName("TEST"), "POSTFIXTEST");

    // Act
    Data::MappedName mappedName = temp;

    // Assert
    EXPECT_EQ(mappedName.isRaw(), false);
    EXPECT_EQ(mappedName.empty(), false);
    EXPECT_EQ(mappedName.size(), 15);
    EXPECT_EQ(mappedName.dataBytes(), bv("TEST"));
    EXPECT_EQ(mappedName.postfixBytes(), bv("POSTFIXTEST"));
}

TEST(MappedName, assignmentOperatorString)
{
    // Arrange
    Data::MappedName mappedName;

    // Act
    mappedName = std::string("TEST");

    // Assert
    EXPECT_EQ(mappedName.isRaw(), false);
    EXPECT_EQ(mappedName.empty(), false);
    EXPECT_EQ(mappedName.size(), 4);
    EXPECT_EQ(mappedName.dataBytes(), bv("TEST"));
    EXPECT_EQ(mappedName.postfixBytes(), Base::BytesView());
}

TEST(MappedName, assignmentOperatorConstCharPtr)
{
    // Arrange
    Data::MappedName mappedName;

    // Act
    mappedName = "TEST";

    // Assert
    EXPECT_EQ(mappedName.isRaw(), false);
    EXPECT_EQ(mappedName.empty(), false);
    EXPECT_EQ(mappedName.size(), 4);
    EXPECT_EQ(mappedName.dataBytes(), bv("TEST"));
    EXPECT_EQ(mappedName.postfixBytes(), Base::BytesView());
}

TEST(MappedName, assignmentOperatorMove)
{
    // Arrange
    Data::MappedName temp(Data::MappedName("TEST"), "POSTFIXTEST");

    // Act
    Data::MappedName mappedName = std::move(temp);

    // Assert
    EXPECT_EQ(mappedName.isRaw(), false);
    EXPECT_EQ(mappedName.empty(), false);
    EXPECT_EQ(mappedName.size(), 15);
    EXPECT_EQ(mappedName.dataBytes(), bv("TEST"));
    EXPECT_EQ(mappedName.postfixBytes(), bv("POSTFIXTEST"));
}

TEST(MappedName, streamInsertionOperator)
{
    // Arrange
    Data::MappedName mappedName(Data::MappedName("TEST"), "POSTFIXTEST");
    std::stringstream ss;

    // Act
    ss << mappedName;

    // Assert
    EXPECT_EQ(ss.str(), std::string("TESTPOSTFIXTEST"));
}

TEST(MappedName, comparisonOperators)
{
    // Arrange
    Data::MappedName mappedName1(Data::MappedName("TEST"), "POSTFIXTEST");
    Data::MappedName mappedName2(Data::MappedName("TEST"), "POSTFIXTEST");
    Data::MappedName mappedName3(Data::MappedName("TESTPOST"), "FIXTEST");
    Data::MappedName mappedName4(Data::MappedName("THIS"), "ISDIFFERENT");

    // Act & Assert
    EXPECT_EQ(mappedName1 == mappedName1, true);
    EXPECT_EQ(mappedName1 == mappedName2, true);
    EXPECT_EQ(mappedName1 == mappedName3, true);
    EXPECT_EQ(mappedName1 == mappedName4, false);

    EXPECT_EQ(mappedName1 != mappedName1, false);
    EXPECT_EQ(mappedName1 != mappedName2, false);
    EXPECT_EQ(mappedName1 != mappedName3, false);
    EXPECT_EQ(mappedName1 != mappedName4, true);
}

TEST(MappedName, additionOperators)
{
    // Arrange
    Data::MappedName mappedName(Data::MappedName("TEST"), "POSTFIXTEST");

    // Act
    mappedName += "POST1";
    mappedName += std::string("POST2");
    mappedName += bv("POST3");
    mappedName += Data::MappedName("POST4");

    // Assert
    EXPECT_EQ(mappedName.isRaw(), false);
    EXPECT_EQ(mappedName.empty(), false);
    EXPECT_EQ(mappedName.size(), 35);
    EXPECT_EQ(mappedName.dataBytes(), bv("TEST"));
    EXPECT_EQ(mappedName.postfixBytes(), bv("POSTFIXTESTPOST1POST2POST3POST4"));

    // Arrange
    mappedName = Data::MappedName(Data::MappedName("TEST"), "POSTFIXTEST");

    // Act
    mappedName = mappedName + Data::MappedName("POST5");
    mappedName = mappedName + "POST6";
    mappedName = mappedName + std::string("POST7");
    mappedName = mappedName + bv("POST8");

    // Assert
    EXPECT_EQ(mappedName.isRaw(), false);
    EXPECT_EQ(mappedName.empty(), false);
    EXPECT_EQ(mappedName.size(), 35);
    EXPECT_EQ(mappedName.dataBytes(), bv("TEST"));
    EXPECT_EQ(mappedName.postfixBytes(), bv("POSTFIXTESTPOST5POST6POST7POST8"));
}

TEST(MappedName, append)
{
    // Arrange
    Data::MappedName mappedName;

    // Act
    mappedName.append("TEST");

    // Assert
    EXPECT_EQ(mappedName.isRaw(), false);
    EXPECT_EQ(mappedName.empty(), false);
    EXPECT_EQ(mappedName.size(), 4);
    EXPECT_EQ(mappedName.dataBytes(), bv("TEST"));
    EXPECT_EQ(mappedName.postfixBytes(), Base::BytesView());

    // Act
    mappedName.append("POSTFIX");

    // Assert
    EXPECT_EQ(mappedName.isRaw(), false);
    EXPECT_EQ(mappedName.empty(), false);
    EXPECT_EQ(mappedName.size(), 11);
    EXPECT_EQ(mappedName.dataBytes(), bv("TEST"));
    EXPECT_EQ(mappedName.postfixBytes(), bv("POSTFIX"));

    // Act
    mappedName.append("ANOTHERPOSTFIX", 5);

    // Assert
    EXPECT_EQ(mappedName.isRaw(), false);
    EXPECT_EQ(mappedName.empty(), false);
    EXPECT_EQ(mappedName.size(), 16);
    EXPECT_EQ(mappedName.dataBytes(), bv("TEST"));
    EXPECT_EQ(mappedName.postfixBytes(), bv("POSTFIXANOTH"));
}

TEST(MappedName, appendMappedNameObj)
{
    // Arrange
    Data::MappedName mappedName;
    Data::MappedName temp(Data::MappedName("TEST"), "POSTFIXTEST");

    // Act
    mappedName.append(temp);

    // Assert
    EXPECT_EQ(mappedName.isRaw(), false);
    EXPECT_EQ(mappedName.empty(), false);
    EXPECT_EQ(mappedName.size(), 15);
    EXPECT_EQ(mappedName.dataBytes(), bv("TEST"));
    EXPECT_EQ(mappedName.postfixBytes(), bv("POSTFIXTEST"));

    // Act
    mappedName.append(temp, 2, 7);

    // Assert
    EXPECT_EQ(mappedName.isRaw(), false);
    EXPECT_EQ(mappedName.empty(), false);
    EXPECT_EQ(mappedName.size(), 22);
    EXPECT_EQ(mappedName.dataBytes(), bv("TEST"));
    EXPECT_EQ(mappedName.postfixBytes(), bv("POSTFIXTESTSTPOSTF"));
}

TEST(MappedName, toString)
{
    // Arrange
    Data::MappedName mappedName(Data::MappedName("TEST"), "POSTFIXTEST");

    // Act & Assert
    EXPECT_EQ(mappedName.toString(0), "TESTPOSTFIXTEST");
    EXPECT_EQ(mappedName.toString(0), std::string("TESTPOSTFIXTEST"));
    EXPECT_EQ(mappedName.toString(2, 8), "STPOSTFI");
    EXPECT_EQ(mappedName.toString(2, 8), std::string("STPOSTFI"));
}

TEST(MappedName, toConstString)
{
    // Arrange
    Data::MappedName mappedName(Data::MappedName("TEST"), "POSTFIXTEST");
    int size {0};

    // Act
    const char* temp = mappedName.toConstString(0, size);

    // Assert
    EXPECT_EQ(std::string(temp, static_cast<std::size_t>(size)), std::string("TEST"));
    EXPECT_EQ(size, 4);

    // Act
    const char* temp2 = mappedName.toConstString(7, size);

    // Assert
    EXPECT_EQ(std::string(temp2, static_cast<std::size_t>(size)), std::string("TFIXTEST"));
    EXPECT_EQ(size, 8);
}

TEST(MappedName, toRawBytes)
{
    // Arrange
    Data::MappedName mappedName(Data::MappedName("TEST"), "POSTFIXTEST");

    // Act & Assert
    EXPECT_EQ(mappedName.toRawBytes().view(), bv("TESTPOSTFIXTEST"));
    EXPECT_EQ(mappedName.toRawBytes(3).view(), bv("TPOSTFIXTEST"));
    EXPECT_EQ(mappedName.toRawBytes(7, 3).view(), bv("TFI"));
    EXPECT_EQ(mappedName.toRawBytes(502, 5).view(), Base::BytesView());
}

TEST(MappedName, toIndexedNameASCIIOnly)
{
    // Arrange
    Data::MappedName mappedName {"MAPPED_NAME"};

    // Act
    auto indexedName = mappedName.toIndexedName();

    // Assert
    EXPECT_FALSE(indexedName.isNull());
}

TEST(MappedName, toIndexedNameInvalid)
{
    // Arrange
    Data::MappedName mappedName {"MAPPED-NAME"};

    // Act
    auto indexedName = mappedName.toIndexedName();

    // Assert
    EXPECT_TRUE(indexedName.isNull());
}

TEST(MappedName, appendToBuffer)
{
    // Arrange
    Data::MappedName mappedName(Data::MappedName("TEST"), "POSTFIXTEST");
    std::string buffer("STUFF");

    // Act
    mappedName.appendToBuffer(buffer);

    // Assert
    EXPECT_EQ(buffer, std::string("STUFFTESTPOSTFIXTEST"));

    // Act
    mappedName.appendToBuffer(buffer, 2, 7);

    // Assert
    EXPECT_EQ(buffer, std::string("STUFFTESTPOSTFIXTESTSTPOSTF"));
}

TEST(MappedName, appendToBufferWithPrefix)
{
    // Arrange
    Data::MappedName mappedName(Data::MappedName("TEST"), "POSTFIXTEST");
    std::string buffer("STUFF");
    std::string elemMapPrefix = Data::ELEMENT_MAP_PREFIX;

    // Act
    mappedName.appendToBufferWithPrefix(buffer);

    // Assert
    EXPECT_EQ(buffer, std::string("STUFF") + elemMapPrefix + std::string("TESTPOSTFIXTEST"));

    // Arrange
    Data::MappedName mappedName2("TEST");  // If mappedName does not have a postfix and is a valid
                                           // indexedName: prefix is not added

    // Act
    mappedName2.appendToBufferWithPrefix(buffer);

    // Assert
    EXPECT_EQ(
        buffer,
        std::string("STUFF") + elemMapPrefix + std::string("TESTPOSTFIXTEST")
            + /*missing prefix*/ std::string("TEST")
    );
}

TEST(MappedName, toPrefixedString)
{
    // Arrange
    Data::MappedName mappedName(Data::MappedName("TEST"), "POSTFIXTEST");
    std::string buffer("STUFF");
    std::string elemMapPrefix = Data::ELEMENT_MAP_PREFIX;

    // Act
    buffer += mappedName.toPrefixedString();

    // Assert
    EXPECT_EQ(buffer, std::string("STUFF") + elemMapPrefix + std::string("TESTPOSTFIXTEST"));

    // Arrange
    Data::MappedName mappedName2("TEST");  // If mappedName does not have a postfix and is a valid
                                           // indexedName: prefix is not added

    // Act
    buffer += mappedName2.toPrefixedString();

    // Assert
    EXPECT_EQ(
        buffer,
        std::string("STUFF") + elemMapPrefix + std::string("TESTPOSTFIXTEST")
            + /*missing prefix*/ std::string("TEST")
    );
}

TEST(MappedName, toBytes)
{
    // Arrange
    Data::MappedName mappedName(Data::MappedName("TEST"), "POSTFIXTEST");

    // Act & Assert
    EXPECT_EQ(mappedName.toBytes().view(), bv("TESTPOSTFIXTEST"));
}

TEST(MappedName, compare)
{
    // Arrange
    Data::MappedName mappedName1(Data::MappedName("TEST"), "POSTFIXTEST");
    Data::MappedName mappedName2(Data::MappedName("TEST"), "POSTFIXTEST");
    Data::MappedName mappedName3(Data::MappedName("TESTPOST"), "FIXTEST");
    Data::MappedName mappedName4(Data::MappedName("THIS"), "ISDIFFERENT");
    Data::MappedName mappedName5(Data::MappedName("SH"), "ORTHER");
    Data::MappedName mappedName6(Data::MappedName("VERYVERYVERY"), "VERYMUCHLONGER");

    // Act & Assert
    EXPECT_EQ(mappedName1.compare(mappedName1), 0);
    EXPECT_EQ(mappedName1.compare(mappedName2), 0);
    EXPECT_EQ(mappedName1.compare(mappedName3), 0);
    EXPECT_EQ(mappedName1.compare(mappedName4), -1);
    EXPECT_EQ(mappedName1.compare(mappedName5), 1);
    EXPECT_EQ(mappedName1.compare(mappedName6), -1);

    EXPECT_EQ(mappedName1 < mappedName1, false);
    EXPECT_EQ(mappedName1 < mappedName2, false);
    EXPECT_EQ(mappedName1 < mappedName3, false);
    EXPECT_EQ(mappedName1 < mappedName4, true);
    EXPECT_EQ(mappedName1 < mappedName5, false);
    EXPECT_EQ(mappedName1 < mappedName6, true);
}

TEST(MappedName, subscriptOperator)
{
    // Arrange
    Data::MappedName mappedName(Data::MappedName("TEST"), "POSTFIXTEST");

    // Act & Assert
    EXPECT_EQ(mappedName[0], 'T');
    EXPECT_EQ(mappedName[1], 'E');
    EXPECT_EQ(mappedName[2], 'S');
    EXPECT_EQ(mappedName[3], 'T');
    EXPECT_EQ(mappedName[4], 'P');
    EXPECT_EQ(mappedName[5], 'O');
    EXPECT_EQ(mappedName[6], 'S');
    EXPECT_EQ(mappedName[7], 'T');
    EXPECT_EQ(mappedName[8], 'F');
    EXPECT_EQ(mappedName[9], 'I');
}

TEST(MappedName, copy)
{
    // Arrange
    Data::MappedName mappedName(Data::MappedName("TEST"), "POSTFIXTEST");

    // Act
    Data::MappedName mappedName2 = mappedName.copy();

    // Assert
    EXPECT_EQ(mappedName, mappedName2);
}

TEST(MappedName, compact)
{
    // Arrange
    Data::MappedName mappedName = Data::MappedName::fromRawData("TESTTEST", 8);

    // Act
    mappedName.compact();

    // Assert
    EXPECT_EQ(mappedName.isRaw(), false);
    EXPECT_EQ(mappedName.empty(), false);
    EXPECT_EQ(mappedName.size(), 8);
    EXPECT_EQ(mappedName.dataBytes(), bv("TESTTEST", 8));
    EXPECT_EQ(mappedName.postfixBytes(), Base::BytesView());
}

TEST(MappedName, boolOperator)
{
    // Arrange
    Data::MappedName mappedName;

    // Act & Assert
    EXPECT_EQ((bool)mappedName, false);

    // Arrange
    mappedName.append("TEST");

    // Act & Assert
    EXPECT_EQ((bool)mappedName, true);
}

TEST(MappedName, clear)
{
    // Arrange
    Data::MappedName mappedName(Data::MappedName("TEST"), "POSTFIXTEST");

    // Act
    mappedName.clear();

    // Assert
    EXPECT_EQ(mappedName.empty(), true);
}

TEST(MappedName, find)
{
    // Arrange
    Data::MappedName mappedName(Data::MappedName("TEST"), "POSTFIXTEST");

    // Act & Assert
    EXPECT_EQ(mappedName.find(nullptr), -1);
    EXPECT_EQ(mappedName.find(""), 0);
    EXPECT_EQ(mappedName.find(std::string("")), 0);
    EXPECT_EQ(mappedName.find("TEST"), 0);
    EXPECT_EQ(mappedName.find("STPO"), -1);  // sentence must be fully contained in data or postfix
    EXPECT_EQ(mappedName.find("POST"), 4);
    EXPECT_EQ(mappedName.find("POST", 4), 4);
    EXPECT_EQ(mappedName.find("POST", 5), -1);

    EXPECT_EQ(mappedName.rfind("ST"), 13);
    EXPECT_EQ(mappedName.rfind("ST", 15), 13);
    EXPECT_EQ(mappedName.rfind("ST", 14), 13);
    EXPECT_EQ(mappedName.rfind("ST", 13), 13);
    EXPECT_EQ(mappedName.rfind("ST", 12), 6);
    EXPECT_EQ(mappedName.rfind("ST", 11), 6);
    EXPECT_EQ(mappedName.rfind("ST", 10), 6);
    EXPECT_EQ(mappedName.rfind("ST", 9), 6);
    EXPECT_EQ(mappedName.rfind("ST", 8), 6);
    EXPECT_EQ(mappedName.rfind("ST", 7), 6);
    EXPECT_EQ(mappedName.rfind("ST", 6), 6);
    EXPECT_EQ(mappedName.rfind("ST", 5), 2);
    EXPECT_EQ(mappedName.rfind("ST", 4), 2);
    EXPECT_EQ(mappedName.rfind("ST", 3), 2);
    EXPECT_EQ(mappedName.rfind("ST", 2), 2);
    EXPECT_EQ(mappedName.rfind("ST", 1), -1);
    EXPECT_EQ(mappedName.rfind("ST", 0), -1);
}

TEST(MappedName, findPreservesEmbeddedNullBytes)
{
    // Arrange
    const std::string name {"AA\0BB", 5};
    const std::string needle {"A\0B", 3};
    const Data::MappedName mappedName(name);

    // Act & Assert
    EXPECT_EQ(mappedName.find(Base::BytesView(needle.data(), needle.size())), 1);
    EXPECT_EQ(mappedName.find(needle), 1);
    EXPECT_EQ(mappedName.find(needle, 2), -1);
}

TEST(MappedName, rfind)
{
    // Arrange
    Data::MappedName mappedName(Data::MappedName("TEST"), "POSTFIXTEST");

    // Act & Assert
    EXPECT_EQ(mappedName.rfind(nullptr), -1);
    EXPECT_EQ(mappedName.rfind(""), mappedName.size());
    EXPECT_EQ(mappedName.rfind(std::string("")), mappedName.size());
    EXPECT_EQ(mappedName.rfind("TEST"), 11);
    EXPECT_EQ(mappedName.rfind("STPO"), -1);  // sentence must be fully contained in data or postfix
    EXPECT_EQ(mappedName.rfind("POST"), 4);
    EXPECT_EQ(mappedName.rfind("POST", 4), 4);
    EXPECT_EQ(mappedName.rfind("POST", 3), -1);

    EXPECT_EQ(mappedName.rfind("ST"), 13);
    EXPECT_EQ(mappedName.rfind("ST", 0), -1);
    EXPECT_EQ(mappedName.rfind("ST", 1), -1);
    EXPECT_EQ(mappedName.rfind("ST", 2), 2);
    EXPECT_EQ(mappedName.rfind("ST", 3), 2);
    EXPECT_EQ(mappedName.rfind("ST", 4), 2);
    EXPECT_EQ(mappedName.rfind("ST", 5), 2);
    EXPECT_EQ(mappedName.rfind("ST", 6), 6);
    EXPECT_EQ(mappedName.rfind("ST", 7), 6);
    EXPECT_EQ(mappedName.rfind("ST", 8), 6);
    EXPECT_EQ(mappedName.rfind("ST", 9), 6);
    EXPECT_EQ(mappedName.rfind("ST", 10), 6);
    EXPECT_EQ(mappedName.rfind("ST", 11), 6);
    EXPECT_EQ(mappedName.rfind("ST", 12), 6);
    EXPECT_EQ(mappedName.rfind("ST", 13), 13);
    EXPECT_EQ(mappedName.rfind("ST", 14), 13);
    EXPECT_EQ(mappedName.rfind("ST", 15), 13);
}

TEST(MappedName, rfindPreservesEmbeddedNullBytes)
{
    // Arrange
    const std::string name {"AB\0CDB", 6};
    const std::string needle {"B\0C", 3};
    const Data::MappedName mappedName(name);

    // Act & Assert
    EXPECT_EQ(mappedName.rfind(Base::BytesView(needle.data(), needle.size())), 1);
    EXPECT_EQ(mappedName.rfind(needle), 1);
    EXPECT_EQ(mappedName.rfind(needle, 0), -1);
}

TEST(MappedName, endswith)
{
    // Arrange
    Data::MappedName mappedName("TEST");

    // Act & Assert
    EXPECT_EQ(mappedName.endsWith(nullptr), false);
    EXPECT_EQ(mappedName.endsWith("TEST"), true);
    EXPECT_EQ(mappedName.endsWith(std::string("TEST")), true);
    EXPECT_EQ(mappedName.endsWith("WASD"), false);

    // Arrange
    mappedName.append("POSTFIX");

    // Act & Assert
    EXPECT_EQ(mappedName.endsWith(nullptr), false);
    EXPECT_EQ(mappedName.endsWith("TEST"), false);
    EXPECT_EQ(mappedName.endsWith("FIX"), true);
}

TEST(MappedName, endsWithPreservesEmbeddedNullBytes)
{
    // Arrange
    const std::string suffix {"A\0BB", 4};
    Data::MappedName mappedName("TEST");
    mappedName.append(suffix.data(), static_cast<int>(suffix.size()));

    // Act & Assert
    EXPECT_EQ(mappedName.endsWith(Base::BytesView(suffix.data(), suffix.size())), true);
    EXPECT_EQ(mappedName.endsWith(suffix), true);
}

TEST(MappedName, startsWith)
{
    // Arrange
    Data::MappedName mappedName;

    // Act & Assert
    EXPECT_EQ(mappedName.startsWith(nullptr), false);
    EXPECT_EQ(mappedName.startsWith(Base::BytesView()), true);
    EXPECT_EQ(mappedName.startsWith(""), true);
    EXPECT_EQ(mappedName.startsWith(std::string("")), true);
    EXPECT_EQ(mappedName.startsWith("WASD"), false);

    // Arrange
    mappedName.append("TEST");

    // Act & Assert
    EXPECT_EQ(mappedName.startsWith(nullptr), false);
    EXPECT_EQ(mappedName.startsWith(Base::BytesView()), true);
    EXPECT_EQ(mappedName.startsWith("TEST"), true);
    EXPECT_EQ(mappedName.startsWith(std::string("TEST")), true);
    EXPECT_EQ(mappedName.startsWith("WASD"), false);
}

TEST(MappedName, findTagInElementNameHexPositiveIndexNonrecursive)
{
    // The non-recursive version will find just the last tag, prefixed by the POSTFIX_TAG (";:H").
    // It consists of a tag (stored in hexadecimal) and a length (also stored in hex), separated by
    // a colon. In this example, we expect the get ;:H1b:10,F as our element, which is a tag of
    // 0x1b and an indicated length of 0x10 (16 bytes). So the output length is the position of the
    // tag (36) minus the indicated length, giving 20.

    // Arrange
    Data::MappedName mappedName("#94;:G0;XTR;:H19:8,F;:H1a,F;BND:-1:0;:H1b:10,F");
    long tagOutput {0};
    int lenOutput {0};
    std::string postfix;
    char type {0};

    // Act
    int result = mappedName.findTagInElementName(&tagOutput, &lenOutput, &postfix, &type, false, false);

    // Assert
    EXPECT_EQ(result, 36);       // The location of the tag
    EXPECT_EQ(tagOutput, 0x1b);  // The tag
    EXPECT_EQ(lenOutput, 20);    // The calculated length based on the tag's length parameter
    EXPECT_EQ(postfix, ";:H1b:10,F");
    EXPECT_EQ(type, 'F');  // F=Face
}

TEST(MappedName, findTagInElementNameDecPositiveIndexNonrecursive)
{
    // Test backwards compatibility with the older style decimal tag storage.

    // Arrange
    Data::MappedName mappedName("#94;:G0;XTR;:T19:8,F;:T26,F;BND:-1:0;:T27:16,F");
    long tagOutput {0};
    int lenOutput {0};
    std::string postfix;
    char type {0};

    // Act
    int result = mappedName.findTagInElementName(&tagOutput, &lenOutput, &postfix, &type, false, false);

    // Assert
    EXPECT_EQ(result, 36);     // The location of the tag
    EXPECT_EQ(tagOutput, 27);  // The tag
    EXPECT_EQ(lenOutput, 16);  // The specified length
    EXPECT_EQ(postfix, ";:T27:16,F");
    EXPECT_EQ(type, 'F');  // F=Face
}

TEST(MappedName, findTagInElementNameHexNegativeIndexNonrecursive)
{
    // Test handling a negative index that is flipped to positive

    // Arrange
    Data::MappedName mappedName("#94;:G0;XTR;:H19:8,F;:H1a,F;BND:-1:0;:H-1b:10,F");
    long tagOutput {0};
    int lenOutput {0};
    std::string postfix;
    char type {0};

    // Act
    int result = mappedName.findTagInElementName(&tagOutput, &lenOutput, &postfix, &type, false, false);

    // Assert
    EXPECT_EQ(result, 36);       // The location of the tag
    EXPECT_EQ(tagOutput, 0x1b);  // The tag is returned positive, even though it was input negative
    EXPECT_EQ(lenOutput, 20);    // The calculated length based on the tag's length parameter
    EXPECT_EQ(postfix, ";:H-1b:10,F");
    EXPECT_EQ(type, 'F');  // F=Face
}

TEST(MappedName, findTagInElementNameHexExpectedNegativeIndexNonrecursive)
{
    // Test handling an untransformed negative index

    // Arrange
    Data::MappedName mappedName("#94;:G0;XTR;:H19:8,F;:H1a,F;BND:-1:0;:H-1b:10,F");
    long tagOutput {0};
    int lenOutput {0};
    std::string postfix;
    char type {0};

    // Act
    int result = mappedName.findTagInElementName(&tagOutput, &lenOutput, &postfix, &type, true, false);

    // Assert
    EXPECT_EQ(result, 36);        // The location of the tag
    EXPECT_EQ(tagOutput, -0x1b);  // The tag is returned negative
    EXPECT_EQ(lenOutput, 20);     // The calculated length based on the tag's length parameter
    EXPECT_EQ(postfix, ";:H-1b:10,F");
    EXPECT_EQ(type, 'F');  // F=Face
}

TEST(MappedName, findTagInElementNameRecursive)
{
    // Test the recursive resolution of the name

    // Arrange
    Data::MappedName mappedName("#94;:G0;XTR;:H19:8,F;:H1a,F;BND:-1:0;:H1b:10,F");
    long tagOutput {0};
    int lenOutput {0};
    std::string postfix;
    char type {0};

    // Act
    int result = mappedName.findTagInElementName(&tagOutput, &lenOutput, &postfix, &type, false, true);

    // Assert
    EXPECT_EQ(result, 36);       // The location of the tag
    EXPECT_EQ(tagOutput, 0x1b);  // The tag
    EXPECT_EQ(lenOutput, 27);    // Now includes the next tag, from the recursive search
    EXPECT_EQ(postfix, ";:H1b:10,F");
    EXPECT_EQ(type, 'F');  // F=Face
}

// NOLINTEND(readability-magic-numbers)
