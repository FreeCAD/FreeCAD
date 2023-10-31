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

#include <Mod/Material/MaterialGlobal.h>

#include <QMutex>

#include <boost/filesystem.hpp>

#include "Exceptions.h"
#include "FolderTree.h"
#include "Model.h"

namespace fs = boost::filesystem;

namespace Materials
{

typedef FolderTreeNode<Model> ModelTreeNode;

class MaterialsExport ModelManager: public Base::BaseClass
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    enum ModelFilter
    {
        ModelFilter_None,
        ModelFilter_Physical,
        ModelFilter_Appearance
    };

    ModelManager();
    ~ModelManager() override = default;

    void refresh();

    std::shared_ptr<std::list<ModelLibrary*>> getModelLibraries()
    {
        return _libraryList;
    }
    std::shared_ptr<std::map<QString, Model*>> getModels()
    {
        return _modelMap;
    }
    std::shared_ptr<std::map<QString, ModelTreeNode*>>
    getModelTree(const ModelLibrary& library, ModelFilter filter = ModelFilter_None) const;
    const Model& getModel(const QString& uuid) const;
    const Model& getModelByPath(const QString& path) const;
    const Model& getModelByPath(const QString& path, const QString& libraryPath) const;

    static bool isModel(const fs::path& p);
    bool passFilter(ModelFilter filter, Model::ModelType modelType) const;

private:
    static void initLibraries();

    static std::shared_ptr<std::list<ModelLibrary*>> _libraryList;
    static std::shared_ptr<std::map<QString, Model*>> _modelMap;
    static QMutex _mutex;
};

}  // namespace Materials

#endif  // MATERIAL_MODELMANAGER_H
