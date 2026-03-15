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

#include <QDir>
#include <QByteArray>
#include <QString>

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
    Library(const QString& libraryName, const QString& icon, bool readOnly = true);
    Library(const QString& libraryName, const QByteArray& icon, bool readOnly);
    Library(const QString& libraryName,
            const QString& dir,
            const QString& iconPath,
            bool readOnly = true);
    ~Library() override = default;

    bool isLocal() const;
    void setLocal(bool local);

    QString getName() const
    {
        return _name;
    }
    void setName(const QString& newName)
    {
        _name = newName;
    }
    bool isName(const QString& name)
    {
        return (_name == name);
    }

    QByteArray getIcon() const
    {
        return _icon;
    }
    static QByteArray getIcon(const QString& iconPath);
    void setIcon(const QByteArray& icon)
    {
        _icon = icon;
    }
    void setIcon(const QString& iconPath);
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

    QString getDirectory() const
    {
        return _directory;
    }
    void setDirectory(const QString& directory)
    {
        _directory = directory;
    }
    QString getDirectoryPath() const
    {
        return QDir(_directory).absolutePath();
    }

    bool operator==(const Library& library) const;
    bool operator!=(const Library& library) const
    {
        return !operator==(library);
    }

    QString getLocalPath(const QString& path) const;
    QString getRelativePath(const QString& path) const;
    QString getLibraryPath(const QString& path, const QString& filename) const;
    bool isRoot(const QString& path) const;

    // Validate a remote library against this one (a local library)
    void validate(const Library& remote) const;

private:
    QString _name;
    QString _directory;
    QByteArray _icon;
    bool _readOnly;

    bool _local;

    QByteArray loadByteArrayFromFile(const QString& filePath) const;
};

class MaterialsExport LibraryObject
{
public:
    LibraryObject(const QString& uuid, const QString& path, const QString& name)
        : _uuid(uuid)
        , _path(path)
        , _name(name)
    {}
    LibraryObject(const std::string& uuid, const std::string& path, const std::string& name)
        : _uuid(QString::fromStdString(uuid))
        , _path(QString::fromStdString(path))
        , _name(QString::fromStdString(name))
    {}
    ~LibraryObject() = default;

    void setUUID(const QString& uuid)
    {
        _uuid = uuid;
    }
    void setUUID(const std::string& uuid)
    {
        _uuid = QString::fromStdString(uuid);
    }
    QString getUUID() const
    {
        return _uuid;
    }

    void setPath(const QString& path)
    {
        _path = path;
    }
    void setPath(const std::string& path)
    {
        _path = QString::fromStdString(path);
    }
    QString getPath() const
    {
        return _path;
    }

    void setName(const QString& name)
    {
        _name = name;
    }
    void setName(const std::string& name)
    {
        _name = QString::fromStdString(name);
    }
    QString getName() const
    {
        return _name;
    }

private:
    QString _uuid;
    QString _path;
    QString _name;
};

}  // namespace Materials