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

#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

namespace Material {

class MaterialExport LibraryData
{
public:
    explicit LibraryData(const std::string &libraryName, const fs::path &dir, const std::string &icon);
    virtual ~LibraryData();

    const std::string &getName() const
    {
        return name;
    }
    const fs::path &getDirectory() const
    {
        return directory;
    }
    const std::string &getIconPath() const
    {
        return iconPath;
    }

private:
    explicit LibraryData();
    std::string name;
    fs::path directory;
    std::string iconPath;
};

class MaterialExport Materials
{

public:
    explicit Materials();
    virtual ~Materials();

    static std::list<LibraryData *> *getMaterialLibraries();
    static bool isCard(const fs::path &p);
};

} // namespace Material

#endif // MATERIAL_MATERIALS_H
