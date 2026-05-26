// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>
#define FC_OS_MACOSX 1
#include "App/ProgramOptionsUtilities.h"

#include <App/Application.h>
#include <App/Document.h>
#include <Base/Interpreter.h>
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

TEST_F(ApplicationTest, closeDocumentPythonAcceptsSanitizedDocumentIdentifier)
{
    struct Cleanup
    {
        ~Cleanup()
        {
            auto& app = App::GetApplication();
            if (auto* doc = app.getDocument("close_document_hyphen_test")) {
                app.closeDocument(doc);
            }
        }
    } cleanup;

    Base::Interpreter().runString("import FreeCAD as App\n"
                                  "App.newDocument('close-document-hyphen-test')\n");

    auto* doc = App::GetApplication().getDocument("close_document_hyphen_test");
    ASSERT_NE(doc, nullptr);
    EXPECT_STREQ(doc->Label.getValue(), "close-document-hyphen-test");

    Base::Interpreter().runString("App.closeDocument('close-document-hyphen-test')\n");

    EXPECT_EQ(App::GetApplication().getDocument("close_document_hyphen_test"), nullptr);
}

TEST_F(ApplicationTest, closeDocumentPythonAcceptsSanitizedDocumentIdentifierWithCustomLabel)
{
    struct Cleanup
    {
        ~Cleanup()
        {
            auto& app = App::GetApplication();
            if (auto* doc = app.getDocument("close_document_custom_label_test")) {
                app.closeDocument(doc);
            }
        }
    } cleanup;

    Base::Interpreter().runString("import FreeCAD as App\n"
                                  "App.newDocument('close-document-custom-label-test', "
                                  "'Custom document label')\n");

    auto* doc = App::GetApplication().getDocument("close_document_custom_label_test");
    ASSERT_NE(doc, nullptr);
    EXPECT_STREQ(doc->Label.getValue(), "Custom document label");

    Base::Interpreter().runString("App.closeDocument('close-document-custom-label-test')\n");

    EXPECT_EQ(App::GetApplication().getDocument("close_document_custom_label_test"), nullptr);
}

TEST_F(ApplicationTest, closeDocumentPythonRejectsLabelIdentifierConflict)
{
    const std::string firstName {"close_document_identifier_conflict"};
    const std::string secondName {"close_document_identifier_conflict001"};

    struct Cleanup
    {
        const std::string& firstName;
        const std::string& secondName;

        ~Cleanup()
        {
            auto& app = App::GetApplication();
            if (auto* doc = app.getDocument(firstName.c_str())) {
                app.closeDocument(doc);
            }
            if (auto* doc = app.getDocument(secondName.c_str())) {
                app.closeDocument(doc);
            }
        }
    } cleanup {firstName, secondName};

    auto& app = App::GetApplication();
    auto* first = app.newDocument(firstName.c_str(), "existing-label");
    auto* second = app.newDocument("close-document-identifier-conflict");

    ASSERT_NE(first, nullptr);
    ASSERT_NE(second, nullptr);
    ASSERT_NE(first, second);
    EXPECT_STREQ(first->getName(), firstName.c_str());
    EXPECT_STREQ(second->getName(), secondName.c_str());
    EXPECT_STREQ(second->Label.getValue(), "close-document-identifier-conflict");

    EXPECT_THROW(Base::Interpreter().runString("import FreeCAD as App\n"
                                               "App.closeDocument('close-document-identifier-conflict')\n"),
                 Base::Exception);
    EXPECT_EQ(app.getDocument(firstName.c_str()), first);
    EXPECT_EQ(app.getDocument(secondName.c_str()), second);
}
