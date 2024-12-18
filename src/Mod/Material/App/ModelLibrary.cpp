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

#include "PreCompiled.h"
#ifndef _PreComp_
#include <string>
#endif

#include <QFileInfo>

#include <App/Application.h>

#include "Exceptions.h"
#include "Model.h"
#include "ModelLibrary.h"
#include "ModelManager.h"


using namespace Materials;

TYPESYSTEM_SOURCE(Materials::ModelLibrary, Materials::Library)

ModelLibrary::ModelLibrary(const QString& libraryName,
                           const QString& dir,
                           const QString& icon,
                           bool readOnly)
    : Library(libraryName, dir, icon, readOnly)
{
    _modelPathMap = std::make_unique<std::map<QString, std::shared_ptr<Model>>>();
}

ModelLibrary::ModelLibrary()
{
    _modelPathMap = std::make_unique<std::map<QString, std::shared_ptr<Model>>>();
}

std::shared_ptr<Model> ModelLibrary::getModelByPath(const QString& path) const
{
    QString filePath = getRelativePath(path);
    try {
        std::shared_ptr<Model> model = _modelPathMap->at(filePath);
        return model;
    }
    catch (std::out_of_range&) {
        throw ModelNotFound();
    }
}

std::shared_ptr<Model> ModelLibrary::addModel(const Model& model, const QString& path)
{
    QString filePath = getRelativePath(path);
    QFileInfo info(filePath);
    std::shared_ptr<Model> newModel = std::make_shared<Model>(model);
    newModel->setLibrary(getptr());
    newModel->setDirectory(getLibraryPath(filePath, info.fileName()));
    newModel->setFilename(info.fileName());

    (*_modelPathMap)[filePath] = newModel;

    return newModel;
}

std::shared_ptr<std::map<QString, std::shared_ptr<ModelTreeNode>>>
ModelLibrary::getModelTree(ModelFilter filter) const
{
    std::shared_ptr<std::map<QString, std::shared_ptr<ModelTreeNode>>> modelTree =
        std::make_shared<std::map<QString, std::shared_ptr<ModelTreeNode>>>();

    for (auto& it : *_modelPathMap) {
        auto filename = it.first;
        auto model = it.second;

        if (ModelManager::passFilter(filter, model->getType())) {
            QStringList list = filename.split(QStringLiteral("/"));

            // Start at the root
            std::shared_ptr<std::map<QString, std::shared_ptr<ModelTreeNode>>> node = modelTree;
            for (auto& itp : list) {
                if (ModelManager::isModel(itp)) {
                    std::shared_ptr<ModelTreeNode> child = std::make_shared<ModelTreeNode>();
                    child->setUUID(model->getUUID());
                    child->setData(model);
                    (*node)[itp] = child;
                }
                else {
                    // Add the folder only if it's not already there
                    if (node->count(itp) == 0) {
                        auto mapPtr =
                            std::make_shared<std::map<QString, std::shared_ptr<ModelTreeNode>>>();
                        std::shared_ptr<ModelTreeNode> child = std::make_shared<ModelTreeNode>();
                        child->setFolder(mapPtr);
                        (*node)[itp] = child;
                        node = mapPtr;
                    }
                    else {
                        node = (*node)[itp]->getFolder();
                    }
                }
            }
        }
    }

    return modelTree;
}
