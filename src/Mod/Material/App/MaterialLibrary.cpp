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

#include "MaterialLibrary.h"
#include "Materials.h"
#include "ModelManager.h"


using namespace Materials;

/* TRANSLATOR Material::Materials */

std::map<QString, Material*>* MaterialLibrary::_materialPathMap = nullptr;

TYPESYSTEM_SOURCE(Materials::MaterialLibrary, LibraryBase)

MaterialLibrary::MaterialLibrary()
{
    if (_materialPathMap == nullptr) {
        _materialPathMap = new std::map<QString, Material*>();
    }
}

MaterialLibrary::MaterialLibrary(const QString& libraryName,
                                 const QString& dir,
                                 const QString& icon,
                                 bool readOnly)
    : LibraryBase(libraryName, dir, icon)
    , _readOnly(readOnly)
{
    if (_materialPathMap == nullptr) {
        _materialPathMap = new std::map<QString, Material*>();
    }
}

void MaterialLibrary::createPath(const QString& path)
{
    Q_UNUSED(path)
}

Material* MaterialLibrary::saveMaterial(Material& material, const QString& path, bool saveAsCopy)
{
    Base::Console().Log("MaterialLibrary::saveMaterial(material(%s), path(%s))\n",
                        material.getName().toStdString().c_str(),
                        path.toStdString().c_str());

    QString filePath = getLocalPath(path);
    Base::Console().Log("\tfilePath = '%s'\n", filePath.toStdString().c_str());
    QFile file(filePath);

    // Update UUID if required
    // if name changed true
    if (material.getName() != file.fileName()) {
        material.newUuid();
    }
    // if overwrite false having warned the user
    // if old format true, but already set


    QFileInfo info(file);
    QDir fileDir(info.path());
    if (!fileDir.exists()) {
        if (!fileDir.mkpath(info.path())) {
            Base::Console().Error("Unable to create directory path '%s'\n",
                                  info.path().toStdString().c_str());
        }
    }

    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        stream.setCodec("UTF-8");
        stream.setGenerateByteOrderMark(true);

        // Write the contents
        material.setLibrary(*this);
        Base::Console().Log("\trelativePath = '%s'\n", getRelativePath(path).toStdString().c_str());
        material.setDirectory(getRelativePath(path));
        material.save(stream, saveAsCopy);
    }

    return addMaterial(material, path);
}

Material* MaterialLibrary::addMaterial(const Material& material, const QString& path)
{
    QString filePath = getRelativePath(path);
    Material* newMaterial = new Material(material);
    newMaterial->setLibrary(*this);
    newMaterial->setDirectory(filePath);

    Base::Console().Log("MaterialLibrary::addMaterial() path='%s'\n",
                        filePath.toStdString().c_str());

    try {
        // If there's already a material at that path we'll replace it
        Material* old = _materialPathMap->at(filePath);
        delete old;
    }
    catch (const std::out_of_range&) {
    }

    (*_materialPathMap)[filePath] = newMaterial;

    return newMaterial;
}

const Material& MaterialLibrary::getMaterialByPath(const QString& path) const
{
    // Base::Console().Log("MaterialLibrary::getMaterialByPath(%s)\n", path.toStdString().c_str());
    // for (auto itp = _materialPathMap->begin(); itp != _materialPathMap->end(); itp++) {
    //     Base::Console().Log("\tpath = '%s'\n", itp->first.toStdString().c_str());
    // }

    QString filePath = getRelativePath(path);
    try {
        Material* material = _materialPathMap->at(filePath);
        return *material;
    }
    catch (std::out_of_range& e) {
        throw MaterialNotFound();
    }
}

const QString MaterialLibrary::getUUIDFromPath(const QString& path) const
{
    QString filePath = getRelativePath(path);
    try {
        Material* material = _materialPathMap->at(filePath);
        return material->getUUID();
    }
    catch (std::out_of_range& e) {
        throw MaterialNotFound();
    }
}

TYPESYSTEM_SOURCE(Materials::MaterialExternalLibrary, MaterialLibrary::MaterialLibrary)

MaterialExternalLibrary::MaterialExternalLibrary()
{}

MaterialExternalLibrary::MaterialExternalLibrary(const QString& libraryName,
                                                 const QString& dir,
                                                 const QString& icon,
                                                 bool readOnly)
    : MaterialLibrary(libraryName, dir, icon, readOnly)
{}

MaterialExternalLibrary::~MaterialExternalLibrary()
{
    // delete directory;
}
