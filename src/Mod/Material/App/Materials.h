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

#ifndef MATERIAL_MATERIALS_H
#define MATERIAL_MATERIALS_H

#include <Base/BaseClass.h>
#include <QDir>
#include <QString>

namespace fs = boost::filesystem;

namespace Materials {

class MaterialsExport MaterialLibrary : public Base::BaseClass
{
    TYPESYSTEM_HEADER();

public:
    explicit MaterialLibrary();
    explicit MaterialLibrary(const std::string &libraryName, const QDir &dir, const std::string &icon);
    virtual ~MaterialLibrary();

    const std::string &getName() const { return _name; }
    const QDir &getDirectory() const { return _directory; }
    const std::string getDirectoryPath() const { return _directory.absolutePath().toStdString(); }
    const std::string &getIconPath() const { return _iconPath; }

private:
    std::string _name;
    QDir _directory;
    std::string _iconPath;
};

class MaterialsExport Material : public Base::BaseClass
{
    TYPESYSTEM_HEADER();

public:
    explicit Material();
    virtual ~Material();

private:
    MaterialLibrary _library;
    QDir _directory;
    std::string _uuid;
    std::string _name;
    std::string _authorAndLicense;
    std::string _parentUuid;
    std::string _description;
    std::string _url;
    std::string _reference;
    std::list<std::string> _tags;
    std::vector<std::string> _inheritedUuids;
    // std::map<std::string, ModelProperty> _properties; - TODO: Data types yet to be defined
    // std::map<std::string, ModelProperty> _appearanceProperties;

};

} // namespace Materials

#endif // MATERIAL_MATERIALS_H
