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

#include <App/Application.h>

#include "MaterialManager.h"
#include "MaterialLoader.h"


using namespace Materials;

/* TRANSLATOR Material::Materials */

std::list<MaterialLibrary*> *MaterialManager::_libraryList = nullptr;
std::map<std::string, Material*> *MaterialManager::_materialMap = nullptr;
std::map<std::string, Material*> *MaterialManager::_materialPathMap = nullptr;

TYPESYSTEM_SOURCE(Materials::MaterialManager, Base::BaseClass)

MaterialManager::MaterialManager()
{
    // TODO: Add a mutex or similar
    if (_materialMap == nullptr) {
        _materialMap = new std::map<std::string, Material*>();

        if (_materialPathMap == nullptr)
            _materialPathMap = new std::map<std::string, Material*>();
        if (_libraryList == nullptr)
            _libraryList = new std::list<MaterialLibrary*>();

        // Load the libraries
        MaterialLoader loader(_materialMap, _materialPathMap, _libraryList);
    }
}

/*
 *  Destroys the object and frees any allocated resources
 */
MaterialManager::~MaterialManager()
{
}

bool MaterialManager::isCard(const fs::path &p)
{
    if (!fs::is_regular_file(p))
        return false;
    // check file extension
    if (p.extension() == ".FCMat")
        return true;
    return false;
}

std::list<MaterialLibrary *> *MaterialManager::getMaterialLibraries()
{
    if (_libraryList == nullptr)
    {
        if (_materialMap == nullptr)
            _materialMap = new std::map<std::string, Material*>();
        if (_materialPathMap == nullptr)
            _materialPathMap = new std::map<std::string, Material*>();
        _libraryList = new std::list<MaterialLibrary*>();

        // Load the libraries
        MaterialLoader loader(_materialMap, _materialPathMap, _libraryList);
    }
    return _libraryList;
}

const Material &MaterialManager::getMaterialByPath(const std::string &path) const
{
    const std::string &uuid = getUUIDFromPath(path);
    return *(_materialMap->at(uuid));
}

const std::string MaterialManager::getUUIDFromPath(const std::string &path) const
{
    QDir dirPath(QString::fromStdString(path));
    std::string normalized = dirPath.absolutePath().toStdString();
    Material* material = nullptr;
    try {
        material = _materialPathMap->at(normalized);
    } catch (std::out_of_range e) {
        Base::Console().Log("MaterialManager::getUUIDFromPath error: '%s'\n", normalized.c_str());
        return "";
    }
    return material->getUUID();
}

const Material &MaterialManager::getMaterialByPath(const std::string &path, const std::string &libraryPath) const
{
    QDir materialDir(QDir::cleanPath(QString::fromStdString(libraryPath + "/" + path)));
    std::string absPath = materialDir.absolutePath().toStdString();
    return getMaterialByPath(absPath);
}



#include "moc_MaterialManager.cpp"
