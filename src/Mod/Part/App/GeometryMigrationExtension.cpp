/***************************************************************************
 *   Copyright (c) 2020 Abdullah Tahiri <abdullah.tahiri.yo@gmail.com>     *
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

#include "PreCompiled.h"

#include <Base/Exception.h>

#include "GeometryMigrationExtension.h"


using namespace Part;

//---------- Geometry Extension
TYPESYSTEM_SOURCE(Part::GeometryMigrationExtension,Part::GeometryExtension)

void GeometryMigrationExtension::copyAttributes(Part::GeometryExtension * cpy) const
{
    Part::GeometryExtension::copyAttributes(cpy);
    static_cast<GeometryMigrationExtension *>(cpy)->ConstructionState = this->ConstructionState;
    static_cast<GeometryMigrationExtension *>(cpy)->GeometryMigrationFlags  = this->GeometryMigrationFlags;
}

std::unique_ptr<Part::GeometryExtension> GeometryMigrationExtension::copy() const
{
    auto cpy = std::make_unique<GeometryMigrationExtension>();

    copyAttributes(cpy.get());

#if defined (__GNUC__) && (__GNUC__ <=4)
    return std::move(cpy);
#else
    return cpy;
#endif
}

PyObject * GeometryMigrationExtension::getPyObject()
{
    THROWM(Base::NotImplementedError, "GeometryMigrationExtension does not have a Python counterpart");
}
