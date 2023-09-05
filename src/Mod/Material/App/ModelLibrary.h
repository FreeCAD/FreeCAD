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

#ifndef MATERIAL_MODELLIBRARY_H
#define MATERIAL_MODELLIBRARY_H

#include <Base/BaseClass.h>
#include <Base/Quantity.h>
#include <QDir>
#include <QString>

#include "MaterialValue.h"

namespace Materials
{

class Model;

class MaterialsExport LibraryBase: public Base::BaseClass
{
    TYPESYSTEM_HEADER();

public:
    LibraryBase();
    explicit LibraryBase(const QString& libraryName, const QString& dir, const QString& icon);
    virtual ~LibraryBase() = default;

    const QString getName() const
    {
        return _name;
    }
    const QString getDirectory() const
    {
        return _directory;
    }
    const QString getDirectoryPath() const
    {
        return QDir(_directory).absolutePath();
    }
    const QString getIconPath() const
    {
        return _iconPath;
    }
    bool operator==(const LibraryBase& library) const;
    bool operator!=(const LibraryBase& library) const
    {
        return !operator==(library);
    }
    QString getLocalPath(const QString& path) const;
    QString getRelativePath(const QString& path) const;

private:
    QString _name;
    QString _directory;
    QString _iconPath;
};

class MaterialsExport ModelLibrary: public LibraryBase
{
    TYPESYSTEM_HEADER();

public:
    ModelLibrary();
    explicit ModelLibrary(const QString& libraryName, const QString& dir, const QString& icon);
    virtual ~ModelLibrary() = default;

    bool operator==(const ModelLibrary& library) const
    {
        return LibraryBase::operator==(library);
    }
    bool operator!=(const ModelLibrary& library) const
    {
        return !operator==(library);
    }

    Model* addModel(const Model& model, const QString& path);
};

}// namespace Materials

#endif// MATERIAL_MODELLIBRARY_H
