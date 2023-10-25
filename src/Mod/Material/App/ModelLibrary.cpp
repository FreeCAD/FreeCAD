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
#endif

#include <string>

#include <App/Application.h>

#include "Exceptions.h"
#include "Model.h"
#include "ModelLibrary.h"


using namespace Materials;

TYPESYSTEM_SOURCE(Materials::LibraryBase, Base::BaseClass)

LibraryBase::LibraryBase(const QString& libraryName, const QString& dir, const QString& icon)
    : _name(libraryName)
    , _directory(QDir::cleanPath(dir))
    , _iconPath(icon)
{}

LibraryBase::LibraryBase()
{}

bool LibraryBase::operator==(const LibraryBase& library) const
{
    return (_name == library._name) && (_directory == library._directory);
}

QString LibraryBase::getLocalPath(const QString& path) const
{
    QString filePath = getDirectoryPath();
    QString cleanPath = QDir::cleanPath(path);
    QString prefix = QString::fromStdString("/") + getName();
    if (cleanPath.startsWith(prefix)) {
        // Remove the library name from the path
        filePath += cleanPath.right(cleanPath.length() - prefix.length());
    }
    else {
        filePath += cleanPath;
    }

    return filePath;
}

QString LibraryBase::getRelativePath(const QString& path) const
{
    QString filePath;
    QString cleanPath = QDir::cleanPath(path);
    QString prefix = QString::fromStdString("/") + getName();
    if (cleanPath.startsWith(prefix)) {
        // Remove the library name from the path
        filePath = cleanPath.right(cleanPath.length() - prefix.length());
    }
    else {
        filePath = cleanPath;
    }

    prefix = getDirectoryPath();
    if (filePath.startsWith(prefix)) {
        // Remove the library root from the path
        filePath = filePath.right(filePath.length() - prefix.length());
    }

    // Remove any leading '/'
    if (filePath.startsWith(QString::fromStdString("/"))) {
        filePath.remove(0, 1);
    }

    return filePath;
}

TYPESYSTEM_SOURCE(Materials::ModelLibrary, LibraryBase)

ModelLibrary::ModelLibrary(const QString& libraryName, const QString& dir, const QString& icon)
    : LibraryBase(libraryName, dir, icon)
{}

ModelLibrary::ModelLibrary()
{}

Model* ModelLibrary::addModel(const Model& model, const QString& path)
{
    QString filePath = getRelativePath(path);
    Model* newModel = new Model(model);
    newModel->setLibrary(*this);
    newModel->setDirectory(filePath);

    return newModel;
}
