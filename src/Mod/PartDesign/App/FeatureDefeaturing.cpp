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
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/


#include <Base/Exception.h>
#include <Mod/Part/App/TopoShape.h>

#include "FeatureDefeaturing.h"


using namespace PartDesign;


PROPERTY_SOURCE(PartDesign::Defeaturing, PartDesign::DressUp)

Defeaturing::Defeaturing() = default;

short Defeaturing::mustExecute() const
{
    return DressUp::mustExecute();
}

App::DocumentObjectExecReturn* Defeaturing::execute()
{
    if (onlyHaveRefined()) {
        return App::DocumentObject::StdReturn;
    }

    Part::TopoShape baseShape;
    try {
        baseShape = getBaseTopoShape();
    }
    catch (Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }
    baseShape.setTransform(Base::Matrix4D());

    auto faces = getFaces(baseShape);
    if (faces.empty()) {
        this->positionByBaseFeature();
        this->Shape.setValue(getSolid(baseShape));
        return App::DocumentObject::StdReturn;
    }

    std::vector<TopoDS_Shape> faceShapes;
    faceShapes.reserve(faces.size());
    for (const auto& f : faces) {
        faceShapes.push_back(f.getShape());
    }

    this->positionByBaseFeature();

    try {
        TopoDS_Shape result = baseShape.defeaturing(faceShapes);
        if (result.IsNull()) {
            return new App::DocumentObjectExecReturn(
                QT_TRANSLATE_NOOP("Exception", "Defeaturing failed: result is null")
            );
        }

        Part::TopoShape res(result);
        res = refineShapeIfActive(res);

        if (!isSingleSolidRuleSatisfied(res.getShape())) {
            return new App::DocumentObjectExecReturn(
                QT_TRANSLATE_NOOP("Exception", "Defeaturing did not produce a single solid")
            );
        }

        this->Shape.setValue(getSolid(res));
    }
    catch (Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }

    return App::DocumentObject::StdReturn;
}
