// SPDX-License-Identifier: LGPL-2.1-or-later

#include "gtest/gtest.h"

#include "App/MappedName.h"
#include "App/ComplexGeoData.h"

#include <string>
#include <sstream>



// clang-format off
TEST(MappedName, defaultConstruction)
{
    Data::MappedName mappedName = Data::MappedName();
    EXPECT_EQ(mappedName.isRaw(), false);
    EXPECT_EQ(mappedName.empty(), true);
    EXPECT_EQ(mappedName.size(), 0);
}

TEST(MappedName, namedConstruction)
{
    Data::MappedName mappedName = Data::MappedName("TEST");
    EXPECT_EQ(mappedName.isRaw(), false);
    EXPECT_EQ(mappedName.empty(), false);
    EXPECT_EQ(mappedName.size(), 4);
    EXPECT_EQ(mappedName.dataBytes(), QByteArray("TEST"));
    EXPECT_EQ(mappedName.postfixBytes(), QByteArray());
}

TEST(MappedName, namedConstructionWithMaxSize)
{
    Data::MappedName mappedName = Data::MappedName("TEST", 2);
    EXPECT_EQ(mappedName.isRaw(), false);
    EXPECT_EQ(mappedName.empty(), false);
    EXPECT_EQ(mappedName.size(), 2);
    EXPECT_EQ(mappedName.dataBytes(), QByteArray("TE"));
    EXPECT_EQ(mappedName.postfixBytes(), QByteArray());
}

TEST(MappedName, namedConstructionDiscardPrefix)
{
    std::string name = Data::ComplexGeoData::elementMapPrefix() + "TEST";
    Data::MappedName mappedName = Data::MappedName(name.c_str());
    EXPECT_EQ(mappedName.isRaw(), false);
    EXPECT_EQ(mappedName.empty(), false);
    EXPECT_EQ(mappedName.size(), 4);
    EXPECT_EQ(mappedName.dataBytes(), QByteArray("TEST"));
    EXPECT_EQ(mappedName.postfixBytes(), QByteArray());

}

TEST(MappedName, stringNamedConstruction)
{
    Data::MappedName mappedName = Data::MappedName(std::string("TEST"));
    EXPECT_EQ(mappedName.isRaw(), false);
    EXPECT_EQ(mappedName.empty(), false);
    EXPECT_EQ(mappedName.size(), 4);
    EXPECT_EQ(mappedName.dataBytes(), QByteArray("TEST"));
    EXPECT_EQ(mappedName.postfixBytes(), QByteArray());
}

TEST(MappedName, stringNamedConstructionDiscardPrefix)
{
    std::string name = Data::ComplexGeoData::elementMapPrefix() + "TEST";
    Data::MappedName mappedName = Data::MappedName(name);
    EXPECT_EQ(mappedName.isRaw(), false);
    EXPECT_EQ(mappedName.empty(), false);
    EXPECT_EQ(mappedName.size(), 4);
    EXPECT_EQ(mappedName.dataBytes(), QByteArray("TEST"));
    EXPECT_EQ(mappedName.postfixBytes(), QByteArray());

}

TEST(MappedName, copyConstructor)
{
    Data::MappedName temp = Data::MappedName("TEST");
    Data::MappedName mappedName = Data::MappedName(temp);
    EXPECT_EQ(mappedName.isRaw(), false);
    EXPECT_EQ(mappedName.empty(), false);
    EXPECT_EQ(mappedName.size(), 4);
    EXPECT_EQ(mappedName.dataBytes(), QByteArray("TEST"));
    EXPECT_EQ(mappedName.postfixBytes(), QByteArray());

}

TEST(MappedName, copyConstructorWithPostfix)
{
    Data::MappedName temp = Data::MappedName("TEST");
    Data::MappedName mappedName = Data::MappedName(temp, "POSTFIXTEST");
    EXPECT_EQ(mappedName.isRaw(), false);
    EXPECT_EQ(mappedName.empty(), false);
    EXPECT_EQ(mappedName.size(), 15);
    EXPECT_EQ(mappedName.dataBytes(), QByteArray("TEST"));
    EXPECT_EQ(mappedName.postfixBytes(), QByteArray("POSTFIXTEST"));
}

