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

#ifndef MATERIAL_MATERIALLIBRARY_H
#define MATERIAL_MATERIALLIBRARY_H

#include <Base/BaseClass.h>
#include <QDir>
#include <QString>
#include <QVariant>

#include "Model.h"

namespace Materials {

class Material;

class MaterialsExport MaterialLibrary : public Base::BaseClass
{
    TYPESYSTEM_HEADER();

public:
    MaterialLibrary();
    explicit MaterialLibrary(const QString &libraryName, const QDir &dir, const QString &icon, bool readOnly = true);
    virtual ~MaterialLibrary();

    const QString getName() const { return _name; }
    const QDir getDirectory() const { return _directory; }
    const QString getDirectoryPath() const { return _directory.absolutePath(); }
    const QString getIconPath() const { return _iconPath; }
    bool operator==(const MaterialLibrary& library) const
    {
        return (_name == library._name) && (_directory == library._directory);
    }
    bool operator!=(const MaterialLibrary& library) const { return !operator==(library); }

    void createPath(const QString& path);
    void saveMaterial(Material& material, const QString& path, bool saveAsCopy);

    bool isReadOnly() const { return _readOnly; }

protected:
    QString getFilePath(const QString &path) const;

    QString _name;
    QDir _directory;
    QString _iconPath;
    bool _readOnly;
};

class MaterialsExport MaterialExternalLibrary : public MaterialLibrary
{
    TYPESYSTEM_HEADER();

public:
    MaterialExternalLibrary();
    explicit MaterialExternalLibrary(const QString &libraryName, const QDir &dir, const QString &icon, bool readOnly = true);
    virtual ~MaterialExternalLibrary();
};

} // namespace Materials

Q_DECLARE_METATYPE(Materials::MaterialLibrary)
Q_DECLARE_METATYPE(Materials::MaterialExternalLibrary)

#endif // MATERIAL_MATERIALLIBRARY_H
