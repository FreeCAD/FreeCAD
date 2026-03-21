// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Morten Vajhøj
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/


#ifndef SHAPE_PROPERTIES_H
#define SHAPE_PROPERTIES_H

#include <vector>
#include <string>

#include <Mod/Measure/MeasureGlobal.h>

#include <Base/Placement.h>
#include <Base/Quantity.h>
#include <Base/Vector3D.h>

#include <TopoDS_Shape.hxx>


class TopoDS_Shape;
namespace App
{
class DocumentObject;
}

struct MeasureExport MassPropertiesData
{
    Base::Quantity volume = Base::Quantity(0.0, Base::Unit::Volume);
    Base::Quantity mass = Base::Quantity(0.0, Base::Unit::Mass);
    Base::Quantity density = Base::Quantity(0.0, Base::Unit::Density);
    Base::Quantity surfaceArea = Base::Quantity(0.0, Base::Unit::Area);

    Base::Vector3d cog = Base::Vector3d(0.0, 0.0, 0.0);

    Base::Vector3d cov = Base::Vector3d(0.0, 0.0, 0.0);

    Base::Vector3d inertiaJo = Base::Vector3d(0.0, 0.0, 0.0);

    Base::Vector3d inertiaJCross = Base::Vector3d(0.0, 0.0, 0.0);
    Base::Vector3d inertiaJ = Base::Vector3d(0.0, 0.0, 0.0);

    Base::Vector3d principalAxisX = Base::Vector3d(0.0, 0.0, 0.0);
    Base::Vector3d principalAxisY = Base::Vector3d(0.0, 0.0, 0.0);
    Base::Vector3d principalAxisZ = Base::Vector3d(0.0, 0.0, 0.0);

    double axisInertia;
};

struct MeasureExport MassPropertiesInput
{
    App::DocumentObject* object = nullptr;
    TopoDS_Shape shape;
    Base::Placement placement;
};

enum class MassPropertiesMode
{
    CenterOfGravity,
    Custom
};

MeasureExport MassPropertiesData CalculateMassProperties(
    const std::vector<MassPropertiesInput>& objects,
    MassPropertiesMode mode,
    App::DocumentObject const* referenceDatum,
    const Base::Placement* referencePlacement = nullptr
);

#endif  // SHAPE_PROPERTIES_H