TEST(MappedName, constructorWithPostfixAndCopy)
{
    Data::MappedName temp = Data::MappedName(Data::MappedName("TEST"), "POSTFIXTEST");
    Data::MappedName mappedName = Data::MappedName(temp, "ANOTHERPOSTFIX");
    EXPECT_EQ(mappedName.isRaw(), false);
    EXPECT_EQ(mappedName.empty(), false);
    EXPECT_EQ(mappedName.size(), 29);
    EXPECT_EQ(mappedName.dataBytes(), QByteArray("TESTPOSTFIXTEST"));
    EXPECT_EQ(mappedName.postfixBytes(), QByteArray("ANOTHERPOSTFIX"));
}

TEST(MappedName, copyConstructorStartpos)
{
    Data::MappedName temp = Data::MappedName(Data::MappedName("TEST"), "POSTFIXTEST");
    Data::MappedName mappedName = Data::MappedName(temp, 2, -1);
    EXPECT_EQ(mappedName.isRaw(), false);
    EXPECT_EQ(mappedName.empty(), false);
    EXPECT_EQ(mappedName.size(), 13);
    EXPECT_EQ(mappedName.dataBytes(), QByteArray("ST"));
    EXPECT_EQ(mappedName.postfixBytes(), QByteArray("POSTFIXTEST"));
}

TEST(MappedName, copyConstructorStartposAndSize)
{
    Data::MappedName temp = Data::MappedName(Data::MappedName("TEST"), "POSTFIXTEST");
    Data::MappedName mappedName = Data::MappedName(temp, 2, 6);
    EXPECT_EQ(mappedName.isRaw(), false);
    EXPECT_EQ(mappedName.empty(), false);
    EXPECT_EQ(mappedName.size(), 6);
    EXPECT_EQ(mappedName.dataBytes(), QByteArray("ST"));
    EXPECT_EQ(mappedName.postfixBytes(), QByteArray("POST"));
}

