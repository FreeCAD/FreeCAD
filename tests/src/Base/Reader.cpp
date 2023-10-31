// SPDX-License-Identifier: LGPL-2.1-or-later

#include "gtest/gtest.h"

#ifdef _MSC_VER
#pragma warning(disable : 4996)
#endif

#include "Base/Exception.h"
#include "Base/Reader.h"
#include <array>
#include <boost/filesystem.hpp>
#include <fmt/format.h>
#include <fstream>

namespace fs = boost::filesystem;

class ReaderTest: public ::testing::Test
{
protected:
    void SetUp() override
    {
        xercesc_3_2::XMLPlatformUtils::Initialize();
        _tempDir = fs::temp_directory_path();
        std::string filename = "unit_test_Reader.xml";
        _tempFile = _tempDir / filename;
    }

    void TearDown() override
    {
        if (inputStream.is_open()) {
            inputStream.close();
        }
        if (fs::exists(_tempFile)) {
            fs::remove(_tempFile);
        }
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

    Base::XMLReader* Reader()
    {
        return _reader.get();
    }

private:
    std::unique_ptr<Base::XMLReader> _reader;
    fs::path _tempDir;
    fs::path _tempFile;
    std::ifstream inputStream;
};

TEST_F(ReaderTest, beginCharStreamNormal)
{
    // Arrange
    givenDataAsXMLStream("<data>Test ASCII data</data>");
    Reader()->readElement("data");

    // Act
    auto& result = Reader()->beginCharStream();

    // Assert
    EXPECT_TRUE(result.good());
}

TEST_F(ReaderTest, beginCharStreamOpenClose)
{
    // Arrange
    givenDataAsXMLStream("<data id='12345' />");
    Reader()->readElement("data");

    // Act
    auto& result = Reader()->beginCharStream();  // Not an error, even though there is no data

    // Assert
    EXPECT_TRUE(result.good());
}

TEST_F(ReaderTest, beginCharStreamAlreadyBegun)
{
    // Arrange
    givenDataAsXMLStream("<data>Test ASCII data</data>");
    Reader()->readElement("data");
    Reader()->beginCharStream();

    // Act & Assert
    EXPECT_THROW(Reader()->beginCharStream(), Base::XMLParseException);  // NOLINT
}

TEST_F(ReaderTest, charStreamGood)
{
    // Arrange
    givenDataAsXMLStream("<data>Test ASCII data</data>");
    Reader()->readElement("data");
    Reader()->beginCharStream();

    // Act
    auto& result = Reader()->charStream();

    // Assert
    EXPECT_TRUE(result.good());
}

TEST_F(ReaderTest, charStreamBad)
{
    // Arrange
    givenDataAsXMLStream("<data>Test ASCII data</data>");
    Reader()->readElement("data");

    // Act & Assert
    EXPECT_THROW(Reader()->charStream(), Base::XMLParseException);  // NOLINT
}

TEST_F(ReaderTest, endCharStreamGood)
{
    // Arrange
    givenDataAsXMLStream("<data>Test ASCII data</data>");
    Reader()->readElement("data");
    Reader()->beginCharStream();

    // Act & Assert
    Reader()->endCharStream();  // Does not throw
}

TEST_F(ReaderTest, endCharStreamBad)
{
    // Arrange
    givenDataAsXMLStream("<data>Test ASCII data</data>");
    Reader()->readElement("data");
    // Do not open the stream...

    // Act & Assert
    Reader()->endCharStream();  // Does not throw, even with no open stream
}

TEST_F(ReaderTest, readDataSmallerThanBuffer)
{
    // Arrange
    constexpr size_t bufferSize {20};
    std::string expectedData {"Test ASCII data"};
    givenDataAsXMLStream("<data>" + expectedData + "</data>");
    Reader()->readElement("data");
    Reader()->beginCharStream();
    std::array<char, bufferSize> buffer {};

    // Act
    auto bytesRead = Reader()->read(buffer.data(), bufferSize);

    // Assert
    EXPECT_STREQ(expectedData.c_str(), buffer.data());
    EXPECT_EQ(expectedData.length(), bytesRead);
}

TEST_F(ReaderTest, readDataLargerThanBuffer)
{
    // Arrange
    constexpr size_t bufferSize {5};
    std::string expectedData {"Test ASCII data"};
    givenDataAsXMLStream("<data>" + expectedData + "</data>");
    Reader()->readElement("data");
    Reader()->beginCharStream();
    std::array<char, bufferSize> buffer {};

    // Act
    auto bytesRead = Reader()->read(buffer.data(), bufferSize);

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
    givenDataAsXMLStream("<data>" + expectedData + "</data>");
    Reader()->readElement("data");
    Reader()->beginCharStream();
    std::array<char, bufferSize> buffer {};
    Reader()->read(buffer.data(), bufferSize);  // Read the first five bytes

    // Act
    auto bytesRead = Reader()->read(buffer.data(), bufferSize);  // Second five bytes

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
    givenDataAsXMLStream("<data>" + expectedData + "</data>");
    Reader()->readElement("data");
    std::array<char, bufferSize> buffer {};

    // Act
    auto bytesRead = Reader()->read(buffer.data(), bufferSize);

    // Assert
    EXPECT_EQ(-1, bytesRead);  // Because we didn't call beginCharStream
}

TEST_F(ReaderTest, readNextStartElement)
{
    auto xmlBody = R"(
<node1 attr='1'>Node1</node1>
<node2 attr='2'>Node2</node2>
)";

