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

#include <iterator>
#include <string>

#include <App/Application.h>
#include <src/App/InitApplication.h>

#include <Mod/Material/App/Exceptions.h>
#include <Mod/Material/App/MaterialManager.h>
#include <Mod/Material/App/ModelLibrary.h>
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
    _modelManager = &(Materials::ModelManager::getManager());
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
    auto libraries = _modelManager->getLibraries();
    ASSERT_GT(libraries->size(), 0);

    // We should have at least one model
    auto models = _modelManager->getModels();
    ASSERT_GT(models->size(), 0);
}

TEST_F(TestModel, TestModelLoad)
{
    ASSERT_NE(_modelManager, nullptr);

    auto density = _modelManager->getModel("454661e5-265b-4320-8e6f-fcf6223ac3af");
    EXPECT_EQ(density->getName(), "Density");
    EXPECT_EQ(density->getUUID(), "454661e5-265b-4320-8e6f-fcf6223ac3af");

    auto& prop = (*density)["Density"];
    EXPECT_EQ(prop.getName(), "Density");
}

TEST_F(TestModel, TestModelByPath)
{
    ASSERT_NE(_modelManager, nullptr);

    auto linearElastic = _modelManager->getModelByPath(
        "Mechanical/LinearElastic.yml",
        "System");
    EXPECT_NE(&linearElastic, nullptr);
    EXPECT_EQ(linearElastic->getName(), "Linear Elastic");
    EXPECT_EQ(linearElastic->getUUID(), "7b561d1d-fb9b-44f6-9da9-56a4f74d7536");

    // The same but with a leading '/'
    auto linearElastic2 = _modelManager->getModelByPath(
        "/Mechanical/LinearElastic.yml",
        "System");
    EXPECT_NE(&linearElastic2, nullptr);
    EXPECT_EQ(linearElastic2->getName(), "Linear Elastic");
    EXPECT_EQ(linearElastic2->getUUID(), "7b561d1d-fb9b-44f6-9da9-56a4f74d7536");

    // Same with the library name as a prefix
    auto linearElastic3 = _modelManager->getModelByPath(
        "/System/Mechanical/LinearElastic.yml",
        "System");
    EXPECT_NE(&linearElastic3, nullptr);
    EXPECT_EQ(linearElastic3->getName(), "Linear Elastic");
    EXPECT_EQ(linearElastic3->getUUID(), "7b561d1d-fb9b-44f6-9da9-56a4f74d7536");

    // Test with the file system path
    ASSERT_NO_THROW(linearElastic->getLibrary());
    ASSERT_NO_THROW(linearElastic->getLibrary()->getName());
    ASSERT_NO_THROW(linearElastic->getLibrary()->getDirectoryPath());
    EXPECT_EQ(linearElastic->getLibrary()->getName(), "System");
    const std::string path = linearElastic->getLibrary()->getDirectoryPath() + "/Mechanical/LinearElastic.yml";

    ASSERT_NO_THROW(_modelManager->getModelByPath(path));
    auto linearElastic4 = _modelManager->getModelByPath(path);
    EXPECT_NE(&linearElastic4, nullptr);
    EXPECT_EQ(linearElastic4->getName(), "Linear Elastic");
    EXPECT_EQ(linearElastic4->getUUID(), "7b561d1d-fb9b-44f6-9da9-56a4f74d7536");
}

TEST_F(TestModel, TestValidateProperties)
{
    // The local library and the matching remote one
    const Materials::Library library {"Library", QByteArray(), true};
    auto localLibrary = std::make_shared<Materials::ModelLibrary>(library);
    auto remoteLibrary = std::make_shared<Materials::ModelLibrary>(library);

    Materials::Model model;
    model.setType(Materials::Model::ModelType_Physical);
    model.setLibrary(localLibrary);
    model.setUUID("d0e6b5a4-3a1f-4d4e-8a5b-7c3f2b1a0987");
    Materials::Model remote;
    remote.setType(Materials::Model::ModelType_Physical);
    remote.setLibrary(remoteLibrary);
    remote.setUUID(model.getUUID());

    Materials::ModelProperty density {"Density",
                                     "Density",
                                     "Quantity",
                                     "kg/m^3",
                                     std::string(),
                                     std::string()};
    model.addProperty(density);

    // The same property on both sides validates
    Materials::ModelProperty remoteDensity {density};
    remote.addProperty(remoteDensity);
    EXPECT_NO_THROW(model.validate(remote));

    // A property the remote model does not have is reported, not dereferenced
    Materials::Model other;
    other.setType(Materials::Model::ModelType_Physical);
    other.setLibrary(remoteLibrary);
    other.setUUID(model.getUUID());
    Materials::ModelProperty mass {"Mass",
                                  "Mass",
                                  "Quantity",
                                  "kg",
                                  std::string(),
                                  std::string()};
    other.addProperty(mass);
    EXPECT_EQ(std::distance(model.begin(), model.end()),
              std::distance(other.begin(), other.end()));

    try {
        model.validate(other);
        FAIL() << "A missing remote property was accepted";
    }
    catch (const Materials::InvalidModel& e) {
        EXPECT_STREQ(e.what(), "Model properties don't match");
    }
}

// clang-format on
