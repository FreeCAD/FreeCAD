// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2026 FreeCAD contributors                              *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 ***************************************************************************/

#include <cmath>

#include <BRepOffset_Mode.hxx>

#include <Base/Exception.h>

#include "RectoVersoThickness.h"


namespace PartDesign
{

namespace
{
void ensureValidWall(const Part::TopoShape& wall, const char* message)
{
    if (wall.isNull() || !wall.isValid() || wall.countSubShapes(TopAbs_SOLID) != 1) {
        throw Base::CADKernelError(message);
    }
}
}  // namespace

Part::TopoShape makeRectoVersoThickness(
    const Part::TopoShape& solid,
    const std::vector<Part::TopoShape>& closingFaces,
    double offset,
    double tolerance,
    bool intersection,
    Part::JoinType join
)
{
    const double distance = std::abs(offset);
    if (distance <= tolerance) {
        throw Base::CADKernelError("Recto-verso thickness must exceed the modeling tolerance");
    }

    // Signed offsets are only meaningful for consistently oriented solids.
    // Imported and programmatically constructed solids are not guaranteed to
    // have that orientation, so normalize it without resetting element names.
    Part::TopoShape orientedSolid = solid;
    orientedSolid.fixSolidOrientation();

    constexpr auto skinMode = static_cast<short>(BRepOffset_Skin);
    Part::TopoShape recto = orientedSolid.makeElementThickSolid(
        closingFaces,
        distance,
        tolerance,
        intersection,
        false,
        skinMode,
        join,
        "RectoVersoRecto"
    );
    Part::TopoShape verso = orientedSolid.makeElementThickSolid(
        closingFaces,
        -distance,
        tolerance,
        intersection,
        false,
        skinMode,
        join,
        "RectoVersoVerso"
    );
    ensureValidWall(recto, "Recto-verso positive-side wall is invalid");
    ensureValidWall(verso, "Recto-verso negative-side wall is invalid");

    Part::TopoShape result(0);
    result.makeElementFuse({recto, verso}, "RectoVerso", tolerance);
    if (result.isNull() || !result.isValid() || result.countSubShapes(TopAbs_SOLID) != 1) {
        throw Base::CADKernelError("Recto-verso thickness produced an invalid solid");
    }
    return result;
}

}  // namespace PartDesign
