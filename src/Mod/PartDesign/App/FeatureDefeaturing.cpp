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


#include <BRepAlgoAPI_Cut.hxx>
#include <BRepAlgoAPI_Defeaturing.hxx>
#include <BRepAlgoAPI_Fuse.hxx>
#include <TopTools_ListOfShape.hxx>

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

    this->positionByBaseFeature();

    try {
        BRepAlgoAPI_Defeaturing defeat;
        defeat.SetRunParallel(true);
        defeat.SetShape(baseShape.getShape());
        for (const auto& f : faces) {
            defeat.AddFaceToRemove(f.getShape());
        }
        defeat.Build();
        if (!defeat.IsDone()) {
            Standard_SStream ss;
            defeat.DumpErrors(ss);
            throw Base::RuntimeError(ss.str().c_str());
        }

        TopoDS_Shape result = defeat.Shape();
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

void Defeaturing::updatePreviewShape()
{
    Part::TopoShape resultShape = Shape.getShape();
    if (resultShape.isNull()) {
        PreviewShape.setValue(TopoDS_Shape());
        return;
    }

    Part::TopoShape baseShape;
    try {
        baseShape = getBaseTopoShape();
    }
    catch (...) {
        PreviewShape.setValue(TopoDS_Shape());
        return;
    }

    baseShape.setTransform(Base::Matrix4D());
    resultShape.setTransform(Base::Matrix4D());

    try {
        // Removed volume: material in base but not in result (e.g. defeature a fillet)
        BRepAlgoAPI_Cut removedCut;
        removedCut.SetRunParallel(true);
        TopTools_ListOfShape args1, tools1;
        args1.Append(baseShape.getShape());
        tools1.Append(resultShape.getShape());
        removedCut.SetArguments(args1);
        removedCut.SetTools(tools1);
        removedCut.Build();

        // Added volume: material in result but not in base (e.g. defeature a hole)
        BRepAlgoAPI_Cut addedCut;
        addedCut.SetRunParallel(true);
        TopTools_ListOfShape args2, tools2;
        args2.Append(resultShape.getShape());
        tools2.Append(baseShape.getShape());
        addedCut.SetArguments(args2);
        addedCut.SetTools(tools2);
        addedCut.Build();

        bool hasRemoved = removedCut.IsDone() && !removedCut.Shape().IsNull();
        bool hasAdded = addedCut.IsDone() && !addedCut.Shape().IsNull();

        if (!hasRemoved && !hasAdded) {
            PreviewShape.setValue(TopoDS_Shape());
            return;
        }

        TopoDS_Shape previewShape;
        if (hasRemoved && hasAdded) {
            BRepAlgoAPI_Fuse fuse;
            fuse.SetRunParallel(true);
            TopTools_ListOfShape fuseArgs, fuseTools;
            fuseArgs.Append(removedCut.Shape());
            fuseTools.Append(addedCut.Shape());
            fuse.SetArguments(fuseArgs);
            fuse.SetTools(fuseTools);
            fuse.Build();
            if (!fuse.IsDone() || fuse.Shape().IsNull()) {
                previewShape = removedCut.Shape();
            }
            else {
                previewShape = fuse.Shape();
            }
        }
        else if (hasRemoved) {
            previewShape = removedCut.Shape();
        }
        else {
            previewShape = addedCut.Shape();
        }

        PreviewShape.setValue(Part::TopoShape(previewShape));
    }
    catch (...) {
        PreviewShape.setValue(TopoDS_Shape());
    }
}
