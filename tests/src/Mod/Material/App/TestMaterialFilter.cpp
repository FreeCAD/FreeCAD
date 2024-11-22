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

#include <QMetaType>
#include <QString>

#include <App/Application.h>
#include <Base/Quantity.h>
#include <Gui/MetaTypes.h>
#include <src/App/InitApplication.h>

#include <Mod/Material/App/MaterialLibrary.h>
#include <Mod/Material/App/MaterialManager.h>
#include <Mod/Material/App/MaterialValue.h>
#include <Mod/Material/App/Model.h>
#include <Mod/Material/App/ModelManager.h>
#include <Mod/Material/App/ModelUuids.h>

// clang-format off

class TestMaterialFilter : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        if (App::Application::GetARGC() == 0) {
            tests::initApplication();
        }
    }

    void SetUp() override {
        _modelManager = new Materials::ModelManager();
        _materialManager = new Materials::MaterialManager();

        // Use our test files as a custom directory
        ParameterGrp::handle hGrp =
            App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Material/Resources");

        _customDir = hGrp->GetASCII("CustomMaterialsDir", "");
        _useBuiltInDir = hGrp->GetBool("UseBuiltInMaterials", true);
        _useWorkbenchDir = hGrp->GetBool("UseMaterialsFromWorkbenches", true);
        _useUserDir = hGrp->GetBool("UseMaterialsFromConfigDir", true);
        _useCustomDir = hGrp->GetBool("UseMaterialsFromCustomDir", false);

        std::string testPath = App::Application::getHomePath() + "/tests/Materials/";
        hGrp->SetASCII("CustomMaterialsDir", testPath);
        hGrp->SetBool("UseBuiltInMaterials", false);
        hGrp->SetBool("UseMaterialsFromWorkbenches", false);
        hGrp->SetBool("UseMaterialsFromConfigDir", false);
        hGrp->SetBool("UseMaterialsFromCustomDir", true);

        _materialManager->refresh();

        _library = _materialManager->getLibrary(QLatin1String("Custom"));
    }

    void TearDown() override {
        ParameterGrp::handle hGrp =
            App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Material/Resources");

        // Restore preferences
        hGrp->SetASCII("CustomMaterialsDir", _customDir);
        hGrp->SetBool("UseBuiltInMaterials", _useBuiltInDir);
        hGrp->SetBool("UseMaterialsFromWorkbenches", _useWorkbenchDir);
        hGrp->SetBool("UseMaterialsFromConfigDir", _useUserDir);
        hGrp->SetBool("UseMaterialsFromCustomDir", _useCustomDir);

        _materialManager->refresh();
    }

    Materials::ModelManager* _modelManager;
    Materials::MaterialManager* _materialManager;
    std::shared_ptr<Materials::MaterialLibrary> _library;
    QString _testMaterialUUID;

    std::string _customDir;
    bool _useBuiltInDir;
    bool _useWorkbenchDir;
    bool _useUserDir;
    bool _useCustomDir;

    const char* UUIDAcrylicLegacy = ""; // This can't be known until it is loaded
    const char* UUIDAluminumAppearance = "3c6d0407-66b3-48ea-a2e8-ee843edf0311";
    const char* UUIDAluminumMixed =  "5f546608-fcbb-40db-98d7-d8e104eb33ce";
    const char* UUIDAluminumPhysical = "a8e60089-550d-4370-8e7e-1734db12a3a9";
    const char* UUIDBrassAppearance = "fff3d5c8-98c3-4ee2-8fe5-7e17403c48fcc";
};

