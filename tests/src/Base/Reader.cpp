// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#ifdef _MSC_VER
#pragma warning(disable : 4996)
#endif

#include "Base/Exception.h"
#include "Base/Reader.h"
#include <array>
#include <filesystem>
#include <fstream>
#include <random>
#include <string>
#include <xercesc/util/PlatformUtils.hpp>
#include <QString>

namespace fs = std::filesystem;

static std::string random_string(size_t length)
{
    const std::string digits = "0123456789";

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, static_cast<int>(digits.size()) - 1);

    std::string result;
    for (size_t i = 0; i < length; ++i) {
        result += digits[dis(gen)];
    }

    return result;
}

class ReaderXML
{
public:
    ReaderXML()
    {
        _tempDir = fs::temp_directory_path();
        fs::path filename =
            std::string("unit_test_Reader-") + random_string(4) + std::string(".xml");
        _tempFile = _tempDir / filename;
    }
    ~ReaderXML()
    {
        if (inputStream.is_open()) {
            inputStream.close();
        }
        if (fs::exists(_tempFile)) {
            fs::remove(_tempFile);
        }
    }

    Base::XMLReader* Reader()
    {
        return _reader.get();
    }

    void givenDataAsXMLStream(const std::string& data)
    {
        auto stringData =
            R"(<?xml version="1.0" encoding="UTF-8"?><document>)" + data + "</document>";
        std::istringstream stream(stringData);
        std::ofstream fileStream(_tempFile.string());
        fileStream.write(stringData.data(), static_cast<std::streamsize>(stringData.length()));
        fileStream.close();
        inputStream.open(_tempFile.string());
        _reader = std::make_unique<Base::XMLReader>(_tempFile.string().c_str(), inputStream);
    }

private:
    std::unique_ptr<Base::XMLReader> _reader;
    fs::path _tempDir;
    fs::path _tempFile;
    std::ifstream inputStream;
};

class ReaderTest: public ::testing::Test
{
protected:
    void SetUp() override
    {
        XERCES_CPP_NAMESPACE::XMLPlatformUtils::Initialize();
    }

    void TearDown() override
    {}
};

TEST_F(ReaderTest, beginCharStreamNormal)
{
    // Arrange
    ReaderXML xml;
    xml.givenDataAsXMLStream("<data>Test ASCII data</data>");
    xml.Reader()->readElement("data");

    // Act
    auto& result = xml.Reader()->beginCharStream();

    // Assert
    EXPECT_TRUE(result.good());
}

TEST_F(ReaderTest, beginCharStreamOpenClose)
{
    // Arrange
    ReaderXML xml;
    xml.givenDataAsXMLStream("<data id='12345' />");
    xml.Reader()->readElement("data");

    // Act
    auto& result = xml.Reader()->beginCharStream();  // Not an error, even though there is no data

    // Assert
    EXPECT_TRUE(result.good());
}

TEST_F(ReaderTest, beginCharStreamAlreadyBegun)
{
    // Arrange
    ReaderXML xml;
    xml.givenDataAsXMLStream("<data>Test ASCII data</data>");
    xml.Reader()->readElement("data");
    xml.Reader()->beginCharStream();

    // Act & Assert
    EXPECT_THROW(xml.Reader()->beginCharStream(), Base::XMLParseException);  // NOLINT
}

TEST_F(ReaderTest, charStreamGood)
{
    // Arrange
    ReaderXML xml;
    xml.givenDataAsXMLStream("<data>Test ASCII data</data>");
    xml.Reader()->readElement("data");
    xml.Reader()->beginCharStream();

    // Act
    auto& result = xml.Reader()->charStream();

    // Assert
    EXPECT_TRUE(result.good());
}

TEST_F(ReaderTest, charStreamBad)
{
    // Arrange
    ReaderXML xml;
    xml.givenDataAsXMLStream("<data>Test ASCII data</data>");
    xml.Reader()->readElement("data");

    // Act & Assert
    EXPECT_THROW(xml.Reader()->charStream(), Base::XMLParseException);  // NOLINT
}

