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

#pragma once

#include <string>
#include <utility>
#include <vector>

#include <QByteArray>

#include <Base/BaseClass.h>

#include <Mod/Material/MaterialGlobal.h>

namespace Materials
{

class MaterialsExport Library: public Base::BaseClass
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    Library() = default;
    Library(const Library &other) = default;
    Library(const std::string& libraryName, const std::string& icon, bool readOnly = true);
    Library(const std::string& libraryName, const QByteArray& icon, bool readOnly);
    Library(const std::string& libraryName,
            const std::string& dir,
            const std::string& iconPath,
            bool readOnly = true);
    ~Library() override = default;

    bool isLocal() const;
    void setLocal(bool local);

    const std::string& getName() const
    {
        return _name;
    }
    void setName(std::string newName)
    {
        _name = std::move(newName);
    }
    bool isName(const std::string& name)
    {
        return (_name == name);
    }

    QByteArray getIcon() const
    {
        return _icon;
    }
    static QByteArray getIcon(const std::string& iconPath);
    void setIcon(const QByteArray& icon)
    {
        _icon = icon;
    }
    void setIcon(const std::string& iconPath);
    bool hasIcon() const
    {
        return !_icon.isEmpty();
    }
    bool isReadOnly() const
    {
        return _readOnly;
    }
    void setReadOnly(bool readOnly)
    {
        _readOnly = readOnly;
    }

    const std::string& getDirectory() const;
    void setDirectory(const std::string& directory);
    std::string getDirectoryPath() const;

    bool operator==(const Library& library) const;
    bool operator!=(const Library& library) const
    {
        return !operator==(library);
    }

    std::string getLocalPath(const std::string& path) const;
    std::string getRelativePath(const std::string& path) const;
    std::string getLibraryPath(const std::string& path, const std::string& filename) const;
    bool isRoot(const std::string& path) const;

    // Validate a remote library against this one (a local library)
    void validate(const Library& remote) const;

    static std::string canonical(const std::string& path);
    static std::string cleanPath(const std::string& path);
    /// Split \a text on \a separator, keeping empty parts
    static std::vector<std::string> split(const std::string& text, char separator);

    /// Does \a path start with this library's directory, as the file system compares names?
    bool startsWithDirectory(const std::string& path) const;

private:
    std::string _name;
    std::string _directory;
    QByteArray _icon;
    bool _readOnly;
    bool _caseSensitive;

    bool _local;

    QByteArray loadByteArrayFromFile(const std::string& filePath) const;
    void setCaseSensitivity();
};

class MaterialsExport LibraryObject
{
public:
    LibraryObject(std::string uuid, std::string path, std::string name)
        : _uuid(std::move(uuid))
        , _path(std::move(path))
        , _name(std::move(name))
    {}
    ~LibraryObject() = default;

    void setUUID(std::string uuid)
    {
        _uuid = std::move(uuid);
    }
    const std::string& getUUID() const
    {
        return _uuid;
    }

    void setPath(std::string path)
    {
        _path = std::move(path);
    }
    const std::string& getPath() const
    {
        return _path;
    }

    void setName(std::string name)
    {
        _name = std::move(name);
    }
    const std::string& getName() const
    {
        return _name;
    }

private:
    std::string _uuid;
    std::string _path;
    std::string _name;
};

}  // namespace Materials