/***************************************************************************
 *   Copyright (c) 2024 David Carter <dcarter@david.carter.ca>             *
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
#endif

#include <QDirIterator>
#include <QMutexLocker>

#include "Model.h"
#include "ModelLoader.h"
#include "ModelManagerExternal.h"
#include "ExternalManager.h"

using namespace Materials;

TYPESYSTEM_SOURCE(Materials::ModelManagerExternal, Base::BaseClass)

ModelManagerExternal::ModelManagerExternal()
{
}

void ModelManagerExternal::cleanup()
{
}

void ModelManagerExternal::refresh()
{
}

//=====
//
// Library management
//
//=====

std::shared_ptr<std::list<std::shared_ptr<ModelLibrary>>> ModelManagerExternal::getLibraries()
{
    auto libraryList = std::make_shared<std::list<std::shared_ptr<ModelLibrary>>>();
    auto externalLibraries = ExternalManager::getManager()->libraries();
    for (auto& entry : *externalLibraries) {
        auto libName = std::get<0>(entry);
        auto icon = std::get<1>(entry);
        auto readOnly = std::get<2>(entry);
        Base::Console().Log("Library name '%s', Icon '%s', readOnly %s\n",
                            libName.toStdString().c_str(),
                            icon.toStdString().c_str(),
                            readOnly ? "true" : "false");
        auto library = std::make_shared<ModelLibrary>(libName, QString(), icon, readOnly);
        libraryList->push_back(library);
    }

    return libraryList;
}

void ModelManagerExternal::createLibrary(const QString& libraryName,
                                      const QString& icon,
                                      bool readOnly)
{
    ExternalManager::getManager()->createLibrary(libraryName, icon, readOnly);
}

//=====
//
// Model management
//
//=====

void ModelManagerExternal::addModel(const QString& libraryName,
                                    const QString& path,
                                    const std::shared_ptr<Model>& model)
{
    ExternalManager::getManager()->addModel(libraryName, path, model);
}
