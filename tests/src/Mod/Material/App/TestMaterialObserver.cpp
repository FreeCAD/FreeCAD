// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Joao Matos
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/

#include <gtest/gtest.h>

#include <QString>

#include <vector>

#include <App/Application.h>
#include <Base/Interpreter.h>
#include <src/App/InitApplication.h>
#include <src/TempDirectory.h>

#include <Mod/Material/App/MaterialLibrary.h>
#include <Mod/Material/App/MaterialManager.h>
#include <Mod/Material/App/ModelManager.h>

class TestMaterialObserver: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        if (App::Application::GetARGC() == 0) {
            tests::initApplication();
        }
    }

    void SetUp() override
    {
        Base::Interpreter().runString("import Part");

        QString libPath = QString::fromStdString(_tempDir.string());
        _library = std::make_shared<Materials::MaterialLibraryLocal>(
            QStringLiteral("Testing"),
            libPath,
            QStringLiteral(":/icons/preferences-general.svg"),
            false
        );
        _modelManager = &(Materials::ModelManager::getManager());
        _materialManager = &(Materials::MaterialManager::getManager());

        createdConn = _materialManager->signalCreatedMaterial.connect(
            [this](const Materials::Material& material) { createdEvents.push_back(material); }
        );
        changedConn = _materialManager->signalChangedMaterial.connect(
            [this](const Materials::Material& material) { changedEvents.push_back(material); }
        );
        deletedConn = _materialManager->signalDeletedMaterial.connect(
            [this](const Materials::Material& material) { deletedEvents.push_back(material); }
        );
    }

    void TearDown() override
    {
        ASSERT_NE(_materialManager, nullptr);
        ASSERT_TRUE(_library);

        for (const auto& path :
             {QStringLiteral("/SignalCard.FCMat"),
              QStringLiteral("/Folder"),
              QStringLiteral("/Renamed")}) {
            try {
                _materialManager->deleteRecursive(_library, path);
            }
            catch (...) {
            }
        }

        createdConn.disconnect();
        changedConn.disconnect();
        deletedConn.disconnect();

        _modelManager->refresh();
        _materialManager->refresh();
    }

    tests::TempDirectory _tempDir {"TestMaterialObserver"};
    Materials::ModelManager* _modelManager {};
    Materials::MaterialManager* _materialManager {};
    std::shared_ptr<Materials::MaterialLibraryLocal> _library;
    std::vector<Materials::Material> createdEvents;
    std::vector<Materials::Material> changedEvents;
    std::vector<Materials::Material> deletedEvents;
    fastsignals::scoped_connection createdConn;
    fastsignals::scoped_connection changedConn;
    fastsignals::scoped_connection deletedConn;
};

TEST_F(TestMaterialObserver, TestMaterialLifecycleSignals)
{
    ASSERT_NE(_materialManager, nullptr);
    ASSERT_TRUE(_library);

    auto material = std::make_shared<Materials::Material>();
    material->setDescription(QStringLiteral("created"));
    QString uuid = material->getUUID();

    _materialManager
        ->saveMaterial(_library, material, QStringLiteral("/SignalCard.FCMat"), true, false, false);

    ASSERT_EQ(createdEvents.size(), 1);
    EXPECT_EQ(createdEvents.front().getUUID(), uuid);
    EXPECT_EQ(createdEvents.front().getName(), QStringLiteral("SignalCard"));
    EXPECT_TRUE(changedEvents.empty());
    EXPECT_TRUE(deletedEvents.empty());

    material->setDescription(QStringLiteral("changed"));
    _materialManager
        ->saveMaterial(_library, material, QStringLiteral("/SignalCard.FCMat"), true, false, false);

    ASSERT_EQ(changedEvents.size(), 1);
    EXPECT_EQ(changedEvents.front().getUUID(), uuid);
    EXPECT_EQ(changedEvents.front().getName(), QStringLiteral("SignalCard"));

    _materialManager->deleteRecursive(_library, QStringLiteral("/SignalCard.FCMat"));

    ASSERT_EQ(deletedEvents.size(), 1);
    EXPECT_EQ(deletedEvents.front().getUUID(), uuid);
    EXPECT_EQ(deletedEvents.front().getName(), QStringLiteral("SignalCard"));
}

TEST_F(TestMaterialObserver, TestRenameFolderSignalsChangedMaterial)
{
    ASSERT_NE(_materialManager, nullptr);
    ASSERT_TRUE(_library);

    auto material = std::make_shared<Materials::Material>();
    _materialManager->saveMaterial(
        _library,
        material,
        QStringLiteral("/Folder/RenamedCard.FCMat"),
        true,
        false,
        false
    );
    createdEvents.clear();
    changedEvents.clear();
    deletedEvents.clear();
    bool renamedPathVisibleDuringCallback = false;
    auto verifyConn = _materialManager->signalChangedMaterial.connect(
        [this, &renamedPathVisibleDuringCallback](const Materials::Material& changed) {
            try {
                auto renamed = _library->getMaterialByPath(QStringLiteral("Renamed/RenamedCard.FCMat"));
                renamedPathVisibleDuringCallback = renamed->getUUID() == changed.getUUID();
            }
            catch (...) {
            }
        }
    );

    _materialManager->renameFolder(_library, QStringLiteral("/Folder"), QStringLiteral("/Renamed"));

    ASSERT_EQ(changedEvents.size(), 1);
    EXPECT_EQ(changedEvents.front().getUUID(), material->getUUID());
    EXPECT_EQ(changedEvents.front().getDirectory(), QStringLiteral("Renamed"));
    EXPECT_TRUE(renamedPathVisibleDuringCallback);

    auto renamed = _library->getMaterialByPath(QStringLiteral("Renamed/RenamedCard.FCMat"));
    EXPECT_EQ(renamed->getDirectory(), QStringLiteral("Renamed"));

    verifyConn.disconnect();
}
