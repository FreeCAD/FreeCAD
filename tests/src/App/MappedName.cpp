// SPDX-License-Identifier: LGPL-2.1-or-later

#include "gtest/gtest.h"

#include "App/MappedName.h"
#include "App/ComplexGeoData.h"

#include <string>
#include <sstream>



// clang-format off
TEST(MappedName, defaultConstruction)
{
    // Act
    Data::MappedName mappedName = Data::MappedName();

    // Assert
    EXPECT_EQ(mappedName.isRaw(), false);
    EXPECT_EQ(mappedName.empty(), true);
    EXPECT_EQ(mappedName.size(), 0);
    EXPECT_EQ(mappedName.dataBytes(), QByteArray());
    EXPECT_EQ(mappedName.postfixBytes(), QByteArray());
}

TEST(MappedName, namedConstruction)
{
    // Act
    Data::MappedName mappedName = Data::MappedName("TEST");

    // Assert
    EXPECT_EQ(mappedName.isRaw(), false);
    EXPECT_EQ(mappedName.empty(), false);
    EXPECT_EQ(mappedName.size(), 4);
    EXPECT_EQ(mappedName.dataBytes(), QByteArray("TEST"));
    EXPECT_EQ(mappedName.postfixBytes(), QByteArray());
}

TEST(MappedName, namedConstructionWithMaxSize)
{
    // Act
    Data::MappedName mappedName = Data::MappedName("TEST", 2);
    
    // Assert
    EXPECT_EQ(mappedName.isRaw(), false);
    EXPECT_EQ(mappedName.empty(), false);
    EXPECT_EQ(mappedName.size(), 2);
    EXPECT_EQ(mappedName.dataBytes(), QByteArray("TE"));
    EXPECT_EQ(mappedName.postfixBytes(), QByteArray());
}

TEST(MappedName, namedConstructionDiscardPrefix)
{
    // Arrange
    std::string name = Data::ComplexGeoData::elementMapPrefix() + "TEST";
    
    // Act
    Data::MappedName mappedName = Data::MappedName(name.c_str());
        
    // Assert
    EXPECT_EQ(mappedName.isRaw(), false);
    EXPECT_EQ(mappedName.empty(), false);
    EXPECT_EQ(mappedName.size(), 4);
    EXPECT_EQ(mappedName.dataBytes(), QByteArray("TEST"));
    EXPECT_EQ(mappedName.postfixBytes(), QByteArray());
}

TEST(MappedName, stringNamedConstruction)
{
    // Act
    Data::MappedName mappedName = Data::MappedName(std::string("TEST"));
        
    // Assert
    EXPECT_EQ(mappedName.isRaw(), false);
    EXPECT_EQ(mappedName.empty(), false);
    EXPECT_EQ(mappedName.size(), 4);
    EXPECT_EQ(mappedName.dataBytes(), QByteArray("TEST"));
    EXPECT_EQ(mappedName.postfixBytes(), QByteArray());
}

TEST(MappedName, stringNamedConstructionDiscardPrefix)
{
    // Arrange
    std::string name = Data::ComplexGeoData::elementMapPrefix() + "TEST";
    
    // Act
    Data::MappedName mappedName = Data::MappedName(name);
        
    // Assert
    EXPECT_EQ(mappedName.isRaw(), false);
    EXPECT_EQ(mappedName.empty(), false);
    EXPECT_EQ(mappedName.size(), 4);
    EXPECT_EQ(mappedName.dataBytes(), QByteArray("TEST"));
    EXPECT_EQ(mappedName.postfixBytes(), QByteArray());
}

TEST(MappedName, copyConstructor)
{
    // Arrange
    Data::MappedName temp = Data::MappedName("TEST");
    
    // Act
    Data::MappedName mappedName = Data::MappedName(temp);
        
    // Assert
    EXPECT_EQ(mappedName.isRaw(), false);
    EXPECT_EQ(mappedName.empty(), false);
    EXPECT_EQ(mappedName.size(), 4);
    EXPECT_EQ(mappedName.dataBytes(), QByteArray("TEST"));
    EXPECT_EQ(mappedName.postfixBytes(), QByteArray());
}

