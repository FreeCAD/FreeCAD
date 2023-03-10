// SPDX-License-Identifier: LGPL-2.1-or-later

#include "gtest/gtest.h"

#include "App/MappedName.h"
#include "App/ComplexGeoData.h"

#include <string>



// clang-format off
TEST(MappedName, defaultConstruction)
{
    auto mappedName = Data::MappedName();
    EXPECT_EQ(mappedName.isRaw(), false);
    EXPECT_EQ(mappedName.empty(), true);
    EXPECT_EQ(mappedName.size(), 0);
}

TEST(MappedName, namedConstruction)
{
    auto mappedName = Data::MappedName("TEST");
    EXPECT_EQ(mappedName.isRaw(), false);
    EXPECT_EQ(mappedName.empty(), false);
    EXPECT_EQ(mappedName.size(), 4);
    EXPECT_EQ(mappedName.dataBytes(), QByteArray("TEST"));
    EXPECT_EQ(mappedName.postfixBytes(), QByteArray());
}

TEST(MappedName, namedConstructionWithMaxSize)
{
    auto mappedName = Data::MappedName("TEST", 2);
    EXPECT_EQ(mappedName.isRaw(), false);
    EXPECT_EQ(mappedName.empty(), false);
    EXPECT_EQ(mappedName.size(), 2);
    EXPECT_EQ(mappedName.dataBytes(), QByteArray("TE"));
    EXPECT_EQ(mappedName.postfixBytes(), QByteArray());
}

TEST(MappedName, namedConstructionDiscardPrefix)
{
    std::string name = Data::ComplexGeoData::elementMapPrefix() + "TEST";
    auto mappedName = Data::MappedName(name.c_str());
    EXPECT_EQ(mappedName.isRaw(), false);
    EXPECT_EQ(mappedName.empty(), false);
    EXPECT_EQ(mappedName.size(), 4);
    EXPECT_EQ(mappedName.dataBytes(), QByteArray("TEST"));
    EXPECT_EQ(mappedName.postfixBytes(), QByteArray());

}

TEST(MappedName, stringNamedConstruction)
{
    auto mappedName = Data::MappedName(std::string("TEST"));
    EXPECT_EQ(mappedName.isRaw(), false);
    EXPECT_EQ(mappedName.empty(), false);
    EXPECT_EQ(mappedName.size(), 4);
    EXPECT_EQ(mappedName.dataBytes(), QByteArray("TEST"));
    EXPECT_EQ(mappedName.postfixBytes(), QByteArray());
}

TEST(MappedName, stringNamedConstructionDiscardPrefix)
{
    std::string name = Data::ComplexGeoData::elementMapPrefix() + "TEST";
    auto mappedName = Data::MappedName(name);
    EXPECT_EQ(mappedName.isRaw(), false);
    EXPECT_EQ(mappedName.empty(), false);
    EXPECT_EQ(mappedName.size(), 4);
    EXPECT_EQ(mappedName.dataBytes(), QByteArray("TEST"));
    EXPECT_EQ(mappedName.postfixBytes(), QByteArray());

}

TEST(MappedName, copyConstructor)
{
    auto temp = Data::MappedName("TEST");
    auto mappedName = Data::MappedName(temp);
    EXPECT_EQ(mappedName.isRaw(), false);
    EXPECT_EQ(mappedName.empty(), false);
    EXPECT_EQ(mappedName.size(), 4);
    EXPECT_EQ(mappedName.dataBytes(), QByteArray("TEST"));
    EXPECT_EQ(mappedName.postfixBytes(), QByteArray());

}

TEST(MappedName, copyConstructorWithPostfix)
{
    auto temp = Data::MappedName("TEST");
    auto mappedName = Data::MappedName(temp, "POSTFIXTEST");
    EXPECT_EQ(mappedName.isRaw(), false);
    EXPECT_EQ(mappedName.empty(), false);
    EXPECT_EQ(mappedName.size(), 15);
    EXPECT_EQ(mappedName.dataBytes(), QByteArray("TEST"));
    EXPECT_EQ(mappedName.postfixBytes(), QByteArray("POSTFIXTEST"));
}

TEST(MappedName, constructorWithPostfixAndCopy)
{
    auto temp = Data::MappedName(Data::MappedName("TEST"), "POSTFIXTEST");
    auto mappedName = Data::MappedName(temp, "ANOTHERPOSTFIX");
    EXPECT_EQ(mappedName.isRaw(), false);
    EXPECT_EQ(mappedName.empty(), false);
    EXPECT_EQ(mappedName.size(), 29);
    EXPECT_EQ(mappedName.dataBytes(), QByteArray("TESTPOSTFIXTEST"));
    EXPECT_EQ(mappedName.postfixBytes(), QByteArray("ANOTHERPOSTFIX"));
}

