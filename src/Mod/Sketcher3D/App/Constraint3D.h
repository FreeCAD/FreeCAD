// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2026 Yash Suthar <yashsuthar983@gmail.com>              *
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

#include "Mod/Sketcher3D/App/GeoEnum3D.h"

#include <Mod/Sketcher3D/App/PreCompiled.h>

#include <Base/FileInfo.h>
#include <Base/Reader.h>
#include <Base/Tools.h>
#include <Base/Writer.h>

#include <Mod/Sketcher3D/Sketcher3DGlobal.h>

#include <Base/Persistence.h>

namespace Base
{
class Writer;
class XMLReader;
}  // namespace Base

namespace Sketcher3D
{

/// Constraint data class for Sketcher3D
class Sketcher3DExport Constraint3D: public Base::Persistence
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    enum Constraint3DType : int
    {
        Distance3D = 0,
        Coincident3D,
        Parallel3D,
        AlongX,
        AlongY,
        AlongZ,
        DistanceX3D,
        DistanceY3D,
        DistanceZ3D,
        Angle3D,
        EqualLength3D,
        NumConstraintTypes
    };

    Constraint3DType Type = Constraint3DType::Distance3D;

    double Value = 0.0;

    bool isDriving = true;

    /// Return the string name for a constraint type.
    static const char* typeToString(Constraint3DType t);

    // Persistence
    unsigned int getMemSize() const override;
    void Save(Base::Writer& writer) const override;
    void Restore(Base::XMLReader& reader) override;

    const std::vector<GeoElementId3D>& getElements() const
    {
        return elements;
    }

    void setElements(std::vector<GeoElementId3D> newElements)
    {
        elements = std::move(newElements);
    }


private:
    constexpr static std::array<const char*, Constraint3DType::NumConstraintTypes> type2str {{
        "Distance3D",
        "Coincident3D",
        "Parallel3D",
        "AlongX",
        "AlongY",
        "AlongZ",
        "DistanceX3D",
        "DistanceY3D",
        "DistanceZ3D",
        "Angle3D",
        "EqualLength3D",
    }};

    std::vector<GeoElementId3D> elements;
};

}  // namespace Sketcher3D
