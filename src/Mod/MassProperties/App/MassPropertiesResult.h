// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   © 2026 Morten Vajhøj                                                     *
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

#include <Base/Placement.h>
#include <Base/Vector3D.h>

#include <TopoDS_Shape.hxx>


class TopoDS_Shape;
namespace App { class DocumentObject; }

struct MassPropertiesData {
    double volume;
    double mass;
    double density;
    double surfaceArea;

    double cogX;
    double cogY;
    double cogZ;

    double covX;
    double covY;
    double covZ;

    double inertiaJox;
    double inertiaJoy;
    double inertiaJoz;
    
    double inertiaJxy;
    double inertiaJzx;
    double inertiaJzy;
    double inertiaJx;
    double inertiaJy;
    double inertiaJz;

    Base::Vector3d principalAxisX = Base::Vector3d(0.0, 0.0, 0.0);
    Base::Vector3d principalAxisY = Base::Vector3d(0.0, 0.0, 0.0);
    Base::Vector3d principalAxisZ = Base::Vector3d(0.0, 0.0, 0.0);

    double axisInertia;
};

struct MassPropertiesInput {
    App::DocumentObject* object = nullptr;
    TopoDS_Shape shape;
    Base::Placement placement;
};

MassPropertiesData CalculateMassProperties(
    const std::vector<MassPropertiesInput>& objects,
    std::string& mode,
    App::DocumentObject const* referenceDatum,
    const Base::Placement* referencePlacement = nullptr
);

#endif // SHAPE_PROPERTIES_H