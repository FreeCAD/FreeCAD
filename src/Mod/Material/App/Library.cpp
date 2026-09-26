// SPDX-License-Identifier: LGPL-2.1-or-later

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

#include <string>
#include <utility>
#include <vector>

#include <App/Application.h>

#include "Exceptions.h"
#include "Library.h"

#include <QDir>
#include <QFile>

using namespace Materials;

TYPESYSTEM_SOURCE(Materials::Library, Base::BaseClass)

Library::Library(const std::string& libraryName, const std::string& iconPath, bool readOnly)
    : _name(libraryName)
    , _readOnly(readOnly)
    , _caseSensitive(true)
    , _local(false)
{
    setIcon(iconPath);
}

Library::Library(const std::string& libraryName, const QByteArray& icon, bool readOnly)
    : _name(libraryName)
    , _icon(icon)
    , _readOnly(readOnly)
    , _caseSensitive(true)
    , _local(false)
{}

Library::Library(const std::string& libraryName,
                 const std::string& dir,
                 const std::string& iconPath,
                 bool readOnly)
    : _name(libraryName)
    , _directory(canonical(dir))
    , _readOnly(readOnly)
    , _local(false)
{
    setIcon(iconPath);
    setCaseSensitivity();
}

QByteArray Library::getIcon(const std::string& iconPath)
{
    QFile file(QString::fromStdString(iconPath));
    if (!file.open(QIODevice::ReadOnly)) {
        Base::Console().log("Failed to open icon file '{}'\n", iconPath);
        return QByteArray();  // Return an empty QByteArray if file opening fails
    }

    QByteArray data = file.readAll();
    file.close();
    return data;
}

void Library::setIcon(const std::string& iconPath)
{
    _icon = getIcon(iconPath);
}

bool Library::isLocal() const
{
    return _local;
}

void Library::setLocal(bool local)
{
    _local = local;
}

const std::string& Library::getDirectory() const
{
    return _directory;
}

void Library::setDirectory(const std::string& directory)
{
    _directory = canonical(directory);
    setCaseSensitivity();
}

void Library::setCaseSensitivity()
{
    const QString directory = QString::fromStdString(_directory);

    _caseSensitive = true;
    if (QDir(directory).exists()) {
        const auto upper = directory.toUpper();
        const auto lower = directory.toLower();
        if ((directory != upper) && QDir(upper).exists()) {
            _caseSensitive = false;
        }
        else if ((directory != lower) && QDir(lower).exists()) {
            _caseSensitive = false;
        }
    }
}

bool Library::startsWithDirectory(const std::string& path) const
{
    // the comparison follows the file system, so let Qt fold the case
    return QString::fromStdString(path).startsWith(QString::fromStdString(_directory),
                                                   _caseSensitive ? Qt::CaseSensitive
                                                                  : Qt::CaseInsensitive);
}

std::string Library::getDirectoryPath() const
{
    return QDir(QString::fromStdString(_directory)).canonicalPath().toStdString();
}

bool Library::operator==(const Library& library) const
{
    return (getName() == library.getName()) && (_directory == library._directory);
}

void Library::validate(const Library& remote) const
{
    if (getName() != remote.getName()) {
        throw InvalidLibrary("Library names don't match");
    }
    if (getIcon() != remote.getIcon()) {
        throw InvalidLibrary("Library icons don't match");
    }

    // Local and remote paths will differ
    if (!remote.getDirectory().empty()) {
        throw InvalidLibrary("Remote library should not have a path");
    }

    if (isReadOnly() != remote.isReadOnly()) {
        throw InvalidLibrary("Library readonly settings don't match");
    }
}

std::string Library::getLocalPath(const std::string& path) const
{
    std::string filePath = getDirectoryPath();
    if (!filePath.ends_with('/') && !filePath.ends_with('\\')) {
        filePath += '/';
    }

    const std::string clean = cleanPath(path);
    const std::string prefix = "/" + getName();
    if (clean.starts_with(prefix)) {
        // Remove the library name from the path
        filePath += clean.substr(prefix.length());
    }
    else {
        filePath += clean;
    }

    return filePath;
}

bool Library::isRoot(const std::string& path) const
{
    return getLocalPath("") == getLocalPath(path);
}

std::string Library::getRelativePath(const std::string& path) const
{
    std::string filePath;
    const std::string clean = cleanPath(path);
    const std::string prefix = "/" + getName();
    if (clean.starts_with(prefix)) {
        // Remove the library name from the path
        filePath = clean.substr(prefix.length());
    }
    else {
        filePath = clean;
    }

    if (startsWithDirectory(filePath)) {
        // Remove the library root from the path
        filePath = filePath.substr(getDirectoryPath().length());
    }

    // Remove any leading '/'
    if (filePath.starts_with('/')) {
        filePath.erase(0, 1);
    }

    return filePath;
}

std::string Library::getLibraryPath(const std::string& path, const std::string& filename) const
{
    std::string filePath {path};
    if (filePath.ends_with(filename)) {
        filePath.erase(filePath.length() - filename.length());
    }
    if (filePath.ends_with('/')) {
        filePath.pop_back();
    }

    return filePath;
}

std::string Library::canonical(const std::string& path)
{
    return QDir(QString::fromStdString(path)).canonicalPath().toStdString();
}

std::string Library::cleanPath(const std::string& path)
{
    return QDir::cleanPath(QString::fromStdString(path)).toStdString();
}

std::vector<std::string> Library::split(const std::string& text, char separator)
{
    std::vector<std::string> parts;

    for (std::size_t pos = 0;;) {
        const auto end = text.find(separator, pos);
        parts.push_back(text.substr(pos, end == std::string::npos ? end : end - pos));
        if (end == std::string::npos) {
            return parts;
        }
        pos = end + 1;
    }
}
