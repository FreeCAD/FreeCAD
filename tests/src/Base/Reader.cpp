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
        std::ifstream inputStream(_tempFile.string());
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
