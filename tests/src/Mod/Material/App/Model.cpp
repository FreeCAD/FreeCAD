// SPDX-License-Identifier: LGPL-2.1-or-later

#include "gtest/gtest.h"

#include <Mod/Material/App/PreCompiled.h>
#ifndef _PreComp_
#endif

#include <QString>

#include <App/Application.h>

#include <Mod/Material/App/MaterialManager.h>
#include <Mod/Material/App/Model.h>
#include <Mod/Material/App/ModelManager.h>

// clang-format off

class MaterialTest : public ::testing::Test {
 protected:
  static void SetUpTestSuite() {
    if (App::Application::GetARGC() == 0) {
        constexpr int argc = 1;
        std::array<char*, argc> argv {"FreeCAD"};
        App::Application::Config()["ExeName"] = "FreeCAD";
        App::Application::init(argc, argv.data());
    }
  }

  void SetUp() override {
    _modelManager = Materials::ModelManager::getManager();
    _materialManager = new Materials::MaterialManager();
  }

  // void TearDown() override {}
  Materials::ModelManager* _modelManager;
  Materials::MaterialManager* _materialManager;
};

TEST_F(MaterialTest, TestApplication)
{
    App::Application& application = App::GetApplication();
    if (&application == nullptr)
        ADD_FAILURE() << "Application failure\n";

    SUCCEED();
}

TEST_F(MaterialTest, TestResources)
{
    try {
        auto param = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Mod/Material/Resources");
        EXPECT_NE(param, nullptr);
    }
    catch (const std::exception &e)
    {
        FAIL() << "Exception: " << e.what() << "\n";
    }
}

TEST_F(MaterialTest, TestModelLoad)
{
    EXPECT_NE(_modelManager, nullptr);

    auto density = _modelManager->getModel(QString::fromStdString("454661e5-265b-4320-8e6f-fcf6223ac3af"));
    EXPECT_EQ(density.getName(), QString::fromStdString("Density"));
    EXPECT_EQ(density.getUUID(), QString::fromStdString("454661e5-265b-4320-8e6f-fcf6223ac3af"));

    auto& prop = density[QString::fromStdString("Density")];
    EXPECT_EQ(prop.getName(), QString::fromStdString("Density"));
}

TEST_F(MaterialTest, TestMaterialsWithModel)
{
    auto materials = _materialManager->materialsWithModel(
        QString::fromStdString("f6f9e48c-b116-4e82-ad7f-3659a9219c50")); // IsotropicLinearElastic
    EXPECT_GT(materials->size(), 0);
    
    auto materialsComplete = _materialManager->materialsWithModelComplete(
        QString::fromStdString("f6f9e48c-b116-4e82-ad7f-3659a9219c50"));  // IsotropicLinearElastic
    EXPECT_GE(materialsComplete->size(), materials->size());
}
    // def testMaterialsWithModel(self):
    //     materials = self.MaterialManager.materialsWithModel('f6f9e48c-b116-4e82-ad7f-3659a9219c50') # IsotropicLinearElastic
    //     materialsComplete = self.MaterialManager.materialsWithModelComplete('f6f9e48c-b116-4e82-ad7f-3659a9219c50') # IsotropicLinearElastic

    //     self.assertTrue(len(materialsComplete) <= len(materials)) # Not all will be complete

    //     materialsLinearElastic = self.MaterialManager.materialsWithModel('7b561d1d-fb9b-44f6-9da9-56a4f74d7536') # LinearElastic

    //     # All LinearElastic models should be in IsotropicLinearElastic since it is inherited
    //     self.assertTrue(len(materialsLinearElastic) <= len(materials))
    //     for mat in materialsLinearElastic:
    //         self.assertIn(mat, materials)

// clang-format on