TEST(MappedName, copyConstructorWithPostfix)
{
    // Arrange
    Data::MappedName temp = Data::MappedName("TEST");

    // Act
    Data::MappedName mappedName = Data::MappedName(temp, "POSTFIXTEST");
        
    // Assert
    EXPECT_EQ(mappedName.isRaw(), false);
    EXPECT_EQ(mappedName.empty(), false);
    EXPECT_EQ(mappedName.size(), 15);
    EXPECT_EQ(mappedName.dataBytes(), QByteArray("TEST"));
    EXPECT_EQ(mappedName.postfixBytes(), QByteArray("POSTFIXTEST"));

    // Arrange
    temp = Data::MappedName(Data::MappedName("TEST"), "POSTFIXTEST");
    
    // Act
    mappedName = Data::MappedName(temp, "ANOTHERPOSTFIX");
        
    // Assert
    EXPECT_EQ(mappedName.isRaw(), false);
    EXPECT_EQ(mappedName.empty(), false);
    EXPECT_EQ(mappedName.size(), 29);
    EXPECT_EQ(mappedName.dataBytes(), QByteArray("TESTPOSTFIXTEST"));
    EXPECT_EQ(mappedName.postfixBytes(), QByteArray("ANOTHERPOSTFIX"));
}

TEST(MappedName, copyConstructorStartpos)
{
    // Arrange
    Data::MappedName temp = Data::MappedName(Data::MappedName("TEST"), "POSTFIXTEST");
    
    // Act
    Data::MappedName mappedName = Data::MappedName(temp, 2, -1);
        
    // Assert
    EXPECT_EQ(mappedName.isRaw(), false);
    EXPECT_EQ(mappedName.empty(), false);
    EXPECT_EQ(mappedName.size(), 13);
    EXPECT_EQ(mappedName.dataBytes(), QByteArray("ST"));
    EXPECT_EQ(mappedName.postfixBytes(), QByteArray("POSTFIXTEST"));
}

TEST(MappedName, copyConstructorStartposAndSize)
{
    // Arrange
    Data::MappedName temp = Data::MappedName(Data::MappedName("TEST"), "POSTFIXTEST");
    
    // Act
    Data::MappedName mappedName = Data::MappedName(temp, 2, 6);
        
    // Assert
    EXPECT_EQ(mappedName.isRaw(), false);
    EXPECT_EQ(mappedName.empty(), false);
    EXPECT_EQ(mappedName.size(), 6);
    EXPECT_EQ(mappedName.dataBytes(), QByteArray("ST"));
    EXPECT_EQ(mappedName.postfixBytes(), QByteArray("POST"));
}

TEST(MappedName, moveConstructor)
{
    // Arrange
    Data::MappedName temp = Data::MappedName(Data::MappedName("TEST"), "POSTFIXTEST");
    
    // Act
    Data::MappedName mappedName = Data::MappedName(std::move(temp));
        
    // Assert
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
    // Act
    Data::MappedName mappedName = Data::MappedName::fromRawData("TESTTEST", 10);
        
    // Assert
    EXPECT_EQ(mappedName.isRaw(), true);
    EXPECT_EQ(mappedName.empty(), false);
    EXPECT_EQ(mappedName.size(), 10);
    EXPECT_EQ(mappedName.dataBytes(), QByteArray("TESTTEST", 10));
    EXPECT_EQ(mappedName.postfixBytes(), QByteArray());
}

TEST(MappedName, fromRawDataQByteArray)
{
    // Act
    Data::MappedName mappedName = Data::MappedName::fromRawData(QByteArray("TESTTEST", 10));
        
    // Assert
    EXPECT_EQ(mappedName.isRaw(), true);
    EXPECT_EQ(mappedName.empty(), false);
    EXPECT_EQ(mappedName.size(), 10);
    EXPECT_EQ(mappedName.dataBytes(), QByteArray("TESTTEST", 10));
    EXPECT_EQ(mappedName.postfixBytes(), QByteArray());
}

TEST(MappedName, fromRawDataCopy)
{
    // Arrange
    Data::MappedName temp = Data::MappedName::fromRawData(QByteArray("TESTTEST", 10));
    temp.append("TESTPOSTFIX");
    
    // Act
    Data::MappedName mappedName = Data::MappedName::fromRawData(temp, 0);
        
    // Assert
    EXPECT_EQ(mappedName.isRaw(), true);
    EXPECT_EQ(mappedName.empty(), false);
    EXPECT_EQ(mappedName.size(), 21);
    EXPECT_EQ(mappedName.dataBytes(), QByteArray("TESTTEST", 10));
    EXPECT_EQ(mappedName.postfixBytes(), QByteArray("TESTPOSTFIX"));
}