TEST_F(ReaderTest, endCharStreamGood)
{
    // Arrange
    ReaderXML xml;
    xml.givenDataAsXMLStream("<data>Test ASCII data</data>");
    xml.Reader()->readElement("data");
    xml.Reader()->beginCharStream();

    // Act & Assert
    xml.Reader()->endCharStream();  // Does not throw
}

TEST_F(ReaderTest, endCharStreamBad)
{
    // Arrange
    ReaderXML xml;
    xml.givenDataAsXMLStream("<data>Test ASCII data</data>");
    xml.Reader()->readElement("data");
    // Do not open the stream...

    // Act & Assert
    xml.Reader()->endCharStream();  // Does not throw, even with no open stream
}

TEST_F(ReaderTest, readDataSmallerThanBuffer)
{
    // Arrange
    constexpr size_t bufferSize {20};
    std::string expectedData {"Test ASCII data"};
    ReaderXML xml;
    xml.givenDataAsXMLStream("<data>" + expectedData + "</data>");
    xml.Reader()->readElement("data");
    xml.Reader()->beginCharStream();
    std::array<char, bufferSize> buffer {};

    // Act
    auto bytesRead = xml.Reader()->read(buffer.data(), bufferSize);

    // Assert
    EXPECT_STREQ(expectedData.c_str(), buffer.data());
    EXPECT_EQ(expectedData.length(), bytesRead);
}

TEST_F(ReaderTest, readDataLargerThanBuffer)
{
    // Arrange
    constexpr size_t bufferSize {5};
    std::string expectedData {"Test ASCII data"};
    ReaderXML xml;
    xml.givenDataAsXMLStream("<data>" + expectedData + "</data>");
    xml.Reader()->readElement("data");
    xml.Reader()->beginCharStream();
    std::array<char, bufferSize> buffer {};

    // Act
    auto bytesRead = xml.Reader()->read(buffer.data(), bufferSize);

    // Assert
    for (size_t i = 0; i < bufferSize; ++i) {
        EXPECT_EQ(expectedData[i], buffer.at(i));
    }
    EXPECT_EQ(bufferSize, bytesRead);
}

TEST_F(ReaderTest, readDataLargerThanBufferSecondRead)
{
    // Arrange
    constexpr size_t bufferSize {5};
    std::string expectedData {"Test ASCII data"};
    ReaderXML xml;
    xml.givenDataAsXMLStream("<data>" + expectedData + "</data>");
    xml.Reader()->readElement("data");
    xml.Reader()->beginCharStream();
    std::array<char, bufferSize> buffer {};
    xml.Reader()->read(buffer.data(), bufferSize);  // Read the first five bytes

    // Act
    auto bytesRead = xml.Reader()->read(buffer.data(), bufferSize);  // Second five bytes

    // Assert
    for (size_t i = 0; i < bufferSize; ++i) {
        EXPECT_EQ(expectedData[i + bufferSize], buffer.at(i));
    }
    EXPECT_EQ(bufferSize, bytesRead);
}

TEST_F(ReaderTest, readDataNotStarted)
{
    // Arrange
    constexpr size_t bufferSize {20};
    std::string expectedData {"Test ASCII data"};
    ReaderXML xml;
    xml.givenDataAsXMLStream("<data>" + expectedData + "</data>");
    xml.Reader()->readElement("data");
    std::array<char, bufferSize> buffer {};

    // Act
    auto bytesRead = xml.Reader()->read(buffer.data(), bufferSize);

    // Assert
    EXPECT_EQ(-1, bytesRead);  // Because we didn't call beginCharStream
}

TEST_F(ReaderTest, readNextStartElement)
{
    auto xmlBody = R"(
<node1 attr='1'>Node1</node1>
<node2 attr='2'>Node2</node2>
)";

    ReaderXML xml;
    xml.givenDataAsXMLStream(xmlBody);

    // start of document
    EXPECT_TRUE(xml.Reader()->isStartOfDocument());
    xml.Reader()->readElement("document");
    EXPECT_STREQ(xml.Reader()->localName(), "document");

    // next element
    EXPECT_TRUE(xml.Reader()->readNextElement());
    EXPECT_STREQ(xml.Reader()->localName(), "node1");
    EXPECT_STREQ(xml.Reader()->getAttribute<const char*>("attr"), "1");
    xml.Reader()->readEndElement("node1");
    EXPECT_TRUE(xml.Reader()->isEndOfElement());

    // next element
    EXPECT_TRUE(xml.Reader()->readNextElement());
    EXPECT_STREQ(xml.Reader()->localName(), "node2");
    EXPECT_STREQ(xml.Reader()->getAttribute<const char*>("attr"), "2");
    xml.Reader()->readEndElement("node2");
    EXPECT_TRUE(xml.Reader()->isEndOfElement());
    xml.Reader()->readEndElement("document");
    EXPECT_TRUE(xml.Reader()->isEndOfDocument());
}

