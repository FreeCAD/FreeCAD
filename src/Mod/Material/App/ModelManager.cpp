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

#include "Model.h"
#include "ModelManager.h"
#include "ModelLoader.h"


using namespace Materials;

ModelManager *ModelManager::manager = nullptr;
std::list<ModelLibrary*> *ModelManager::_libraryList = nullptr;
std::map<std::string, Model*> *ModelManager::_modelMap = nullptr;

TYPESYSTEM_SOURCE(Materials::ModelManager, Base::BaseClass)

ModelManager::ModelManager()
{
    // TODO: Add a mutex or similar
    if (_modelMap == nullptr) {
        _modelMap = new std::map<std::string, Model*>();
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
}

bool ModelManager::isModel(const fs::path &p)
{
    if (!fs::is_regular_file(p))
        return false;
    // check file extension
    if (p.extension() == ".yml")
        return true;
    return false;
}

ModelManager *ModelManager::getManager()
{
    if (manager == nullptr)
        manager = new ModelManager();
    return manager;
}
    
const Model &ModelManager::getModel(const std::string &uuid) const
{
    try {
        return *(_modelMap->at(uuid));
    } catch (std::out_of_range e) {
        throw ModelNotFound();
        // return nullptr;
    }
}

const Model &ModelManager::getModelByPath(const std::string &path) const
{
    const std::string &uuid = ModelLoader::getUUIDFromPath(path);
    const Model &model = getModel(uuid);
    // if (model)
        return model;
    // throw ModelNotFound();
}

const Model &ModelManager::getModelByPath(const std::string &path, const std::string &libraryPath) const
{
    QDir modelDir(QDir::cleanPath(QString::fromStdString(libraryPath + "/" + path)));
    std::string absPath = modelDir.absolutePath().toStdString();
    return getModelByPath(absPath);
}


#include "moc_ModelManager.cpp"
