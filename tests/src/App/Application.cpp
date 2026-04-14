// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>
#define FC_OS_MACOSX 1
#include <filesystem>

#include "App/Application.h"
#include "App/Document.h"
#include "App/Link.h"
#include "Base/FileInfo.h"
#include "App/ProgramOptionsUtilities.h"

#include <src/App/InitApplication.h>


using namespace App::Util;

using Spr = std::pair<std::string, std::string>;


class ApplicationTest: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }

    void SetUp() override
    {
        App::GetApplication().closeAllDocuments();
    }

    void TearDown() override
    {
        App::GetApplication().closeAllDocuments();
    }
};

TEST_F(ApplicationTest, fCustomSyntaxLookup)
{
    Spr res {customSyntax("-display")};
    Spr exp {"display", "null"};
    EXPECT_EQ(res, exp);
};
TEST_F(ApplicationTest, fCustomSyntaxMac)
{
    Spr res {customSyntax("-psn_stuff")};
    Spr exp {"psn", "stuff"};
    EXPECT_EQ(res, exp);
};
TEST_F(ApplicationTest, fCustomSyntaxWidgetCount)
{
    Spr res {customSyntax("-widgetcount")};
    Spr exp {"widgetcount", ""};
    EXPECT_EQ(res, exp);
}
TEST_F(ApplicationTest, fCustomSyntaxNotFound)
{
    Spr res {customSyntax("-displayx")};
    Spr exp {"", ""};
    EXPECT_EQ(res, exp);
};
TEST_F(ApplicationTest, fCustomSyntaxAmpersand)
{
    Spr res {customSyntax("@freddie")};
    Spr exp {"response-file", "freddie"};
    EXPECT_EQ(res, exp);
};
TEST_F(ApplicationTest, fCustomSyntaxEmptyIn)
{
    Spr res {customSyntax("")};
    Spr exp {"", ""};
    EXPECT_EQ(res, exp);
};

TEST_F(ApplicationTest, processFilesNormalizesRelativeDocumentPathsForLinks)
{
    namespace fs = std::filesystem;

    struct CurrentPathGuard
    {
        explicit CurrentPathGuard(fs::path path)
            : original(std::move(path))
        {}

        ~CurrentPathGuard()
        {
            fs::current_path(original);
        }

        fs::path original;
    };

    const fs::path originalPath = fs::current_path();
    CurrentPathGuard pathGuard(originalPath);

    const fs::path root = Base::FileInfo::getTempFileName("fc28683");
    fs::remove(root);
    fs::create_directories(root / "cwd");
    fs::create_directories(root / "owner");

    const fs::path sourceFile = root / "cwd" / "box.FCStd";
    const fs::path ownerFile = root / "owner" / "owner.FCStd";

    auto sourceName = App::GetApplication().getUniqueDocumentName("box_source");
    auto* sourceDoc = App::GetApplication().newDocument(sourceName.c_str(), "box");
    ASSERT_NE(sourceDoc, nullptr);
    ASSERT_NE(sourceDoc->addObject("App::DocumentObjectGroup", "Target"), nullptr);
    ASSERT_TRUE(sourceDoc->saveAs(sourceFile.string().c_str()));
    App::GetApplication().closeDocument(sourceDoc->getName());

    fs::current_path(root / "cwd");
    auto processed = App::Application::processFiles({std::string("box.FCStd")});
    ASSERT_EQ(processed.size(), 1);
    EXPECT_EQ(processed.front(), "box.FCStd");

    auto* reopenedDoc = App::GetApplication().getDocument("box");
    ASSERT_NE(reopenedDoc, nullptr);
    EXPECT_EQ(fs::path(reopenedDoc->getFileName()), sourceFile);

    auto* target = reopenedDoc->getObject("Target");
    ASSERT_NE(target, nullptr);

    auto ownerName = App::GetApplication().getUniqueDocumentName("owner_doc");
    auto* ownerDoc = App::GetApplication().newDocument(ownerName.c_str(), "owner");
    ASSERT_NE(ownerDoc, nullptr);
    ASSERT_TRUE(ownerDoc->saveAs(ownerFile.string().c_str()));

    auto* link = freecad_cast<App::Link*>(ownerDoc->addObject("App::Link", "Link"));
    ASSERT_NE(link, nullptr);
    EXPECT_NO_THROW(link->setLink(-1, target));
    EXPECT_EQ(link->getLinkedObject(), target);

    fs::current_path(originalPath);
    fs::remove_all(root);
}