TEST_F(ReaderTest, readNextStartEndElement)
{
    // Arrange
    enum class TimesIGoToBed
    {
        Late,
        Later,
        VeryLate,
        FreeCADDevLate  // https://user-images.githubusercontent.com/12400097/235325792-606bffd6-6607-4542-a7d9-a04f12120666.png
    };

    auto xmlBody = R"(
<node1 attr='1'/>
<node2 attr='2'/>
<node3 attr='3'/>
<node4 attr='1'/>
<node5 attr='0'/>
<node5b attr='0xFF'/>
<node6 attr='asdaf'/>
<node7 attr="const char* is faster :'("/>
<node8 attr='8'/>
<node9 attr='9'/>
<node10 attr='10'/>
<node11 attr='11'/>
)";

    ReaderXML xml;
    xml.givenDataAsXMLStream(xmlBody);

    // start of document
    EXPECT_TRUE(xml.Reader()->isStartOfDocument());
    xml.Reader()->readElement("document");
    EXPECT_STREQ(xml.Reader()->localName(), "document");

    // next element
    EXPECT_TRUE(xml.Reader()->readNextElement());
    EXPECT_STREQ(xml.Reader()->localName(), "node1");
    EXPECT_STREQ(xml.Reader()->getAttribute<const char*>("attr"), "1");

    // next element
    EXPECT_TRUE(xml.Reader()->readNextElement());
    EXPECT_STREQ(xml.Reader()->localName(), "node2");
    EXPECT_STREQ(xml.Reader()->getAttribute<const char*>("attr"), "2");

    // next element
    EXPECT_TRUE(xml.Reader()->readNextElement());
    EXPECT_STREQ(xml.Reader()->localName(), "node3");
    EXPECT_EQ(xml.Reader()->getAttribute<TimesIGoToBed>("attr"), TimesIGoToBed::FreeCADDevLate);

    // next element
    EXPECT_TRUE(xml.Reader()->readNextElement());
    EXPECT_STREQ(xml.Reader()->localName(), "node4");
    EXPECT_EQ(xml.Reader()->getAttribute<bool>("attr"), true);

    // next element
    EXPECT_TRUE(xml.Reader()->readNextElement());
    EXPECT_STREQ(xml.Reader()->localName(), "node5");
    EXPECT_EQ(xml.Reader()->getAttribute<bool>("attr"), false);

    // next element
    EXPECT_TRUE(xml.Reader()->readNextElement());
    EXPECT_STREQ(xml.Reader()->localName(), "node5b");
    EXPECT_EQ(xml.Reader()->getAttribute<bool>("attr"), true);

    // next element
    EXPECT_TRUE(xml.Reader()->readNextElement());
    EXPECT_STREQ(xml.Reader()->localName(), "node6");
    EXPECT_EQ(xml.Reader()->getAttribute<bool>("attr"), true);

    // next element
    EXPECT_TRUE(xml.Reader()->readNextElement());
    EXPECT_STREQ(xml.Reader()->localName(), "node7");
    EXPECT_EQ(xml.Reader()->getAttribute<std::string>("attr"),
              std::string("const char* is faster :'("));

    // next element
    EXPECT_TRUE(xml.Reader()->readNextElement());
    EXPECT_STREQ(xml.Reader()->localName(), "node8");
    EXPECT_EQ(xml.Reader()->getAttribute<long>("attr"), 8);

    // next element
    EXPECT_TRUE(xml.Reader()->readNextElement());
    EXPECT_STREQ(xml.Reader()->localName(), "node9");
    EXPECT_EQ(xml.Reader()->getAttribute<unsigned long>("attr"), 9);

    // next element
    EXPECT_TRUE(xml.Reader()->readNextElement());
    EXPECT_STREQ(xml.Reader()->localName(), "node10");
    EXPECT_EQ(xml.Reader()->getAttribute<int>("attr"), 10);

    // next element
    EXPECT_TRUE(xml.Reader()->readNextElement());
    EXPECT_STREQ(xml.Reader()->localName(), "node11");
    EXPECT_EQ(xml.Reader()->getAttribute<QString>("attr"), QStringLiteral("11"));

    EXPECT_FALSE(xml.Reader()->readNextElement());
    EXPECT_TRUE(xml.Reader()->isEndOfDocument());
}

