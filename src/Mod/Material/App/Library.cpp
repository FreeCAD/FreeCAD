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

Library::Library(const QString& libraryName, const QString& dir, const QString& icon, bool readOnly)
    : _name(libraryName)
    , _iconPath(icon)
    , _readOnly(readOnly)
    , _directory(QDir::cleanPath(dir))
{}

bool Library::operator==(const Library& library) const
{
    return (getName() == library.getName()) && (_directory == library._directory);
}

void Library::validate(const Library& remote) const
{
    if (getName() != remote.getName()) {
        throw InvalidLibrary("Library names don't match");
    }
    if (getIconPath() != remote.getIconPath()) {
        Base::Console().Log("Icon path 1 '%s'\n", getIconPath().toStdString().c_str());
        Base::Console().Log("Icon path 2 '%s'\n", remote.getIconPath().toStdString().c_str());
        throw InvalidLibrary("Library icon paths don't match");
    }

    // Local and remote paths will differ
    if (!remote.getDirectory().isEmpty()) {
        throw InvalidLibrary("Remote library should not have a path");
    }

    if (isReadOnly() != remote.isReadOnly()) {
        throw InvalidLibrary("Library readonly settings don't match");
    }
}

QString Library::getLocalPath(const QString& path) const
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

bool Library::isRoot(const QString& path) const
{
    QString localPath = getLocalPath(path);
    QString cleanPath = getLocalPath(QString::fromStdString(""));
    std::string pLocal = localPath.toStdString();
    std::string pclean = cleanPath.toStdString();
    return (cleanPath == localPath);
}

QString Library::getRelativePath(const QString& path) const
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

QString Library::getLibraryPath(const QString& path, const QString& filename) const
{
    QString filePath(path);
    if (filePath.endsWith(filename)) {
        filePath = filePath.left(filePath.length() - filename.length());
    }
    if (filePath.endsWith(QLatin1String("/"))) {
        filePath = filePath.left(filePath.length() - 1);
    }

    return filePath;
}
