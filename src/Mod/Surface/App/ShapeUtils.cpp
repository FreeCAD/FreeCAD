// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Max Wilfinger
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
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Lesser General Public License for more details.                      *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/

#include "ShapeUtils.h"

#include <App/GeoFeature.h>
#include <Mod/Part/App/PartFeature.h>

namespace Surface
{

Part::TopoShape getTopoShapeInFeatureCoordinates(
    const App::DocumentObject* obj,
    const App::GeoFeature* feature
)
{
    Part::TopoShape shape = Part::Feature::getTopoShape(
        obj,
        Part::ShapeOption::ResolveLink | Part::ShapeOption::Transform
    );
    if (shape.isNull() || !feature) {
        return shape;
    }

    const auto geo = dynamic_cast<const App::GeoFeature*>(obj);
    if (geo) {
        const Base::Placement sourcePlacement = geo->globalPlacement()
            * geo->Placement.getValue().inverse();
        const Base::Placement placement = feature->globalPlacement().inverse() * sourcePlacement;
        shape.transformShape(placement.toMatrix(), false, true);
    }
    return shape;
}

}  // namespace Surface