TEST_F(ReaderTest, charStreamBase64Encoded)
{
    // Arrange
    static constexpr size_t bufferSize {100};
    std::array<char, bufferSize> buffer {};
    ReaderXML xml;
    xml.givenDataAsXMLStream("<data>RnJlZUNBRCByb2NrcyEg8J+qqPCfqqjwn6qo\n</data>");
    xml.Reader()->readElement("data");
    xml.Reader()->beginCharStream(Base::CharStreamFormat::Base64Encoded);

    // Act
    xml.Reader()->charStream().getline(buffer.data(), bufferSize);
    xml.Reader()->endCharStream();

    // Assert
    // Conversion done using https://www.base64encode.org for testing purposes
    EXPECT_EQ(std::string("FreeCAD rocks! 🪨🪨🪨"), std::string(buffer.data()));
}

TEST_F(ReaderTest, validDefaults)
{
    // Arrange
    enum class TimesIGoToBed
    {
        Late,
        Later,
        VeryLate,
        FreeCADDevLate  // https://user-images.githubusercontent.com/12400097/235325792-606bffd6-6607-4542-a7d9-a04f12120666.png
    };

    auto xmlBody = R"(
<node1 attr='1'/>
<node2 attr='2'/>
)";

    ReaderXML xml;
    xml.givenDataAsXMLStream(xmlBody);

    // Act
    const char* value2 = xml.Reader()->getAttribute<const char*>("missing", "expected value");
    int value4 = xml.Reader()->getAttribute<long>("missing", -123);
    unsigned value6 = xml.Reader()->getAttribute<unsigned long>("missing", 123);
    double value8 = xml.Reader()->getAttribute<double>("missing", 1.234);
    bool value12 = xml.Reader()->getAttribute<bool>("missing", 0);
    bool value14 = xml.Reader()->getAttribute<bool>("missing", 1);
    bool value16 = xml.Reader()->getAttribute<bool>("missing", -10);
    bool value18 = xml.Reader()->getAttribute<bool>("missing", 10);
    TimesIGoToBed value20 =
        xml.Reader()->getAttribute<TimesIGoToBed>("missing", TimesIGoToBed::Late);

    // Assert
    EXPECT_THROW({ xml.Reader()->getAttribute<const char*>("missing"); }, Base::XMLBaseException);
    EXPECT_EQ(value2, "expected value");
    EXPECT_THROW({ xml.Reader()->getAttribute<long>("missing"); }, Base::XMLBaseException);
    EXPECT_EQ(value4, -123);
    EXPECT_THROW({ xml.Reader()->getAttribute<unsigned long>("missing"); }, Base::XMLBaseException);
    EXPECT_EQ(value6, 123);
    EXPECT_THROW({ xml.Reader()->getAttribute<double>("missing"); }, Base::XMLBaseException);
    EXPECT_NEAR(value8, 1.234, 0.001);
    EXPECT_THROW({ xml.Reader()->getAttribute<int>("missing"); }, Base::XMLBaseException);
    EXPECT_NEAR(value8, 1.234, 0.001);
    EXPECT_THROW({ xml.Reader()->getAttribute<bool>("missing"); }, Base::XMLBaseException);
    EXPECT_EQ(value12, false);
    EXPECT_EQ(value14, true);
    EXPECT_EQ(value16, true);
    EXPECT_EQ(value18, true);
    EXPECT_THROW({ xml.Reader()->getAttribute<TimesIGoToBed>("missing"); }, Base::XMLBaseException);
    EXPECT_EQ(value20, TimesIGoToBed::Late);
}
