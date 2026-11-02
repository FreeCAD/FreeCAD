// SPDX-License-Identifier: LGPL-2.1-or-later

#include <gtest/gtest.h>

#include "InitApplication.h"

#include <App/Application.h>

class ApplicationExportTypesTest: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        tests::initApplication();
    }
};

// Translatable export types are re-added whenever the language changes, which must not undo a
// module changed with changeExportModule()
TEST_F(ApplicationExportTypesTest, changedModuleSurvivesRetranslation)
{
    auto& app = App::GetApplication();
    app.addTranslatableExportType("Retranslation test", {"fcretranslationtest"}, "OldModule");
    app.changeExportModule("Retranslation test (*.fcretranslationtest)", "OldModule", "NewModule");
    EXPECT_EQ(app.getExportModules("fcretranslationtest"), std::vector<std::string> {"NewModule"});

    app.retranslateExportTypes();
    EXPECT_EQ(app.getExportModules("fcretranslationtest"), std::vector<std::string> {"NewModule"});
}

TEST_F(ApplicationExportTypesTest, changeRequiresMatchingModule)
{
    auto& app = App::GetApplication();
    app.addTranslatableExportType("Module match test", {"fcmodulematchtest"}, "OldModule");
    app.changeExportModule("Module match test (*.fcmodulematchtest)", "OtherModule", "NewModule");
    app.retranslateExportTypes();
    EXPECT_EQ(app.getExportModules("fcmodulematchtest"), std::vector<std::string> {"OldModule"});
}
