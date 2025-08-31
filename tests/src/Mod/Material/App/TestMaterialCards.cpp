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

class TestMaterialCards : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        if (App::Application::GetARGC() == 0) {
            tests::initApplication();
        }
    }

    void SetUp() override {
        // Create a temporary library
        QString libPath = QDir::tempPath() + QStringLiteral("/TestMaterialCards");
        QDir libDir(libPath);
        libDir.removeRecursively(); // Clear old run data
        libDir.mkdir(libPath);
        _library = std::make_shared<Materials::MaterialLibraryLocal>(QStringLiteral("Testing"),
                        libPath,
                        QStringLiteral(":/icons/preferences-general.svg"),
                        false);
        _modelManager = &(Materials::ModelManager::getManager());
        _materialManager = &(Materials::MaterialManager::getManager());

        _testMaterialUUID = QStringLiteral("c6c64159-19c1-40b5-859c-10561f20f979");
    }

    // void TearDown() override {}
    Materials::ModelManager* _modelManager;
    Materials::MaterialManager* _materialManager;
    std::shared_ptr<Materials::MaterialLibraryLocal> _library;
    QString _testMaterialUUID;
};

TEST_F(TestMaterialCards, TestCopy)
{
    ASSERT_NE(_modelManager, nullptr);
    ASSERT_TRUE(_library);
    // FAIL() << "Test library " << _library->getDirectoryPath().toStdString() << "\n";

    auto testMaterial = _materialManager->getMaterial(_testMaterialUUID);
    auto newMaterial = std::make_shared<Materials::Material>(*testMaterial);

    EXPECT_EQ(testMaterial->getUUID(), _testMaterialUUID);
    EXPECT_EQ(newMaterial->getUUID(), _testMaterialUUID);

    // Save the material
    _materialManager->saveMaterial(_library,
                      newMaterial,
                      QStringLiteral("/Test Material2.FCMat"),
                      false, // overwrite
                      true,  // saveAsCopy
                      false); // saveInherited
    EXPECT_EQ(newMaterial->getUUID(), _testMaterialUUID);
    EXPECT_EQ(newMaterial->getName(), QStringLiteral("Test Material2"));

    // Save it when it already exists throwing an error
    EXPECT_THROW(_materialManager->saveMaterial(_library,
                      newMaterial,
                      QStringLiteral("/Test Material2.FCMat"),
                      false, // overwrite
                      true,  // saveAsCopy
                      false) // saveInherited
                      , Materials::MaterialExists);
    EXPECT_EQ(newMaterial->getUUID(), _testMaterialUUID);
    EXPECT_EQ(newMaterial->getName(), QStringLiteral("Test Material2"));

    // Overwrite the existing file
    _materialManager->saveMaterial(_library,
                      newMaterial,
                      QStringLiteral("/Test Material2.FCMat"),
                      true,  // overwrite
                      true,  // saveAsCopy
                      false);// saveInherited
    EXPECT_EQ(newMaterial->getUUID(), _testMaterialUUID);
    EXPECT_EQ(newMaterial->getName(), QStringLiteral("Test Material2"));

    // Save to a new file, inheritance mode
    _materialManager->saveMaterial(_library,
                      newMaterial,
                      QStringLiteral("/Test Material3.FCMat"),
                      false,  // overwrite
                      true,  // saveAsCopy
                      true);// saveInherited
    EXPECT_EQ(newMaterial->getUUID(), _testMaterialUUID);
    EXPECT_EQ(newMaterial->getName(), QStringLiteral("Test Material3"));

    // Save to a new file, inheritance mode. no copy
    _materialManager->saveMaterial(_library,
                      newMaterial,
                      QStringLiteral("/Test Material4.FCMat"),
                      false,  // overwrite
                      false,  // saveAsCopy
                      true);// saveInherited
    EXPECT_NE(newMaterial->getUUID(), _testMaterialUUID);
    EXPECT_EQ(newMaterial->getName(), QStringLiteral("Test Material4"));
    QString uuid1 = newMaterial->getUUID();

    // Save to a new file, inheritance mode, testing overwrite, new copy
    _materialManager->saveMaterial(_library,
                      newMaterial,
                      QStringLiteral("/Test Material5.FCMat"),
                      false,  // overwrite
                      true,  // saveAsCopy
                      true);// saveInherited
    EXPECT_EQ(newMaterial->getUUID(), uuid1);
    EXPECT_EQ(newMaterial->getName(), QStringLiteral("Test Material5"));

    _materialManager->saveMaterial(_library,
                      newMaterial,
                      QStringLiteral("/Test Material5.FCMat"),
                      true,  // overwrite
                      true,  // saveAsCopy
                      true);// saveInherited
    EXPECT_EQ(newMaterial->getUUID(), uuid1);
    EXPECT_EQ(newMaterial->getName(), QStringLiteral("Test Material5"));

    // Save to a new file, inheritance mode, testing overwrite as no copy, new copy
    _materialManager->saveMaterial(_library,
                      newMaterial,
                      QStringLiteral("/Test Material6.FCMat"),
                      false,  // overwrite
                      true,  // saveAsCopy
                      true);// saveInherited
    EXPECT_EQ(newMaterial->getUUID(), uuid1);
    EXPECT_EQ(newMaterial->getName(), QStringLiteral("Test Material6"));

    _materialManager->saveMaterial(_library,
                      newMaterial,
                      QStringLiteral("/Test Material6.FCMat"),
                      true,  // overwrite
                      false,  // saveAsCopy
                      true);// saveInherited
    EXPECT_EQ(newMaterial->getUUID(), uuid1);
    EXPECT_EQ(newMaterial->getName(), QStringLiteral("Test Material6"));
}

TEST_F(TestMaterialCards, TestColumns)
{
    ASSERT_NE(_modelManager, nullptr);
    ASSERT_TRUE(_library);

    auto testMaterial = _materialManager->getMaterial(_testMaterialUUID);

    EXPECT_TRUE(testMaterial->hasPhysicalProperty(QStringLiteral("TestArray2D")));
    auto array2d = testMaterial->getPhysicalProperty(QStringLiteral("TestArray2D"))->getMaterialValue();
    EXPECT_TRUE(array2d);
    EXPECT_EQ(dynamic_cast<Materials::Array2D &>(*array2d).columns(), 2);

    EXPECT_TRUE(testMaterial->hasPhysicalProperty(QStringLiteral("TestArray2D3Column")));
    auto array2d3Column = testMaterial->getPhysicalProperty(QStringLiteral("TestArray2D3Column"))->getMaterialValue();
    EXPECT_TRUE(array2d3Column);
    EXPECT_EQ(dynamic_cast<Materials::Array2D &>(*array2d3Column).columns(), 3);

    EXPECT_TRUE(testMaterial->hasPhysicalProperty(QStringLiteral("TestArray3D")));
    auto array3d = testMaterial->getPhysicalProperty(QStringLiteral("TestArray3D"))->getMaterialValue();
    EXPECT_TRUE(array3d);
    EXPECT_EQ(dynamic_cast<Materials::Array3D &>(*array3d).columns(), 2);
}

// clang-format on
