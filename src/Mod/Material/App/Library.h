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

#ifndef MATERIAL_LIBRARY_H
#define MATERIAL_LIBRARY_H

#include <QDir>
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
    Library(const QString& libraryName,
            const QString& icon,
            bool readOnly,
            const QString& timestamp);
    Library(const QString& libraryName,
            const QString& dir,
            const QString& icon,
            bool readOnly = true);
    ~Library() override = default;

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

    QString getIconPath() const
    {
        return _iconPath;
    }
    void setIconPath(const QString& icon)
    {
        _iconPath = icon;
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
    QString getTimestamp() const
    {
        return _timestamp;
    }
    void setTimestamp(const QString& timestamp)
    {
        _timestamp = timestamp;
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
    QString _iconPath;
    bool _readOnly;
    QString _timestamp;
};

}  // namespace Materials

#endif  // MATERIAL_LIBRARY_H