TEST_F(TestMaterialFilter, TestFilters)
{
    ASSERT_NE(_modelManager, nullptr);

    // First check that our materials are loading
    auto material = _materialManager->getMaterial(QString::fromLatin1(UUIDAluminumAppearance));
    ASSERT_TRUE(material);
    ASSERT_EQ(material->getName(), QString::fromLatin1("TestAluminumAppearance"));
    ASSERT_EQ(material->getUUID(), QString::fromLatin1(UUIDAluminumAppearance));

    material = _materialManager->getMaterial(QString::fromLatin1(UUIDAluminumMixed));
    ASSERT_TRUE(material);
    ASSERT_EQ(material->getName(), QString::fromLatin1("TestAluminumMixed"));
    ASSERT_EQ(material->getUUID(), QString::fromLatin1(UUIDAluminumMixed));

    material = _materialManager->getMaterial(QString::fromLatin1(UUIDAluminumPhysical));
    ASSERT_TRUE(material);
    ASSERT_EQ(material->getName(), QString::fromLatin1("TestAluminumPhysical"));
    ASSERT_EQ(material->getUUID(), QString::fromLatin1(UUIDAluminumPhysical));

    material = _materialManager->getMaterial(QString::fromLatin1(UUIDBrassAppearance));
    ASSERT_TRUE(material);
    ASSERT_EQ(material->getName(), QString::fromLatin1("TestBrassAppearance"));
    ASSERT_EQ(material->getUUID(), QString::fromLatin1(UUIDBrassAppearance));

    material = _materialManager->getMaterialByPath(QString::fromLatin1("TestAcrylicLegacy.FCMat"),
        QString::fromLatin1("Custom"));
    ASSERT_TRUE(material);
    ASSERT_EQ(material->getName(), QString::fromLatin1("TestAcrylicLegacy"));
    ASSERT_EQ(material->getUUID().size(), 36); // We don't know the UUID

    // Create an empty filter
    auto filter = std::make_shared<Materials::MaterialFilter>();
    Materials::MaterialFilterOptions options;
    ASSERT_TRUE(_library);

    auto tree = _materialManager->getMaterialTree(_library, filter, options);
    ASSERT_EQ(tree->size(), 4);

    options.setIncludeLegacy(true);
    tree = _materialManager->getMaterialTree(_library, filter, options);
    ASSERT_EQ(tree->size(), 5);

    // Create a basic rendering filter
    filter->setName(QLatin1String("Basic Appearance"));
    filter->addRequiredComplete(Materials::ModelUUIDs::ModelUUID_Rendering_Basic);
    options.setIncludeLegacy(false);

    tree = _materialManager->getMaterialTree(_library, filter, options);
    ASSERT_EQ(tree->size(), 3);

    options.setIncludeLegacy(true);
    tree = _materialManager->getMaterialTree(_library, filter, options);
    ASSERT_EQ(tree->size(), 3);

    // Create an advanced rendering filter
    filter->clear();
    filter->setName(QLatin1String("Advanced Appearance"));
    filter->addRequiredComplete(Materials::ModelUUIDs::ModelUUID_Rendering_Advanced);
    options.setIncludeLegacy(false);

    tree = _materialManager->getMaterialTree(_library, filter, options);
    ASSERT_EQ(tree->size(), 0);

    options.setIncludeLegacy(true);
    tree = _materialManager->getMaterialTree(_library, filter, options);
    ASSERT_EQ(tree->size(), 0);

    // Create a Density filter
    filter->clear();
    filter->setName(QLatin1String("Density"));
    filter->addRequiredComplete(Materials::ModelUUIDs::ModelUUID_Mechanical_Density);
    options.setIncludeLegacy(false);

    tree = _materialManager->getMaterialTree(_library, filter, options);
    ASSERT_EQ(tree->size(), 2);

    options.setIncludeLegacy(true);
    tree = _materialManager->getMaterialTree(_library, filter, options);
    ASSERT_EQ(tree->size(), 3);

    // Create a Hardness filter
    filter->clear();
    filter->setName(QLatin1String("Hardness"));
    filter->addRequiredComplete(Materials::ModelUUIDs::ModelUUID_Mechanical_Hardness);
    options.setIncludeLegacy(false);

    tree = _materialManager->getMaterialTree(_library, filter, options);
    ASSERT_EQ(tree->size(), 0);

    options.setIncludeLegacy(true);
    tree = _materialManager->getMaterialTree(_library, filter, options);
    ASSERT_EQ(tree->size(), 0);

    // Create a Density and Basic Rendering filter
    filter->clear();
    filter->setName(QLatin1String("Density and Basic Rendering"));
    filter->addRequiredComplete(Materials::ModelUUIDs::ModelUUID_Rendering_Basic);
    filter->addRequiredComplete(Materials::ModelUUIDs::ModelUUID_Mechanical_Density);
    options.setIncludeLegacy(false);

    tree = _materialManager->getMaterialTree(_library, filter, options);
    ASSERT_EQ(tree->size(), 1);

    options.setIncludeLegacy(true);
    tree = _materialManager->getMaterialTree(_library, filter, options);
    ASSERT_EQ(tree->size(), 1);

    // Create a Linear Elastic filter
    filter->clear();
    filter->setName(QLatin1String("Linear Elastic"));
    filter->addRequiredComplete(Materials::ModelUUIDs::ModelUUID_Mechanical_LinearElastic);
    options.setIncludeLegacy(false);

    tree = _materialManager->getMaterialTree(_library, filter, options);
    ASSERT_EQ(tree->size(), 0);

    options.setIncludeLegacy(true);
    tree = _materialManager->getMaterialTree(_library, filter, options);
    ASSERT_EQ(tree->size(), 0);

    filter->clear();
    filter->setName(QLatin1String("Linear Elastic"));
    filter->addRequired(Materials::ModelUUIDs::ModelUUID_Mechanical_LinearElastic);
    options.setIncludeLegacy(false);

    tree = _materialManager->getMaterialTree(_library, filter, options);
    ASSERT_EQ(tree->size(), 2);

    options.setIncludeLegacy(true);
    tree = _materialManager->getMaterialTree(_library, filter, options);
    ASSERT_EQ(tree->size(), 2);
}