TEST(MappedName, moveConstructor)
{
    Data::MappedName temp = Data::MappedName(Data::MappedName("TEST"), "POSTFIXTEST");
    Data::MappedName mappedName = Data::MappedName(std::move(temp));
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

TEST(MappedName, fromRawData)
{
    Data::MappedName mappedName = Data::MappedName::fromRawData("TEST\0\0TEST", 10);
    EXPECT_EQ(mappedName.isRaw(), true);
    EXPECT_EQ(mappedName.empty(), false);
    EXPECT_EQ(mappedName.size(), 10);
    EXPECT_EQ(mappedName.dataBytes(), QByteArray("TEST\0\0TEST", 10));
    EXPECT_EQ(mappedName.postfixBytes(), QByteArray());
}

TEST(MappedName, fromRawDataQByteArray)
{
    Data::MappedName mappedName = Data::MappedName::fromRawData(QByteArray("TEST\0\0TEST", 10));
    EXPECT_EQ(mappedName.isRaw(), true);
    EXPECT_EQ(mappedName.empty(), false);
    EXPECT_EQ(mappedName.size(), 10);
    EXPECT_EQ(mappedName.dataBytes(), QByteArray("TEST\0\0TEST", 10));
    EXPECT_EQ(mappedName.postfixBytes(), QByteArray());
}

TEST(MappedName, fromRawDataCopy)
{
    Data::MappedName temp = Data::MappedName::fromRawData(QByteArray("TEST\0\0TEST", 10));
    temp.append("TESTPOSTFIX");
    Data::MappedName mappedName = Data::MappedName::fromRawData(temp, 0);
    EXPECT_EQ(mappedName.isRaw(), true);
    EXPECT_EQ(mappedName.empty(), false);
    EXPECT_EQ(mappedName.size(), 21);
    EXPECT_EQ(mappedName.dataBytes(), QByteArray("TEST\0\0TEST", 10));
    EXPECT_EQ(mappedName.postfixBytes(), QByteArray("TESTPOSTFIX"));
}


TEST(MappedName, fromRawDataCopyStartposAndSize)
{
    Data::MappedName temp = Data::MappedName::fromRawData(QByteArray("TEST\0\0TEST", 10));
    temp.append("ABCDEFGHIJKLM"); //postfix
/*  This block is OK 
    EXPECT_EQ(temp.isRaw(), true);
    EXPECT_EQ(temp.empty(), false);
    EXPECT_EQ(temp.size(), 23);
    EXPECT_EQ(temp.dataBytes(), QByteArray("TEST\0\0TEST", 10));
    EXPECT_EQ(temp.postfixBytes(), QByteArray("ABCDEFGHIJKLM"));
*/

    Data::MappedName mappedName = Data::MappedName::fromRawData(temp, 2, 13);
    EXPECT_EQ(mappedName.isRaw(), true);
    EXPECT_EQ(mappedName.empty(), false);
    EXPECT_EQ(mappedName.size(), 13);
    //next line fails with CDE\0TEST != ST\0\0TEST
    //funny thing if i uncomment the block above, which does nothing, now the next line
    //fails with CDE\0GHIJ != ST\0\0TEST
    EXPECT_EQ(mappedName.dataBytes(), QByteArray("ST\0\0TEST", 8)); 
    EXPECT_EQ(mappedName.postfixBytes(), QByteArray("ABCDE"));
}

//TODO raw postfix? answer: apparently postfix will never be raw. See copy()

TEST(MappedName, assignmentOperator)
{
    Data::MappedName temp = Data::MappedName(Data::MappedName("TEST"), "POSTFIXTEST");
    Data::MappedName mappedName = temp;
    EXPECT_EQ(mappedName.isRaw(), false);
    EXPECT_EQ(mappedName.empty(), false);
    EXPECT_EQ(mappedName.size(), 15);
    EXPECT_EQ(mappedName.dataBytes(), QByteArray("TEST"));
    EXPECT_EQ(mappedName.postfixBytes(), QByteArray("POSTFIXTEST"));
}

TEST(MappedName, assignmentOperatorString)
{
    Data::MappedName mappedName;
    mappedName = std::string("TEST");
    EXPECT_EQ(mappedName.isRaw(), false);
    EXPECT_EQ(mappedName.empty(), false);
    EXPECT_EQ(mappedName.size(), 4);
    EXPECT_EQ(mappedName.dataBytes(), QByteArray("TEST"));
    EXPECT_EQ(mappedName.postfixBytes(), QByteArray());
}

TEST(MappedName, assignmentOperatorConstCharPtr)
{
    Data::MappedName mappedName;
    mappedName = "TEST";
    EXPECT_EQ(mappedName.isRaw(), false);
    EXPECT_EQ(mappedName.empty(), false);
    EXPECT_EQ(mappedName.size(), 4);
    EXPECT_EQ(mappedName.dataBytes(), QByteArray("TEST"));
    EXPECT_EQ(mappedName.postfixBytes(), QByteArray());
}

TEST(MappedName, operatorEqualMove)
{
    Data::MappedName temp = Data::MappedName(Data::MappedName("TEST"), "POSTFIXTEST");
    Data::MappedName mappedName = std::move(temp);
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

TEST(MappedName, streamInsertionOperator)
{
    Data::MappedName mappedName = Data::MappedName(Data::MappedName("TEST"), "POSTFIXTEST");

    EXPECT_EQ(mappedName.isRaw(), false);
    EXPECT_EQ(mappedName.empty(), false);
    EXPECT_EQ(mappedName.size(), 15);
    EXPECT_EQ(mappedName.dataBytes(), QByteArray("TEST"));
    EXPECT_EQ(mappedName.postfixBytes(), QByteArray("POSTFIXTEST"));

    std::stringstream ss;
    ss << mappedName;
    EXPECT_EQ(ss.str(), std::string("TESTPOSTFIXTEST"));
}


TEST(MappedName, comparisonOperators)
{
    Data::MappedName mappedName1 = Data::MappedName(Data::MappedName("TEST"), "POSTFIXTEST");
    Data::MappedName mappedName2 = Data::MappedName(Data::MappedName("TEST"), "POSTFIXTEST");
    Data::MappedName mappedName3 = Data::MappedName(Data::MappedName("TESTPOST"), "FIXTEST");
    Data::MappedName mappedName4 = Data::MappedName(Data::MappedName("THIS"), "ISDIFFERENT");

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
    Data::MappedName mappedName1 = Data::MappedName(Data::MappedName("TEST"), "POSTFIXTEST");
    mappedName1 += "POST1";
    mappedName1 += std::string("POST2");
    mappedName1 += QByteArray("POST3");
    mappedName1 += Data::MappedName("POST4");

    EXPECT_EQ(mappedName1.isRaw(), false);
    EXPECT_EQ(mappedName1.empty(), false);
    EXPECT_EQ(mappedName1.size(), 35);
    EXPECT_EQ(mappedName1.dataBytes(), QByteArray("TEST"));
    EXPECT_EQ(mappedName1.postfixBytes(), QByteArray("POSTFIXTESTPOST1POST2POST3POST4"));

    mappedName1 = Data::MappedName(Data::MappedName("TEST"), "POSTFIXTEST");
    mappedName1 = mappedName1 + Data::MappedName("POST5");
    mappedName1 = mappedName1 + "POST6";
    mappedName1 = mappedName1 + std::string("POST7");
    mappedName1 = mappedName1 + QByteArray("POST8");

    EXPECT_EQ(mappedName1.isRaw(), false);
    EXPECT_EQ(mappedName1.empty(), false);
    EXPECT_EQ(mappedName1.size(), 35);
    EXPECT_EQ(mappedName1.dataBytes(), QByteArray("TEST"));
    EXPECT_EQ(mappedName1.postfixBytes(), QByteArray("POSTFIXTESTPOST5POST6POST7POST8"));
}


TEST(MappedName, append)
{
    Data::MappedName mappedName = Data::MappedName();
    mappedName.append("TEST");
    EXPECT_EQ(mappedName.isRaw(), false);
    EXPECT_EQ(mappedName.empty(), false);
    EXPECT_EQ(mappedName.size(), 4);
    EXPECT_EQ(mappedName.dataBytes(), QByteArray("TEST"));
    EXPECT_EQ(mappedName.postfixBytes(), QByteArray(""));

    mappedName.append("POSTFIX");
    EXPECT_EQ(mappedName.isRaw(), false);
    EXPECT_EQ(mappedName.empty(), false);
    EXPECT_EQ(mappedName.size(), 11);
    EXPECT_EQ(mappedName.dataBytes(), QByteArray("TEST"));
    EXPECT_EQ(mappedName.postfixBytes(), QByteArray("POSTFIX"));

    mappedName.append("ANOTHERPOSTFIX", 5);
    EXPECT_EQ(mappedName.isRaw(), false);
    EXPECT_EQ(mappedName.empty(), false);
    EXPECT_EQ(mappedName.size(), 16);
    EXPECT_EQ(mappedName.dataBytes(), QByteArray("TEST"));
    EXPECT_EQ(mappedName.postfixBytes(), QByteArray("POSTFIXANOTH"));
}


TEST(MappedName, appendMappedNameObj)
{
    Data::MappedName mappedName = Data::MappedName();

    mappedName.append(Data::MappedName(Data::MappedName("TEST"), "POSTFIXTEST"));
    EXPECT_EQ(mappedName.isRaw(), false);
    EXPECT_EQ(mappedName.empty(), false);
    EXPECT_EQ(mappedName.size(), 15);
    EXPECT_EQ(mappedName.dataBytes(), QByteArray("TEST"));
    EXPECT_EQ(mappedName.postfixBytes(), QByteArray("POSTFIXTEST"));

    mappedName.append(Data::MappedName(Data::MappedName("TEST"), "POSTFIXTEST"), 2, 7);
    EXPECT_EQ(mappedName.isRaw(), false);
    EXPECT_EQ(mappedName.empty(), false);
    EXPECT_EQ(mappedName.size(), 22);
    EXPECT_EQ(mappedName.dataBytes(), QByteArray("TEST"));
    EXPECT_EQ(mappedName.postfixBytes(), QByteArray("POSTFIXTESTSTPOSTF"));
}

TEST(MappedName, toString)
{
    Data::MappedName mappedName = Data::MappedName(Data::MappedName("TEST"), "POSTFIXTEST");
    EXPECT_EQ(mappedName.toString(0), "TESTPOSTFIXTEST");
    EXPECT_EQ(mappedName.toString(0), std::string("TESTPOSTFIXTEST"));
    EXPECT_EQ(mappedName.toString(2, 8), "STPOSTFI");
    EXPECT_EQ(mappedName.toString(2, 8), std::string("STPOSTFI"));
}


TEST(MappedName, toConstString)
{
    Data::MappedName mappedName = Data::MappedName(Data::MappedName("TEST"), "POSTFIXTEST");
    int size;
    const char *temp = mappedName.toConstString(0, size);
    EXPECT_EQ(QByteArray(temp, size), QByteArray("TEST"));
    EXPECT_EQ(size, 4);
    const char *temp2 = mappedName.toConstString(7, size);
    EXPECT_EQ(QByteArray(temp2, size), QByteArray("TFIXTEST"));
    EXPECT_EQ(size, 8);
}

TEST(MappedName, toRawBytes)
{
    Data::MappedName mappedName = Data::MappedName(Data::MappedName("TEST"), "POSTFIXTEST");
    EXPECT_EQ(mappedName.toRawBytes(), QByteArray("TESTPOSTFIXTEST"));
    EXPECT_EQ(mappedName.toRawBytes(3), QByteArray("TPOSTFIXTEST"));
    EXPECT_EQ(mappedName.toRawBytes(7, 3), QByteArray("TFI"));
    EXPECT_EQ(mappedName.toRawBytes(502, 5), QByteArray());
}

TEST(MappedName, toBytes)
{
    Data::MappedName mappedName = Data::MappedName(Data::MappedName("TEST"), "POSTFIXTEST");
    EXPECT_EQ(mappedName.toBytes(), QByteArray("TESTPOSTFIXTEST"));
}


TEST(MappedName, compare)
{
    Data::MappedName mappedName1 = Data::MappedName(Data::MappedName("TEST"), "POSTFIXTEST");
    Data::MappedName mappedName2 = Data::MappedName(Data::MappedName("TEST"), "POSTFIXTEST");
    Data::MappedName mappedName3 = Data::MappedName(Data::MappedName("TESTPOST"), "FIXTEST");
    Data::MappedName mappedName4 = Data::MappedName(Data::MappedName("THIS"), "ISDIFFERENT");
    Data::MappedName mappedName5 = Data::MappedName(Data::MappedName("SH"), "ORTHER");
    Data::MappedName mappedName6 = Data::MappedName(Data::MappedName("VERYVERYVERY"), "VERYMUCHLONGER");

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

TEST(MappedName, indexOperator)
{
    Data::MappedName mappedName = Data::MappedName(Data::MappedName("TEST"), "POSTFIXTEST");
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
    Data::MappedName mappedName = Data::MappedName(Data::MappedName("TEST"), "POSTFIXTEST");
    Data::MappedName mappedName2 = mappedName.copy();
    EXPECT_EQ(mappedName, mappedName2);
}


TEST(MappedName, compact)
{
    Data::MappedName mappedName = Data::MappedName::fromRawData("TEST\0\0TEST", 10);
    EXPECT_EQ(mappedName.isRaw(), true);
    EXPECT_EQ(mappedName.empty(), false);
    EXPECT_EQ(mappedName.size(), 10);
    EXPECT_EQ(mappedName.dataBytes(), QByteArray("TEST\0\0TEST", 10));
    EXPECT_EQ(mappedName.postfixBytes(), QByteArray());

    mappedName.compact();
    EXPECT_EQ(mappedName.isRaw(), false);
    EXPECT_EQ(mappedName.empty(), false);
    EXPECT_EQ(mappedName.size(), 10);
    EXPECT_EQ(mappedName.dataBytes(), QByteArray("TEST\0\0TEST", 10));
    EXPECT_EQ(mappedName.postfixBytes(), QByteArray());
}

TEST(MappedName, boolOperator)
{
    Data::MappedName mappedName = Data::MappedName();
    EXPECT_EQ((bool)mappedName, false);
    mappedName.append("TEST");
    EXPECT_EQ((bool)mappedName, true);
}

TEST(MappedName, clear)
{
    Data::MappedName mappedName = Data::MappedName(Data::MappedName("TEST"), "POSTFIXTEST");
    EXPECT_EQ(mappedName.empty(), false);
    mappedName.clear();
    EXPECT_EQ(mappedName.empty(), true);
}

TEST(MappedName, find)
{
    Data::MappedName mappedName = Data::MappedName(Data::MappedName("TEST"), "POSTFIXTEST");
    EXPECT_EQ(mappedName.find(nullptr), -1);
    EXPECT_EQ(mappedName.find(""), 0);
    EXPECT_EQ(mappedName.find("TEST"), 0);
    EXPECT_EQ(mappedName.find("STPO"), -1); //sentence must be fully contained in data or postfix
    EXPECT_EQ(mappedName.find("POST"), 4);
    EXPECT_EQ(mappedName.find("ST", 3), 6); //found in postfix
    EXPECT_EQ(mappedName.find("POST", 4), 4);
    EXPECT_EQ(mappedName.find("POST", 5), -1);

    EXPECT_EQ(mappedName.find(std::string("")), 0);
}


TEST(MappedName, rfind)
{
    Data::MappedName mappedName = Data::MappedName(Data::MappedName("TEST"), "POSTFIXTEST");
    EXPECT_EQ(mappedName.rfind(nullptr), -1);
    EXPECT_EQ(mappedName.rfind(""), mappedName.size());
    EXPECT_EQ(mappedName.rfind("TEST"), 11);
    EXPECT_EQ(mappedName.rfind("STPO"), -1); //sentence must be fully contained in data or postfix
    EXPECT_EQ(mappedName.rfind("POST"), 4);


    //FIXME looks broken
    EXPECT_EQ(mappedName.rfind("ST"), 13); 
    EXPECT_EQ(mappedName.rfind("ST", 0), -1); 
    EXPECT_EQ(mappedName.rfind("ST", 1), -1); 
    EXPECT_EQ(mappedName.rfind("ST", 2), 2); 
    EXPECT_EQ(mappedName.rfind("ST", 3), 2); 
    EXPECT_EQ(mappedName.rfind("ST", 4), 2); 
    EXPECT_EQ(mappedName.rfind("ST", 5), -1);
    EXPECT_EQ(mappedName.rfind("ST", 6), -1);
    EXPECT_EQ(mappedName.rfind("ST", 7), -1);
    EXPECT_EQ(mappedName.rfind("ST", 8), -1);
    EXPECT_EQ(mappedName.rfind("ST", 9), -1);
    EXPECT_EQ(mappedName.rfind("ST", 10), -1); 
    EXPECT_EQ(mappedName.rfind("ST", 11), -1); 
    EXPECT_EQ(mappedName.rfind("ST", 12), 2); 
    EXPECT_EQ(mappedName.rfind("ST", 13), 6); 
    EXPECT_EQ(mappedName.rfind("ST", 14), 6); 
    EXPECT_EQ(mappedName.rfind("ST", 15), 6); 
    EXPECT_EQ(mappedName.rfind("ST", 16), 6); 
    EXPECT_EQ(mappedName.rfind("ST", 17), 6); 
    EXPECT_EQ(mappedName.rfind("ST", 18), 6); 
    EXPECT_EQ(mappedName.rfind("ST", 19), 6); 
    EXPECT_EQ(mappedName.rfind("ST", 20), 13); 
    EXPECT_EQ(mappedName.rfind("ST", 21), 13); 
    EXPECT_EQ(mappedName.rfind("ST", 22), 13); 
    EXPECT_EQ(mappedName.rfind("ST", 23), 2); 
    EXPECT_EQ(mappedName.rfind("ST", 24), 2); 
    EXPECT_EQ(mappedName.rfind("ST", 25), 2); 
    EXPECT_EQ(mappedName.rfind("ST", 26), 2); 
    EXPECT_EQ(mappedName.rfind("ST", 27), 2); 
    EXPECT_EQ(mappedName.rfind("ST", 28), 2); 
    //EXPECT_EQ(mappedName.rfind("POST", 7), 4);
    //EXPECT_EQ(mappedName.rfind("POST", 8), -1);
    
    EXPECT_EQ(mappedName.rfind(std::string("")), mappedName.size());
}

TEST(MappedName, endswith)
{
    Data::MappedName mappedName = Data::MappedName("TEST");
    EXPECT_EQ(mappedName.endsWith(nullptr), false);
    EXPECT_EQ(mappedName.endsWith("TEST"), true);
    EXPECT_EQ(mappedName.endsWith("WASD"), false); 

    EXPECT_EQ(mappedName.endsWith(std::string("TEST")), true);

    mappedName.append("POSTFIX");

    EXPECT_EQ(mappedName.endsWith(nullptr), false);
    EXPECT_EQ(mappedName.endsWith("TEST"), false);
    EXPECT_EQ(mappedName.endsWith("FIX"), true); 
}


TEST(MappedName, startsWith)
{
    Data::MappedName mappedName = Data::MappedName("TEST");
    EXPECT_EQ(mappedName.startsWith(QByteArray()), true);
    EXPECT_EQ(mappedName.startsWith("TEST"), true);
    EXPECT_EQ(mappedName.startsWith("WASD"), false); 

    EXPECT_EQ(mappedName.startsWith(nullptr), false);
    EXPECT_EQ(mappedName.startsWith("TEST"), true);
    EXPECT_EQ(mappedName.startsWith(std::string("TEST")), true);
}

//TODO test hash function

// clang-format on