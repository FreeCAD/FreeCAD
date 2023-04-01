// SPDX-License-Identifier: LGPL-2.1-or-later

#include "gtest/gtest.h"

#include "Base/Exception.h"
#include "Base/Reader.h"
#include <array>
#include <filesystem>
#include <fmt/format.h>
#include <fstream>
#include <random>

namespace fs = std::filesystem;

class ReaderTest: public ::testing::Test
{
protected:
    void SetUp() override
    {
        xercesc_3_2::XMLPlatformUtils::Initialize();
        _tempDir = fs::temp_directory_path();
        std::string filename = uniqueName() + ".xml";
        _tempFile = _tempDir / filename;
    }

    void TearDown() override
    {
        std::filesystem::remove(_tempFile);
    }

    void givenDataAsXMLStream(const std::string& data)
    {
        auto stringData = R"(<?xml version="1.0" encoding="UTF-8"?><document>)" + data + "</document>";
        std::istringstream stream(stringData);
        std::ofstream fileStream(_tempFile);
        fileStream.write(stringData.data(), static_cast<std::streamsize>(stringData.length()));
        fileStream.close();
        std::ifstream inputStream(_tempFile);
        _reader = std::make_unique<Base::XMLReader>(_tempFile.string().c_str(), inputStream);
    }

    /// Generate a random, probably-unique 16-character alphanumeric filename.
    static std::string uniqueName()
    {
        constexpr size_t filenameLength {16};
        static std::default_random_engine _generator;
        auto random_char = []() -> char {
            constexpr int numChars {63};
            std::array<char, numChars> charset {
                "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"};
            std::uniform_int_distribution<int> distribution(0, numChars - 1);
            return charset.at(distribution(_generator));
        };
        std::string str(filenameLength, 0);
        std::generate_n(str.begin(), filenameLength, random_char);
        return str;
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
    auto& result = Reader()->beginCharStream(); // Not an error, even though there is no data

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
    EXPECT_THROW(Reader()->beginCharStream(), Base::XMLParseException);
}

TEST_F(ReaderTest, charStreamGood)
{
    // Arrange
    givenDataAsXMLStream("<data>Test ASCII data</data>");
    Reader()->readElement("data");
    Reader()->beginCharStream();

    // Act
    auto &result = Reader()->charStream();

    // Assert
    EXPECT_TRUE(result.good());
}

TEST_F(ReaderTest, charStreamBad)
{
    // Arrange
    givenDataAsXMLStream("<data>Test ASCII data</data>");
    Reader()->readElement("data");

    // Act & Assert
    EXPECT_THROW(Reader()->charStream(), Base::XMLParseException);
}

TEST_F(ReaderTest, endCharStreamGood)
{
    // Arrange
    givenDataAsXMLStream("<data>Test ASCII data</data>");
    Reader()->readElement("data");
    Reader()->beginCharStream();

    // Act & Assert
    Reader()->endCharStream(); // Does not throw
}

TEST_F(ReaderTest, endCharStreamBad)
{
    // Arrange
    givenDataAsXMLStream("<data>Test ASCII data</data>");
    Reader()->readElement("data");
    // Do not open the stream...

    // Act & Assert
    Reader()->endCharStream(); // Does not throw, even with no open stream
}

TEST_F(ReaderTest, readDataSmallerThanBuffer)
{
    // Arrange
    constexpr size_t bufferSize {20};
    std::string expectedData {"Test ASCII data"};
    givenDataAsXMLStream("<data>" + expectedData + "</data>");
    Reader()->readElement("data");
    Reader()->beginCharStream();
    std::array<char,bufferSize> buffer{};

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
    std::array<char,bufferSize> buffer{};

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
    std::array<char,bufferSize> buffer{};
    Reader()->read(buffer.data(), bufferSize); // Read the first five bytes

    // Act
    auto bytesRead = Reader()->read(buffer.data(), bufferSize); // Second five bytes

    // Assert
    for (size_t i = 0; i < bufferSize; ++i) {
        EXPECT_EQ(expectedData[i+bufferSize], buffer.at(i));
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
    std::array<char,bufferSize> buffer{};

    // Act
    auto bytesRead = Reader()->read(buffer.data(), bufferSize);

    // Assert
    EXPECT_EQ(-1, bytesRead); // Because we didn't call beginCharStream
}
