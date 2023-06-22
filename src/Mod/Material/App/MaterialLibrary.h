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

#include "Model.h"

namespace fs = boost::filesystem;

namespace Materials {

class Material;

class MaterialsExport MaterialLibrary : public Base::BaseClass
{
    TYPESYSTEM_HEADER();

public:
    explicit MaterialLibrary();
    explicit MaterialLibrary(const std::string &libraryName, const QDir &dir, const std::string &icon, bool readOnly = true);
    virtual ~MaterialLibrary();

    const std::string &getName() const { return _name; }
    const QDir &getDirectory() const { return _directory; }
    const std::string getDirectoryPath() const { return _directory.absolutePath().toStdString(); }
    const std::string &getIconPath() const { return _iconPath; }
    void createPath(const std::string& path);
    void saveMaterial(const Material& material, const std::string& path);

protected:
    std::string _name;
    QDir _directory;
    std::string _iconPath;
    bool _readOnly;
};

class MaterialsExport MaterialExternalLibrary : public MaterialLibrary
{
    TYPESYSTEM_HEADER();

public:
    explicit MaterialExternalLibrary();
    explicit MaterialExternalLibrary(const std::string &libraryName, const QDir &dir, const std::string &icon, bool readOnly = true);
    virtual ~MaterialExternalLibrary();
};

} // namespace Materials

#endif // MATERIAL_MATERIALLIBRARY_H