TEST(MappedName, copyConstructorStartpos)
{
    auto temp = Data::MappedName(Data::MappedName("TEST"), "POSTFIXTEST");
    auto mappedName = Data::MappedName(temp, 2, -1);
    EXPECT_EQ(mappedName.isRaw(), false);
    EXPECT_EQ(mappedName.empty(), false);
    EXPECT_EQ(mappedName.size(), 13);
    EXPECT_EQ(mappedName.dataBytes(), QByteArray("ST"));
    EXPECT_EQ(mappedName.postfixBytes(), QByteArray("POSTFIXTEST"));
}

TEST(MappedName, copyConstructorStartposAndSize)
{
    auto temp = Data::MappedName(Data::MappedName("TEST"), "POSTFIXTEST");
    auto mappedName = Data::MappedName(temp, 2, 6);
    EXPECT_EQ(mappedName.isRaw(), false);
    EXPECT_EQ(mappedName.empty(), false);
    EXPECT_EQ(mappedName.size(), 6);
    EXPECT_EQ(mappedName.dataBytes(), QByteArray("ST"));
    EXPECT_EQ(mappedName.postfixBytes(), QByteArray("POST"));
}

#if QT_VERSION  >= 0x050200
TEST(MappedName, moveConstructor)
{
    auto temp = Data::MappedName(Data::MappedName("TEST"), "POSTFIXTEST");
    auto mappedName = Data::MappedName(std::move(temp));
    EXPECT_EQ(mappedName.isRaw(), false);
    EXPECT_EQ(mappedName.empty(), false);
    EXPECT_EQ(mappedName.size(), 15);
    EXPECT_EQ(mappedName.dataBytes(), QByteArray("TEST"));
    EXPECT_EQ(mappedName.postfixBytes(), QByteArray("POSTFIXTEST"));

    EXPECT_EQ(temp.isRaw(), false);
    EXPECT_EQ(temp.empty(), true);
    EXPECT_EQ(temp.size(), 0);
    EXPECT_EQ(temp.dataBytes(), QByteArray());
    EXPECT_EQ(temp.postfixBytes(), QByteArray());
}
#endif

TEST(MappedName, fromRawData)
{
    auto mappedName = Data::MappedName::fromRawData("TEST\0\0TEST", 10);
    EXPECT_EQ(mappedName.isRaw(), true);
    EXPECT_EQ(mappedName.empty(), false);
    EXPECT_EQ(mappedName.size(), 10);
    EXPECT_EQ(mappedName.dataBytes(), QByteArray("TEST\0\0TEST", 10));
    EXPECT_EQ(mappedName.postfixBytes(), QByteArray());
}

TEST(MappedName, fromRawDataQByteArray)
{
    auto mappedName = Data::MappedName::fromRawData(QByteArray("TEST\0\0TEST", 10));
    EXPECT_EQ(mappedName.isRaw(), true);
    EXPECT_EQ(mappedName.empty(), false);
    EXPECT_EQ(mappedName.size(), 10);
    EXPECT_EQ(mappedName.dataBytes(), QByteArray("TEST\0\0TEST", 10));
    EXPECT_EQ(mappedName.postfixBytes(), QByteArray());
}

TEST(MappedName, fromRawDataCopy)
{
    auto temp = Data::MappedName::fromRawData(QByteArray("TEST\0\0TEST", 10));
    temp.append("TESTPOSTFIX");
    auto mappedName = Data::MappedName::fromRawData(temp, 0);
    EXPECT_EQ(mappedName.isRaw(), true);
    EXPECT_EQ(mappedName.empty(), false);
    EXPECT_EQ(mappedName.size(), 21);
    EXPECT_EQ(mappedName.dataBytes(), QByteArray("TEST\0\0TEST", 10));
    EXPECT_EQ(mappedName.postfixBytes(), QByteArray("TESTPOSTFIX"));
}


TEST(MappedName, fromRawDataCopyStartposAndSize)
{
    auto temp = Data::MappedName::fromRawData(QByteArray("TEST\0\0TEST", 10));
    temp.append("ABCDEFGHIJKLM"); //postfix
/*  This block is OK 
    EXPECT_EQ(temp.isRaw(), true);
    EXPECT_EQ(temp.empty(), false);
    EXPECT_EQ(temp.size(), 23);
    EXPECT_EQ(temp.dataBytes(), QByteArray("TEST\0\0TEST", 10));
    EXPECT_EQ(temp.postfixBytes(), QByteArray("ABCDEFGHIJKLM"));
*/

    auto mappedName = Data::MappedName::fromRawData(temp, 2, 13);
    EXPECT_EQ(mappedName.isRaw(), true);
    EXPECT_EQ(mappedName.empty(), false);
    EXPECT_EQ(mappedName.size(), 13);
    //next line fails with CDE\0TEST != ST\0\0TEST
    //funny thing if i uncomment the block above, which does nothing, now the next line
    //fails with CDE\0GHIJ != ST\0\0TEST
    EXPECT_EQ(mappedName.dataBytes(), QByteArray("ST\0\0TEST", 8)); 
    EXPECT_EQ(mappedName.postfixBytes(), QByteArray("ABCDE"));
}

//TODO raw postfix?



// clang-format on