TEST(MappedName, fromRawDataCopyStartposAndSize) 
{
    // Arrange
    Data::MappedName temp = Data::MappedName::fromRawData(QByteArray("TESTTEST", 8));
    temp.append("ABCDEFGHIJKLM"); //postfix
    temp.compact(); //Always call compact before accessing data!

    // Act
    Data::MappedName mappedName = Data::MappedName::fromRawData(temp, 2, 13);

    // Assert
    EXPECT_EQ(mappedName.isRaw(), true);
    EXPECT_EQ(mappedName.empty(), false);
    EXPECT_EQ(mappedName.size(), 13);
    EXPECT_EQ(mappedName.dataBytes(), QByteArray("STTEST", 6)); 
    EXPECT_EQ(mappedName.postfixBytes(), QByteArray("ABCDEFG"));
}

TEST(MappedName, assignmentOperator)
{
    // Arrange
    Data::MappedName temp = Data::MappedName(Data::MappedName("TEST"), "POSTFIXTEST");
    
    // Act
    Data::MappedName mappedName = temp;
        
    // Assert
    EXPECT_EQ(mappedName.isRaw(), false);
    EXPECT_EQ(mappedName.empty(), false);
    EXPECT_EQ(mappedName.size(), 15);
    EXPECT_EQ(mappedName.dataBytes(), QByteArray("TEST"));
    EXPECT_EQ(mappedName.postfixBytes(), QByteArray("POSTFIXTEST"));
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
    EXPECT_EQ(mappedName.dataBytes(), QByteArray("TEST"));
    EXPECT_EQ(mappedName.postfixBytes(), QByteArray());
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
    EXPECT_EQ(mappedName.dataBytes(), QByteArray("TEST"));
    EXPECT_EQ(mappedName.postfixBytes(), QByteArray());
}

TEST(MappedName, assignmentOperatorMove)
{
    // Arrange
    Data::MappedName temp = Data::MappedName(Data::MappedName("TEST"), "POSTFIXTEST");

    // Act
    Data::MappedName mappedName = std::move(temp);
        
    // Assert
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
    // Arrange
    Data::MappedName mappedName = Data::MappedName(Data::MappedName("TEST"), "POSTFIXTEST");

    // Act
    std::stringstream ss;
    ss << mappedName;

    // Assert
    EXPECT_EQ(ss.str(), std::string("TESTPOSTFIXTEST"));
}

TEST(MappedName, comparisonOperators)
{
    // Arrange
    Data::MappedName mappedName1 = Data::MappedName(Data::MappedName("TEST"), "POSTFIXTEST");
    Data::MappedName mappedName2 = Data::MappedName(Data::MappedName("TEST"), "POSTFIXTEST");
    Data::MappedName mappedName3 = Data::MappedName(Data::MappedName("TESTPOST"), "FIXTEST");
    Data::MappedName mappedName4 = Data::MappedName(Data::MappedName("THIS"), "ISDIFFERENT");
       
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
    Data::MappedName mappedName = Data::MappedName(Data::MappedName("TEST"), "POSTFIXTEST");
    
    // Act
    mappedName += "POST1";
    mappedName += std::string("POST2");
    mappedName += QByteArray("POST3");
    mappedName += Data::MappedName("POST4");
        
    // Assert
    EXPECT_EQ(mappedName.isRaw(), false);
    EXPECT_EQ(mappedName.empty(), false);
    EXPECT_EQ(mappedName.size(), 35);
    EXPECT_EQ(mappedName.dataBytes(), QByteArray("TEST"));
    EXPECT_EQ(mappedName.postfixBytes(), QByteArray("POSTFIXTESTPOST1POST2POST3POST4"));

    // Arrange
    mappedName = Data::MappedName(Data::MappedName("TEST"), "POSTFIXTEST");

    // Act
    mappedName = mappedName + Data::MappedName("POST5");
    mappedName = mappedName + "POST6";
    mappedName = mappedName + std::string("POST7");
    mappedName = mappedName + QByteArray("POST8");
        
    // Assert
    EXPECT_EQ(mappedName.isRaw(), false);
    EXPECT_EQ(mappedName.empty(), false);
    EXPECT_EQ(mappedName.size(), 35);
    EXPECT_EQ(mappedName.dataBytes(), QByteArray("TEST"));
    EXPECT_EQ(mappedName.postfixBytes(), QByteArray("POSTFIXTESTPOST5POST6POST7POST8"));
}

