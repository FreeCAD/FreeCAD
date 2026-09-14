#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>

#include "Base/XMLParser.h"
#include "Base/Exception.h"
#include "Base/FileInfo.h"
#include "Base/ParameterSchema.h"
namespace fs = std::filesystem;

namespace
{
const std::string validParameterXML = R"(<?xml version="1.0" encoding="UTF-8" standalone="no" ?>)"
                                      "<FCParameters>"
                                      R"(<FCParamGroup Name="LogLevels">)"
                                      R"(<FCInt Name="Default" Value="2"/>)"
                                      R"(<FCText Name="AutoloadModule">PartDesignWorkbench</FCText>)"
                                      "</FCParamGroup>"
                                      "</FCParameters>";

fs::path writeXMLFile(const std::string& content)
{
    const fs::path tmpfile = Base::FileInfo::getTempFileName();
    std::ofstream fout(tmpfile);
    fout << content;
    fout.close();
    return tmpfile;
}
}  // namespace

class XMLParserTest: public ::testing::Test
{
};

TEST_F(XMLParserTest, TestLoadValidDocument)
{
    fs::path tmpfile = writeXMLFile(validParameterXML);
    auto parsedXML = Base::ParseXMLFile(tmpfile);
    EXPECT_EQ(parsedXML->tag, "FCParameters");
    EXPECT_EQ(parsedXML->attrs.size(), 0);
    EXPECT_EQ(parsedXML->content.size(), 0);
    EXPECT_EQ(parsedXML->children.size(), 1);

    auto&& paramGroup = parsedXML->children[0];
    EXPECT_EQ(paramGroup->tag, "FCParamGroup");
    EXPECT_EQ(paramGroup->attrs.size(), 1);
    EXPECT_EQ(paramGroup->attrs["Name"], "LogLevels");
    EXPECT_EQ(paramGroup->content.size(), 0);
    EXPECT_EQ(paramGroup->children.size(), 2);

    auto&& intParam = paramGroup->children[0];
    EXPECT_EQ(intParam->tag, "FCInt");
    EXPECT_EQ(intParam->attrs.size(), 2);
    EXPECT_EQ(intParam->attrs["Name"], "Default");
    EXPECT_EQ(intParam->attrs["Value"], "2");
    EXPECT_EQ(intParam->content.size(), 0);
    EXPECT_EQ(intParam->children.size(), 0);

    auto&& strParam = paramGroup->children[1];
    EXPECT_EQ(strParam->tag, "FCText");
    EXPECT_EQ(strParam->attrs.size(), 1);
    EXPECT_EQ(strParam->attrs["Name"], "AutoloadModule");
    EXPECT_EQ(strParam->content, "PartDesignWorkbench");
    EXPECT_EQ(strParam->children.size(), 0);
}

TEST_F(XMLParserTest, TestLoadInvalidDocument)
{
    const std::string invalidParamFile = validParameterXML + "<Look mom, I'm not XML compliant!";
    fs::path tmpfile = writeXMLFile(invalidParamFile);
    EXPECT_THROW({ auto parsedXML = Base::ParseXMLFile(tmpfile); }, Base::XMLBaseException);
}

TEST_F(XMLParserTest, TestLoadEmptyDocument)
{
    const std::string emptyFile;
    fs::path tmpfile = writeXMLFile(emptyFile);
    EXPECT_THROW({ auto parsedXML = Base::ParseXMLFile(tmpfile); }, Base::XMLBaseException);
}

TEST_F(XMLParserTest, TestLoadInvalidPath)
{
    EXPECT_THROW(
        { auto parsedXML = Base::ParseXMLFile(Base::FileInfo::getTempFileName()); },
        Base::XMLBaseException
    );
}

TEST_F(XMLParserTest, TestCheckValidFile)
{
    fs::path tmpfile = writeXMLFile(validParameterXML);
    auto parsedXML = Base::ParseXMLFile(tmpfile);

    auto res = Base::CheckXMLDocument(*parsedXML, ParameterSchema);
    EXPECT_FALSE(res.has_value());
}

TEST_F(XMLParserTest, TestCheckInvalidFile)
{
    fs::path tmpfile = writeXMLFile(validParameterXML);
    auto parsedXML = Base::ParseXMLFile(tmpfile);
    parsedXML->tag = "NotFCParameters";

    auto res = Base::CheckXMLDocument(*parsedXML, ParameterSchema);
    EXPECT_TRUE(res.has_value());
    EXPECT_TRUE(res.value().size() == 1);
    EXPECT_TRUE(res.value()[0].find("Unexpected XML structure detected") != std::string::npos);
}

TEST_F(XMLParserTest, TestWriteValidDocument)
{
    fs::path tmpfile = writeXMLFile(validParameterXML);
    auto parsedXML = Base::ParseXMLFile(tmpfile);

    const fs::path saveTo = Base::FileInfo::getTempFileName();
    Base::SaveXMLFile(saveTo, *parsedXML);

    std::ifstream fin(saveTo);
    std::string line;
    std::getline(fin, line);
    EXPECT_EQ(line, R"(<?xml version="1.0" encoding="UTF-8" standalone="no" ?>)");
    std::getline(fin, line);
    EXPECT_EQ(line, R"(<FCParameters>)");
    std::getline(fin, line);
    EXPECT_EQ(line, "");
    std::getline(fin, line);
    line.erase(0, line.find_first_not_of(' '));
    EXPECT_EQ(line, R"(<FCParamGroup Name="LogLevels">)");
    std::getline(fin, line);
    line.erase(0, line.find_first_not_of(' '));
    EXPECT_EQ(line, R"(<FCInt Name="Default" Value="2"/>)");
    std::getline(fin, line);
    line.erase(0, line.find_first_not_of(' '));
    EXPECT_EQ(line, R"(<FCText Name="AutoloadModule">PartDesignWorkbench</FCText>)");
    std::getline(fin, line);
    line.erase(0, line.find_first_not_of(' '));
    EXPECT_EQ(line, R"(</FCParamGroup>)");
    std::getline(fin, line);
    EXPECT_EQ(line, "");
    std::getline(fin, line);
    EXPECT_EQ(line, "</FCParameters>");
}
