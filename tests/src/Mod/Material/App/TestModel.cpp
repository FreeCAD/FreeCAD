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

#include <gtest/gtest.h>

#include <Mod/Material/App/PreCompiled.h>
#ifndef _PreComp_
#endif

#include <QString>

#include <App/Application.h>
#include <src/App/InitApplication.h>

#include <Mod/Material/App/MaterialManager.h>
#include <Mod/Material/App/Model.h>
#include <Mod/Material/App/ModelManager.h>

// clang-format off

class TestModel : public ::testing::Test {
 protected:
  static void SetUpTestSuite() {
    if (App::Application::GetARGC() == 0) {
        tests::initApplication();
    }
  }

  void SetUp() override {
    _modelManager = new Materials::ModelManager();
  }

  // void TearDown() override {}
  Materials::ModelManager* _modelManager;
};

TEST_F(TestModel, TestApplication)
{
    ASSERT_NO_THROW(App::GetApplication());
}

TEST_F(TestModel, TestResources)
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

TEST_F(TestModel, TestInstallation)
{
    ASSERT_NE(_modelManager, nullptr);

    // We should have loaded at least the system library
    auto libraries = _modelManager->getModelLibraries();
    ASSERT_GT(libraries->size(), 0);

    // We should have at least one model
    auto models = _modelManager->getModels();
    ASSERT_GT(models->size(), 0);
}

TEST_F(TestModel, TestModelLoad)
{
    ASSERT_NE(_modelManager, nullptr);

    auto density = _modelManager->getModel(QString::fromStdString("454661e5-265b-4320-8e6f-fcf6223ac3af"));
    EXPECT_EQ(density->getName(), QString::fromStdString("Density"));
    EXPECT_EQ(density->getUUID(), QString::fromStdString("454661e5-265b-4320-8e6f-fcf6223ac3af"));

    auto& prop = (*density)[QString::fromStdString("Density")];
    EXPECT_EQ(prop.getName(), QString::fromStdString("Density"));
}

TEST_F(TestModel, TestModelByPath)
{
    ASSERT_NE(_modelManager, nullptr);

    auto linearElastic = _modelManager->getModelByPath(
        QString::fromStdString("Mechanical/LinearElastic.yml"),
        QString::fromStdString("System"));
    EXPECT_NE(&linearElastic, nullptr);
    EXPECT_EQ(linearElastic->getName(), QString::fromStdString("Linear Elastic"));
    EXPECT_EQ(linearElastic->getUUID(), QString::fromStdString("7b561d1d-fb9b-44f6-9da9-56a4f74d7536"));

    // The same but with a leading '/'
    auto linearElastic2 = _modelManager->getModelByPath(
        QString::fromStdString("/Mechanical/LinearElastic.yml"),
        QString::fromStdString("System"));
    EXPECT_NE(&linearElastic2, nullptr);
    EXPECT_EQ(linearElastic2->getName(), QString::fromStdString("Linear Elastic"));
    EXPECT_EQ(linearElastic2->getUUID(), QString::fromStdString("7b561d1d-fb9b-44f6-9da9-56a4f74d7536"));

    // Same with the library name as a prefix
    auto linearElastic3 = _modelManager->getModelByPath(
        QString::fromStdString("/System/Mechanical/LinearElastic.yml"),
        QString::fromStdString("System"));
    EXPECT_NE(&linearElastic3, nullptr);
    EXPECT_EQ(linearElastic3->getName(), QString::fromStdString("Linear Elastic"));
    EXPECT_EQ(linearElastic3->getUUID(), QString::fromStdString("7b561d1d-fb9b-44f6-9da9-56a4f74d7536"));

    // Test with the file system path
    ASSERT_NO_THROW(linearElastic->getLibrary());
    ASSERT_NO_THROW(linearElastic->getLibrary()->getName());
    ASSERT_NO_THROW(linearElastic->getLibrary()->getDirectoryPath());
    EXPECT_EQ(linearElastic->getLibrary()->getName(), QString::fromStdString("System"));
    QString path = linearElastic->getLibrary()->getDirectoryPath() + QString::fromStdString("/Mechanical/LinearElastic.yml");

    ASSERT_NO_THROW(_modelManager->getModelByPath(path));
    auto linearElastic4 = _modelManager->getModelByPath(path);
    EXPECT_NE(&linearElastic4, nullptr);
    EXPECT_EQ(linearElastic4->getName(), QString::fromStdString("Linear Elastic"));
    EXPECT_EQ(linearElastic4->getUUID(), QString::fromStdString("7b561d1d-fb9b-44f6-9da9-56a4f74d7536"));
}

// clang-format on