TEST(MappedName, append)
{
    // Arrange
    Data::MappedName mappedName = Data::MappedName();

    // Act
    mappedName.append("TEST");
        
    // Assert
    EXPECT_EQ(mappedName.isRaw(), false);
    EXPECT_EQ(mappedName.empty(), false);
    EXPECT_EQ(mappedName.size(), 4);
    EXPECT_EQ(mappedName.dataBytes(), QByteArray("TEST"));
    EXPECT_EQ(mappedName.postfixBytes(), QByteArray(""));

    // Act
    mappedName.append("POSTFIX");
        
    // Assert
    EXPECT_EQ(mappedName.isRaw(), false);
    EXPECT_EQ(mappedName.empty(), false);
    EXPECT_EQ(mappedName.size(), 11);
    EXPECT_EQ(mappedName.dataBytes(), QByteArray("TEST"));
    EXPECT_EQ(mappedName.postfixBytes(), QByteArray("POSTFIX"));

    // Act
    mappedName.append("ANOTHERPOSTFIX", 5);
        
    // Assert
    EXPECT_EQ(mappedName.isRaw(), false);
    EXPECT_EQ(mappedName.empty(), false);
    EXPECT_EQ(mappedName.size(), 16);
    EXPECT_EQ(mappedName.dataBytes(), QByteArray("TEST"));
    EXPECT_EQ(mappedName.postfixBytes(), QByteArray("POSTFIXANOTH"));
}

TEST(MappedName, appendMappedNameObj)
{
    // Arrange
    Data::MappedName mappedName = Data::MappedName();
    Data::MappedName temp = Data::MappedName(Data::MappedName("TEST"), "POSTFIXTEST");

    // Act
    mappedName.append(temp);
        
    // Assert
    EXPECT_EQ(mappedName.isRaw(), false);
    EXPECT_EQ(mappedName.empty(), false);
    EXPECT_EQ(mappedName.size(), 15);
    EXPECT_EQ(mappedName.dataBytes(), QByteArray("TEST"));
    EXPECT_EQ(mappedName.postfixBytes(), QByteArray("POSTFIXTEST"));

    // Act
    mappedName.append(temp, 2, 7);
        
    // Assert
    EXPECT_EQ(mappedName.isRaw(), false);
    EXPECT_EQ(mappedName.empty(), false);
    EXPECT_EQ(mappedName.size(), 22);
    EXPECT_EQ(mappedName.dataBytes(), QByteArray("TEST"));
    EXPECT_EQ(mappedName.postfixBytes(), QByteArray("POSTFIXTESTSTPOSTF"));
}

TEST(MappedName, toString)
{
    // Arrange
    Data::MappedName mappedName = Data::MappedName(Data::MappedName("TEST"), "POSTFIXTEST");
        
    // Act & Assert
    EXPECT_EQ(mappedName.toString(0), "TESTPOSTFIXTEST");
    EXPECT_EQ(mappedName.toString(0), std::string("TESTPOSTFIXTEST"));
    EXPECT_EQ(mappedName.toString(2, 8), "STPOSTFI");
    EXPECT_EQ(mappedName.toString(2, 8), std::string("STPOSTFI"));
}

TEST(MappedName, toConstString)
{
    // Arrange
    Data::MappedName mappedName = Data::MappedName(Data::MappedName("TEST"), "POSTFIXTEST");
    int size;

    // Act
    const char *temp = mappedName.toConstString(0, size);
    
    // Assert
    EXPECT_EQ(QByteArray(temp, size), QByteArray("TEST"));
    EXPECT_EQ(size, 4);

    // Act
    const char *temp2 = mappedName.toConstString(7, size);

    // Assert
    EXPECT_EQ(QByteArray(temp2, size), QByteArray("TFIXTEST"));
    EXPECT_EQ(size, 8);
}

TEST(MappedName, toRawBytes)
{
    // Arrange
    Data::MappedName mappedName = Data::MappedName(Data::MappedName("TEST"), "POSTFIXTEST");
        
    // Act & Assert
    EXPECT_EQ(mappedName.toRawBytes(), QByteArray("TESTPOSTFIXTEST"));
    EXPECT_EQ(mappedName.toRawBytes(3), QByteArray("TPOSTFIXTEST"));
    EXPECT_EQ(mappedName.toRawBytes(7, 3), QByteArray("TFI"));
    EXPECT_EQ(mappedName.toRawBytes(502, 5), QByteArray());
}

TEST(MappedName, toBytes)
{
    // Arrange
    Data::MappedName mappedName = Data::MappedName(Data::MappedName("TEST"), "POSTFIXTEST");
        
    // Act & Assert
    EXPECT_EQ(mappedName.toBytes(), QByteArray("TESTPOSTFIXTEST"));
}

