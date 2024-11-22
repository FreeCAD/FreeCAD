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

#include <App/Application.h>

#include "Exceptions.h"
#include "Library.h"


using namespace Materials;

TYPESYSTEM_SOURCE(Materials::Library, Base::BaseClass)

Library::Library(const QString& libraryName, const QString& icon, bool readOnly)
    : _name(libraryName)
    , _iconPath(icon)
    , _readOnly(readOnly)
{}

bool Library::operator==(const Library& library) const
{
    return (_name == library._name);
}

bool Library::isLocal() const
{
    return false;
}

TYPESYSTEM_SOURCE(Materials::LocalLibrary, Materials::Library)

LocalLibrary::LocalLibrary(const QString& libraryName,
                           const QString& dir,
                           const QString& icon,
                           bool readOnly)
    : Library(libraryName, icon, readOnly)
    , _directory(QDir::cleanPath(dir))
{}

bool LocalLibrary::operator==(const LocalLibrary& library) const
{
    return (getName() == library.getName()) && (_directory == library._directory);
}

bool LocalLibrary::isLocal() const
{
    return true;
}

QString LocalLibrary::getLocalPath(const QString& path) const
{
    QString filePath = getDirectoryPath();
    if (!(filePath.endsWith(QLatin1String("/")) || filePath.endsWith(QLatin1String("\\")))) {
        filePath += QLatin1String("/");
    }

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

bool LocalLibrary::isRoot(const QString& path) const
{
    QString localPath = getLocalPath(path);
    QString cleanPath = getLocalPath(QString::fromStdString(""));
    std::string pLocal = localPath.toStdString();
    std::string pclean = cleanPath.toStdString();
    return (cleanPath == localPath);
}

QString LocalLibrary::getRelativePath(const QString& path) const
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
