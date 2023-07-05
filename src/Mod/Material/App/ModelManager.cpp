/***************************************************************************
 *   Copyright (c) 2023 David Carter <dcarter@david.carter.ca>             *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#include "PreCompiled.h"
#ifndef _PreComp_
#endif

#include <Base/Console.h>

#include "Model.h"
#include "ModelManager.h"
#include "ModelLoader.h"


using namespace Materials;

ModelManager *ModelManager::manager = nullptr;
std::list<ModelLibrary*> *ModelManager::_libraryList = nullptr;
std::map<QString, Model*> *ModelManager::_modelMap = nullptr;

TYPESYSTEM_SOURCE(Materials::ModelManager, Base::BaseClass)

ModelManager::ModelManager()
{
    // TODO: Add a mutex or similar
    if (_modelMap == nullptr) {
        _modelMap = new std::map<QString, Model*>();
        if (_libraryList == nullptr)
            _libraryList = new std::list<ModelLibrary*>();

        // Load the libraries
        ModelLoader loader(_modelMap, _libraryList);
    }
}

/*
 *  Destroys the object and frees any allocated resources
 */
ModelManager::~ModelManager()
{
    manager = nullptr;
}

bool ModelManager::isModel(const fs::path &p)
{
    // if (!fs::is_regular_file(p))
    //     return false;
    // check file extension
    if (p.extension() == ".yml")
        return true;
    return false;
}

ModelManager *ModelManager::getManager()
{
    // if (manager == nullptr)
    //     manager = new ModelManager();
    // return manager;
    return new ModelManager();
}

void ModelManager::refresh()
{
    delete _modelMap;
    delete _libraryList;

    _modelMap = new std::map<QString, Model*>();
    _libraryList = new std::list<ModelLibrary*>();

    // Load the libraries
    ModelLoader loader(_modelMap, _libraryList);
}
    
const Model &ModelManager::getModel(const QString &uuid) const
{
    try {
        return *(_modelMap->at(uuid));
    } catch (std::out_of_range const &) {
        throw ModelNotFound();
    }
}

const Model &ModelManager::getModelByPath(const QString &path) const
{
    const QString &uuid = ModelLoader::getUUIDFromPath(path);
    const Model &model = getModel(uuid);

    return model;
}

const Model &ModelManager::getModelByPath(const QString &path, const QString &libraryPath) const
{
    QDir modelDir(QDir::cleanPath(libraryPath + QString::fromStdString("/") + path));
    QString absPath = modelDir.absolutePath();
    return getModelByPath(absPath);
}

bool ModelManager::passFilter(ModelFilter filter, Model::ModelType modelType) const
{
    switch (filter)
    {
        case ModelFilter_None:
            return true;

        case ModelFilter_Physical:
            return (modelType == Model::ModelType_Physical);

        case ModelFilter_Appearance:
            return (modelType == Model::ModelType_Appearance);
    }

    return false;
}

std::map<QString, ModelTreeNode*>* ModelManager::getModelTree(const ModelLibrary &library, ModelFilter filter)
{
    std::map<QString, ModelTreeNode*> *modelTree = new std::map<QString, ModelTreeNode*>();

    for (auto it = _modelMap->begin(); it != _modelMap->end(); it++)
    {
        auto filename = it->first;
        auto model = it->second;

        if (model->getLibrary() == library && passFilter(filter, model->getType()))
        {
            fs::path path = model->getRelativePath().toStdString();
            Base::Console().Log("Relative path '%s'\n\t", path.string().c_str());

            // Start at the root
            std::map<QString, ModelTreeNode*> *node = modelTree;
            for (auto itp = path.begin(); itp != path.end(); itp++)
            {
                if (isModel(itp->string())) {
                    ModelTreeNode *child = new ModelTreeNode();
                    child->setData(model);
                    (*node)[QString::fromStdString(itp->string())] = child;
                } else {
                    // Add the folder only if it's not already there
                    QString folderName = QString::fromStdString(itp->string());
                    std::map<QString, ModelTreeNode*> *mapPtr;
                    if (node->count(QString::fromStdString(itp->string())) == 0)
                    {
                        mapPtr = new std::map<QString, ModelTreeNode*>();
                        ModelTreeNode *child = new ModelTreeNode();
                        child->setFolder(mapPtr);
                        (*node)[QString::fromStdString(itp->string())] = child;
                        node = mapPtr;
                    } else {
                        node = (*node)[QString::fromStdString(itp->string())]->getFolder();
                    }
                }
                Base::Console().Log("'%s' ", itp->string().c_str());
            }
            Base::Console().Log("\n");
        }
    }

    return modelTree;
}


#include "moc_ModelManager.cpp"