TEST(MappedName, compare)
{
    // Arrange
    Data::MappedName mappedName1 = Data::MappedName(Data::MappedName("TEST"), "POSTFIXTEST");
    Data::MappedName mappedName2 = Data::MappedName(Data::MappedName("TEST"), "POSTFIXTEST");
    Data::MappedName mappedName3 = Data::MappedName(Data::MappedName("TESTPOST"), "FIXTEST");
    Data::MappedName mappedName4 = Data::MappedName(Data::MappedName("THIS"), "ISDIFFERENT");
    Data::MappedName mappedName5 = Data::MappedName(Data::MappedName("SH"), "ORTHER");
    Data::MappedName mappedName6 = Data::MappedName(Data::MappedName("VERYVERYVERY"), "VERYMUCHLONGER");
        
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
    Data::MappedName mappedName = Data::MappedName(Data::MappedName("TEST"), "POSTFIXTEST");
        
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
    Data::MappedName mappedName = Data::MappedName(Data::MappedName("TEST"), "POSTFIXTEST");
    
    // Act
    Data::MappedName mappedName2 = mappedName.copy();
        
    // Assert
    EXPECT_EQ(mappedName, mappedName2);
}

TEST(MappedName, compact)
{
    // Arrange
    Data::MappedName mappedName = Data::MappedName::fromRawData("TESTTEST", 10);

    // Act
    mappedName.compact();
        
    // Assert
    EXPECT_EQ(mappedName.isRaw(), false);
    EXPECT_EQ(mappedName.empty(), false);
    EXPECT_EQ(mappedName.size(), 10);
    EXPECT_EQ(mappedName.dataBytes(), QByteArray("TESTTEST", 10));
    EXPECT_EQ(mappedName.postfixBytes(), QByteArray());
}

TEST(MappedName, boolOperator)
{
    // Arrange
    Data::MappedName mappedName = Data::MappedName();
        
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
    Data::MappedName mappedName = Data::MappedName(Data::MappedName("TEST"), "POSTFIXTEST");

    // Act
    mappedName.clear();
        
    // Assert
    EXPECT_EQ(mappedName.empty(), true);
}

TEST(MappedName, find)
{
    // Arrange
    Data::MappedName mappedName = Data::MappedName(Data::MappedName("TEST"), "POSTFIXTEST");
        
    // Act & Assert
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
    // Arrange
    Data::MappedName mappedName = Data::MappedName(Data::MappedName("TEST"), "POSTFIXTEST");
        
    // Act & Assert
    EXPECT_EQ(mappedName.rfind(nullptr), -1);
    EXPECT_EQ(mappedName.rfind(""), mappedName.size());
    EXPECT_EQ(mappedName.rfind("TEST"), 11);
    EXPECT_EQ(mappedName.rfind("STPO"), -1); //sentence must be fully contained in data or postfix
    EXPECT_EQ(mappedName.rfind("POST"), 4);

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

    EXPECT_EQ(mappedName.rfind("POST", 4), 4);
    EXPECT_EQ(mappedName.rfind("POST", 3), -1);
    
    EXPECT_EQ(mappedName.rfind(std::string("")), mappedName.size());
}

TEST(MappedName, endswith)
{
    // Arrange
    Data::MappedName mappedName = Data::MappedName("TEST");
        
    // Act & Assert
    EXPECT_EQ(mappedName.endsWith(nullptr), false);
    EXPECT_EQ(mappedName.endsWith("TEST"), true);
    EXPECT_EQ(mappedName.endsWith("WASD"), false); 

    EXPECT_EQ(mappedName.endsWith(std::string("TEST")), true);

    // Arrange
    mappedName.append("POSTFIX");
   
    // Act & Assert
    EXPECT_EQ(mappedName.endsWith(nullptr), false);
    EXPECT_EQ(mappedName.endsWith("TEST"), false);
    EXPECT_EQ(mappedName.endsWith("FIX"), true); 
}

TEST(MappedName, startsWith)
{
    // Arrange
    Data::MappedName mappedName = Data::MappedName("TEST");
        
    // Act & Assert
    EXPECT_EQ(mappedName.startsWith(QByteArray()), true);
    EXPECT_EQ(mappedName.startsWith("TEST"), true);
    EXPECT_EQ(mappedName.startsWith("WASD"), false); 

    EXPECT_EQ(mappedName.startsWith(nullptr), false);
    EXPECT_EQ(mappedName.startsWith("TEST"), true);
    EXPECT_EQ(mappedName.startsWith(std::string("TEST")), true);
}

//TODO test hash function
//TODO test indexedName functions


// clang-format on