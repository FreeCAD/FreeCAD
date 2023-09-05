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
    Base::Console().Log("LibraryBase::getFilePath: directory path '%s'\n",
                        getDirectoryPath().toStdString().c_str());

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

    Base::Console().Log("LibraryBase::getFilePath: filePath '%s'\n",
                        filePath.toStdString().c_str());
    return filePath;
}

QString LibraryBase::getRelativePath(const QString& path) const
{
    Base::Console().Log("LibraryBase::getRelativePath: directory path '%s'\n",
                        getDirectoryPath().toStdString().c_str());

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

    Base::Console().Log("LibraryBase::getRelativePath: filePath '%s'\n",
                        filePath.toStdString().c_str());
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

    Base::Console().Log("ModelLibrary::addModel() path='%s'\n", filePath.toStdString().c_str());

    return newModel;
}
