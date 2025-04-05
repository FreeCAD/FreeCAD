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

#ifndef MATERIAL_MODELMANAGER_H
#define MATERIAL_MODELMANAGER_H

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

class MaterialsExport ModelManager: public Base::BaseClass
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    ~ModelManager() override;

    static ModelManager& getManager();

    static void cleanup();
    void refresh();

    std::shared_ptr<std::list<std::shared_ptr<ModelLibrary>>> getLibraries();
    std::shared_ptr<std::list<std::shared_ptr<ModelLibrary>>> getLocalLibraries();
    void createLibrary(const QString& libraryName, const QString& icon, bool readOnly = true);
    void createLocalLibrary(const QString& libraryName,
                       const QString& directory,
                       const QString& icon,
                       bool readOnly = true);
    void renameLibrary(const QString& libraryName, const QString& newName);
    void changeIcon(const QString& libraryName, const QString& icon);
    void removeLibrary(const QString& libraryName);
    std::shared_ptr<std::vector<std::tuple<QString, QString, QString>>>
    libraryModels(const QString& libraryName);
    bool isLocalLibrary(const QString& libraryName);

    std::shared_ptr<std::map<QString, std::shared_ptr<Model>>> getModels();
    std::shared_ptr<std::map<QString, std::shared_ptr<Model>>> getLocalModels();
    std::shared_ptr<std::map<QString, std::shared_ptr<ModelTreeNode>>>
    getModelTree(std::shared_ptr<ModelLibrary> library, ModelFilter filter = ModelFilter_None) const
    {
        return library->getModelTree(filter);
    }
    std::shared_ptr<Model> getModel(const QString& uuid) const;
    std::shared_ptr<Model> getModelByPath(const QString& path) const;
    std::shared_ptr<Model> getModelByPath(const QString& path, const QString& lib) const;
    std::shared_ptr<ModelLibrary> getLibrary(const QString& name) const;

    static bool isModel(const QString& file);
    static bool passFilter(ModelFilter filter, Model::ModelType modelType);

private:
    ModelManager();
    static void initManagers();

    static ModelManager* _manager;
    static std::unique_ptr<ModelManagerLocal> _localManager;
    static QMutex _mutex;
};

}  // namespace Materials

#endif  // MATERIAL_MODELMANAGER_H