    givenDataAsXMLStream(xmlBody);

    // start of document
    EXPECT_TRUE(Reader()->isStartOfDocument());
    Reader()->readElement("document");
    EXPECT_STREQ(Reader()->localName(), "document");

    // next element
    EXPECT_TRUE(Reader()->readNextElement());
    EXPECT_STREQ(Reader()->localName(), "node1");
    EXPECT_STREQ(Reader()->getAttribute("attr"), "1");
    Reader()->readEndElement("node1");
    EXPECT_TRUE(Reader()->isEndOfElement());

    // next element
    EXPECT_TRUE(Reader()->readNextElement());
    EXPECT_STREQ(Reader()->localName(), "node2");
    EXPECT_STREQ(Reader()->getAttribute("attr"), "2");
    Reader()->readEndElement("node2");
    EXPECT_TRUE(Reader()->isEndOfElement());
    Reader()->readEndElement("document");
    EXPECT_TRUE(Reader()->isEndOfDocument());
}

TEST_F(ReaderTest, readNextStartEndElement)
{
    auto xmlBody = R"(
<node1 attr='1'/>
<node2 attr='2'/>
)";

    givenDataAsXMLStream(xmlBody);

    // start of document
    EXPECT_TRUE(Reader()->isStartOfDocument());
    Reader()->readElement("document");
    EXPECT_STREQ(Reader()->localName(), "document");

    // next element
    EXPECT_TRUE(Reader()->readNextElement());
    EXPECT_STREQ(Reader()->localName(), "node1");
    EXPECT_STREQ(Reader()->getAttribute("attr"), "1");

    // next element
    EXPECT_TRUE(Reader()->readNextElement());
    EXPECT_STREQ(Reader()->localName(), "node2");
    EXPECT_STREQ(Reader()->getAttribute("attr"), "2");
    EXPECT_FALSE(Reader()->readNextElement());
    EXPECT_TRUE(Reader()->isEndOfDocument());
}

TEST_F(ReaderTest, charStreamBase64Encoded)
{
    // Arrange
    static constexpr size_t bufferSize {100};
    std::array<char, bufferSize> buffer {};
    givenDataAsXMLStream("<data>RnJlZUNBRCByb2NrcyEg8J+qqPCfqqjwn6qo\n</data>");
    Reader()->readElement("data");
    Reader()->beginCharStream(Base::CharStreamFormat::Base64Encoded);

    // Act
    Reader()->charStream().getline(buffer.data(), bufferSize);
    Reader()->endCharStream();

    // Assert
    // Conversion done using https://www.base64encode.org for testing purposes
    EXPECT_EQ(std::string("FreeCAD rocks! 🪨🪨🪨"), std::string(buffer.data()));
}
