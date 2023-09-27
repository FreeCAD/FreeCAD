// SPDX-License-Identifier: LGPL-2.1-or-later
/***************************************************************************
 *   Copyright (c) 2023 David Carter <dcarter@david.carter.ca>             *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/

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
    _modelManager = new Materials::ModelManager();
    _materialManager = new Materials::MaterialManager();
  }

  // void TearDown() override {}
  Materials::ModelManager* _modelManager;
  Materials::MaterialManager* _materialManager;
};

TEST_F(MaterialTest, TestApplication)
{
    try {
        App::GetApplication();
    }
    catch (...) {
        ADD_FAILURE() << "Application failure\n";
    }

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
    EXPECT_LE(materialsComplete->size(), materials->size());

    auto materialsLinearElastic = _materialManager->materialsWithModel(
        QString::fromStdString("7b561d1d-fb9b-44f6-9da9-56a4f74d7536")); // LinearElastic

    // All LinearElastic models should be in IsotropicLinearElastic since it is inherited
    EXPECT_LE(materialsLinearElastic->size(), materials->size());
    for (auto itp = materialsLinearElastic->begin(); itp != materialsLinearElastic->end(); itp++) {
        auto mat = itp->first;
        EXPECT_NO_THROW(materials->at(mat));
    }
}

TEST_F(MaterialTest, testMaterialByPath)
{
    auto& steel = _materialManager->getMaterialByPath(
        QString::fromStdString("StandardMaterial/Metal/Steel/CalculiX-Steel.FCMat"),
        QString::fromStdString("System"));
    EXPECT_NE(&steel, nullptr);
    EXPECT_EQ(steel.getName(), QString::fromStdString("CalculiX-Steel"));
    EXPECT_EQ(steel.getUUID(), QString::fromStdString("92589471-a6cb-4bbc-b748-d425a17dea7d"));

    // The same but with a leading '/'
    auto& steel2 = _materialManager->getMaterialByPath(
        QString::fromStdString("/System/StandardMaterial/Metal/Steel/CalculiX-Steel.FCMat"),
        QString::fromStdString("System"));
    EXPECT_NE(&steel2, nullptr);
    EXPECT_EQ(steel2.getName(), QString::fromStdString("CalculiX-Steel"));
    EXPECT_EQ(steel2.getUUID(), QString::fromStdString("92589471-a6cb-4bbc-b748-d425a17dea7d"));

    // Same with the library name as a prefix
    auto& steel3 = _materialManager->getMaterialByPath(
        QString::fromStdString("StandardMaterial/Metal/Steel/CalculiX-Steel.FCMat"),
        QString::fromStdString("System"));
    EXPECT_NE(&steel3, nullptr);
    EXPECT_EQ(steel3.getName(), QString::fromStdString("CalculiX-Steel"));
    EXPECT_EQ(steel3.getUUID(), QString::fromStdString("92589471-a6cb-4bbc-b748-d425a17dea7d"));
}

// clang-format on
