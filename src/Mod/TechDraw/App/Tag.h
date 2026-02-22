/***************************************************************************
 *   Copyright (c) 2025 WandererFan <wandererfan@gmail.com>                *
 *   Copyright (c) 2025 Benjamin Bræstrup Sayoc <benj5378@outlook.com>     *
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

#pragma once

#include <string>
#include <string_view>

#include <boost/uuid/uuid.hpp>

#include <Mod/TechDraw/TechDrawGlobal.h>

namespace Base {
class XMLReader;
class Writer;
}

namespace TechDraw {


//NOLINTNEXTLINE    (default destructor but no copy)
class TechDrawExport Tag {
public:
    virtual ~Tag() = default;
    //Uniqueness
    boost::uuids::uuid getTag() const;
    virtual std::string getTagAsString() const;

    static boost::uuids::uuid fromString(const std::string& tagString);


protected:
    Tag();
    void setTag(const boost::uuids::uuid& newTag);
    void Save(Base::Writer& writer) const;
    // Setting elementName is only for backwards compatibility!
    void Restore(Base::XMLReader& reader, std::string_view elementName="Tag");

private:
    void createNewTag();
    boost::uuids::uuid tag{};
};
}