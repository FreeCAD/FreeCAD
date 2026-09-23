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

#pragma once

#include <memory>

#include <Base/Parameter.h>
#include <Mod/Material/MaterialGlobal.h>

#include <QMutex>

#include "Exceptions.h"
#include "FolderTree.h"
#include "Model.h"
#include "ModelLibrary.h"

namespace Materials
{
class ModelManagerLocal;
class ModelManagerExternal;

class MaterialsExport ModelManager: public Base::BaseClass, ParameterGrp::ObserverType
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    ~ModelManager() override;

    static ModelManager& getManager();

    static void cleanup();
    void refresh();

    // Library management
    std::shared_ptr<std::list<std::shared_ptr<ModelLibrary>>> getLibraries();
    std::shared_ptr<std::list<std::shared_ptr<ModelLibrary>>> getLocalLibraries();
    std::shared_ptr<ModelLibrary> getLibrary(const std::string& name) const;
    void createLibrary(const std::string& libraryName,
                       const std::string& iconPath,
                       bool readOnly = true);
    void createLocalLibrary(const std::string& libraryName,
                       const std::string& directory,
                       const std::string& icon,
                       bool readOnly = true);
    void renameLibrary(const std::string& libraryName, const std::string& newName);
    void changeIcon(const std::string& libraryName, const std::string& icon);
    void removeLibrary(const std::string& libraryName);
    std::shared_ptr<std::vector<LibraryObject>>
    libraryModels(const std::string& libraryName);
    bool isLocalLibrary(const std::string& libraryName);

    // Folder management

    // Tree management
    std::shared_ptr<std::map<std::string, std::shared_ptr<ModelTreeNode>>>
    getModelTree(std::shared_ptr<ModelLibrary> library, ModelFilter filter = ModelFilter_None) const
    {
        return library->getModelTree(filter);
    }

    // Model management
    std::shared_ptr<std::map<std::string, std::shared_ptr<Model>>> getModels();
    std::shared_ptr<std::map<std::string, std::shared_ptr<Model>>> getLocalModels();
    std::shared_ptr<Model> getModel(const std::string& uuid) const;
    std::shared_ptr<Model> getModel(const std::string& libraryName, const std::string& uuid) const;
    std::shared_ptr<Model> getModelByPath(const std::string& path) const;
    std::shared_ptr<Model> getModelByPath(const std::string& path, const std::string& lib) const;

    static bool isModel(const std::string& file);
    static bool passFilter(ModelFilter filter, Model::ModelType modelType);

    /// Observer message from the ParameterGrp
    void OnChange(ParameterGrp::SubjectType& rCaller, ParameterGrp::MessageType Reason) override;

#if defined(BUILD_MATERIAL_EXTERNAL)
    void migrateToExternal(const std::shared_ptr<Materials::ModelLibrary>& library);
    void validateMigration(const std::shared_ptr<Materials::ModelLibrary>& library);

    // Cache functions
    static double modelHitRate();
#endif

private:
    ModelManager();

    FC_DISABLE_COPY_MOVE(ModelManager);

    static void initManagers();

    static ModelManager* _manager;
    static std::unique_ptr<ModelManagerLocal> _localManager;
#if defined(BUILD_MATERIAL_EXTERNAL)
    static std::unique_ptr<ModelManagerExternal> _externalManager;
#endif
    static QMutex _mutex;
    static bool _useExternal;

    ParameterGrp::handle _hGrp;
};

}  // namespace Materials