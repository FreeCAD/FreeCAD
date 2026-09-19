// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2015 Stefan Tröger <stefantroeger@gmx.net>              *
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

#include <cmath>
#include <deque>
#include <map>
#include <string>
#include <vector>

#include <BRepOffset.hxx>
#include <BRepOffsetAPI_MakeThickSolid.hxx>
#include <GeomAbs_JoinType.hxx>
#include <Precision.hxx>
#include <Standard_Failure.hxx>
#include <TopTools_ListOfShape.hxx>
#include <TopoDS.hxx>


#include <App/Document.h>
#include <App/PropertyLinks.h>
#include <App/SemanticDocumentState.h>
#include <Base/Exception.h>
#include "FeatureThickness.h"
#include <Mod/Part/App/SemanticHistoryAdapter.h>
#include <Mod/Part/App/TopoShape.h>
#include <Mod/Part/App/TopoShapeOpCode.h>

#include "SemanticOpcode.h"

FC_LOG_LEVEL_INIT("PartDesign", true, true)

using namespace PartDesign;

const char* PartDesign::Thickness::ModeEnums[] = {"Skin", "Pipe", "RectoVerso", nullptr};
const char* PartDesign::Thickness::JoinEnums[] = {"Arc", "Intersection", nullptr};

PROPERTY_SOURCE(PartDesign::Thickness, PartDesign::DressUp)

Thickness::Thickness()
{
    ADD_PROPERTY_TYPE(Value, (1.0), "Thickness", App::Prop_None, "Thickness value");
    ADD_PROPERTY_TYPE(Mode, (0L), "Thickness", App::Prop_None, "Mode");
    Mode.setEnums(ModeEnums);
    ADD_PROPERTY_TYPE(Join, (0L), "Thickness", App::Prop_None, "Join type");
    Join.setEnums(JoinEnums);
    ADD_PROPERTY_TYPE(
        Reversed,
        (true),
        "Thickness",
        App::Prop_None,
        "Apply the thickness towards the solids interior"
    );
    ADD_PROPERTY_TYPE(Intersection, (false), "Thickness", App::Prop_None, "Enable intersection-handling");
}

int16_t Thickness::mustExecute() const
{
    if (Placement.isTouched() || Value.isTouched() || Mode.isTouched() || Join.isTouched()) {
        return 1;
    }
    return DressUp::mustExecute();
}

App::DocumentObjectExecReturn* Thickness::execute()
{
    if (onlyHaveRefined()) {
        return App::DocumentObject::StdReturn;
    }

    // Base shape
    Part::TopoShape TopShape;
    try {
        TopShape = getBaseTopoShape();
    }
    catch (Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }

    App::SemanticGraph* graph = SemanticEmitter::graphFor(this);
    App::ObjectId fid = static_cast<App::ObjectId>(getID());
    App::EvalSerial eval = 0;
    AfterExecuteRequest req;
    std::vector<App::SemanticReference> namedFaceReferences;
    if (App::Document* doc = getDocument()) {
        eval = doc->semanticState().currentEval();
    }
    // Thickness.Base is faces to open, not edges.
    for (const App::SemanticReference& ref : Base.getSemanticRefs()) {
        if (!ref.seed.valid()) {
            continue;
        }
        if ((ref.seed.kind == App::SemanticKind::Face || ref.kind == App::SemanticKind::Face)
            && appendUniqueSemanticSeed(req.thicknessFaces, ref.seed)) {
            namedFaceReferences.push_back(ref);
        }
    }
    // R2: resolve ThicknessFace before the maker. Missing / Incompatible
    // skips the maker. Do not pick a neighbour (I10).
    // Empty seeds → do not skip (I7 FaceN fallback).
    const Part::FilletPreflight pre = Part::SemanticHistoryAdapter::preflightNamedReferences(
        graph, namedFaceReferences, App::SemanticKind::Face
    );
    if (pre.makerSkipped) {
        SemanticEmitter::afterExecute(graph, Opcode::Thickness, fid, eval, req);
        return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP(
            "Exception",
            "Thickness face Missing; maker skipped. No neighbour substitution."
        ));
    }

    const std::vector<std::string> subStrings = getFaceSubValues(graph);

    // If the base has no sub elements listed just return a copy of the base.
    if (subStrings.empty()) {
        // We must set the placement of the feature in case it's empty.
        this->positionByBaseFeature();
        this->Shape.setValue(TopShape);
        return App::DocumentObject::StdReturn;
    }

    /* If the feature was ever empty, then Placement was set by positionByBaseFeature.  However,
     * makeThickSolid apparently requires the placement to be empty, so we have to clear it */
    this->Placement.setValue(Base::Placement());

    std::map<int, std::vector<Part::TopoShape>> closeFaces;
    for (const auto& it : subStrings) {
        TopoDS_Shape face;
        try {
            face = TopShape.getSubShape(it.c_str());
        }
        catch (...) {
        }
        if (face.IsNull()) {
            return new App::DocumentObjectExecReturn(
                QT_TRANSLATE_NOOP("Exception", "Invalid face reference")
            );
        }
        // We found the sub element (face) so let's get its history index in our shape
        int index = TopShape.findAncestor(face, TopAbs_SOLID);
        if (!index) {
            FC_WARN(getFullName() << ": Ignore non-solid face  " << it);
            continue;
        }
        closeFaces[index].emplace_back(face);
    }

    bool reversed = Reversed.getValue();
    bool intersection = Intersection.getValue();
    double thickness = (reversed ? -1. : 1.) * Value.getValue();
    double tol = Precision::Confusion();
    auto mode = static_cast<int16_t>(Mode.getValue());
    auto join = Join.getValue();

    std::vector<Part::TopoShape> shapes;
    auto count = static_cast<int>(TopShape.countSubShapes(TopAbs_SOLID));
    if (!count) {
        return new App::DocumentObjectExecReturn("No solid");
    }
    // we do not offer tangent join type
    if (join == 1) {
        join = 2;
    }

    // Keep the maker alive for fromMaker (makeElementThickSolid discards it).
    BRepOffsetAPI_MakeThickSolid mkThick;
    bool makerDone = false;
    try {
        if (fabs(thickness) > 2 * tol) {
            auto mapIterator = closeFaces.begin();
            for (auto loopIndex = 1; loopIndex <= count; ++loopIndex) {
                std::vector<Part::TopoShape> dummy;
                const auto* faces = &dummy;
                Part::TopoShape solid = TopShape;
                // expect the sub element indexes in the map to be in order and matching our loop index,
                // and effectively ignore them if they are not.
                if (mapIterator != closeFaces.end() && loopIndex >= mapIterator->first) {
                    faces = &mapIterator->second;
                    solid = TopShape.getSubTopoShape(TopAbs_SOLID, mapIterator->first);
                }
                if (faces->empty()) {
                    if (mapIterator != closeFaces.end()) {
                        ++mapIterator;
                    }
                    continue;
                }
                TopTools_ListOfShape remFace;
                for (const auto& face : *faces) {
                    if (face.isNull()) {
                        return new App::DocumentObjectExecReturn(
                            QT_TRANSLATE_NOOP("Exception", "Invalid face reference")
                        );
                    }
                    remFace.Append(face.getShape());
                }
                mkThick.MakeThickSolidByJoin(
                    solid.getShape(),
                    remFace,
                    thickness,
                    tol,
                    BRepOffset_Mode(mode),
                    intersection ? Standard_True : Standard_False,
                    Standard_False,
                    GeomAbs_JoinType(join)
                );
                if (!mkThick.IsDone()) {
                    SemanticEmitter::afterExecute(graph, Opcode::Thickness, fid, eval, req);
                    return new App::DocumentObjectExecReturn("Failed to make thick solid");
                }
                Part::TopoShape res(0);
                res.makeElementShape(mkThick, solid, Part::OpCodes::Thicken);
                shapes.push_back(res);
                makerDone = true;
                if (mapIterator != closeFaces.end()) {
                    ++mapIterator;
                }
            }
        }

        Part::TopoShape result(0);
        if (shapes.size() > 1) {
            result.makeElementFuse(shapes);
        }
        else if (shapes.empty()) {
            result = TopShape;
        }
        else {
            result = shapes.front();
        }
        // store shape before refinement
        this->rawShape = result;
        result = refineShapeIfActive(result);
        result = getSolid(result);
        this->Shape.setValue(result);

        // Bind Generated thickness faces from the maker history. Empty / unnamed
        // -> leave Binding empty (I10). Never sequential FaceN.
        // Multi-solid fuse: skip fromMaker (images would not match the fuse).
        // Does not resetElementMap. Does not replace Shape.setValue.
        if (makerDone && shapes.size() == 1) {
            Part::TopoShape shape = result;
            std::deque<TopoDS_Shape> held;
            std::vector<std::pair<App::SemanticId, const void*>> inputs;
            if (graph && Base.getValue()) {
                const App::ObjectId baseFeature =
                    static_cast<App::ObjectId>(Base.getValue()->semanticProjectionFeatureId());
                for (const App::SemanticId& face : req.thicknessFaces) {
                    const auto unique = uniqueFaceBindingOnFeature(graph, face, baseFeature);
                    if (!unique) {
                        continue;
                    }
                    TopoDS_Shape fs = TopShape.findShape(TopAbs_FACE, unique->index.index);
                    if (fs.IsNull()) {
                        continue;
                    }
                    held.push_back(fs);
                    inputs.push_back({face, &held.back()});
                }
            }
            auto indexOf = [&shape](const void* occ) -> App::ElementIndex {
                App::ElementIndex idx;
                if (!occ) {
                    return idx;
                }
                const auto& sub = *static_cast<const TopoDS_Shape*>(occ);
                if (sub.IsNull() || sub.ShapeType() != TopAbs_FACE) {
                    return idx;
                }
                const int n = shape.findShape(sub);
                if (n <= 0) {
                    return idx;
                }
                idx.type = "Face";
                idx.index = n;
                return idx;
            };
            // fromMaker only: empty table -> applyHistory half-map, no sequential FaceN (I13).
            Part::HistoryTable hist = Part::SemanticHistoryAdapter::fromMaker(&mkThick, inputs, indexOf);
            Part::HistoryTable toApply;
            toApply.reserve(hist.size());
            for (const auto& rec : hist) {
                if (App::shouldRefuseDressUpMintKind(rec.kind, graph, fid, rec.toIndex)) {
                    continue;
                }
                toApply.push_back(rec);
                if (Part::isNamedIndex(rec.toIndex)) {
                    req.namedFaceIndices.push_back(rec.toIndex);
                }
            }
            const Part::ApplyResult applied = Part::SemanticHistoryAdapter::applyHistory(
                graph, fid, eval, "Thickness", req.thicknessFaces, toApply);
            if (applied.boundCount == 0) {
                SemanticEmitter::afterExecute(graph, Opcode::Thickness, fid, eval, req);
            }
        }
        else {
            SemanticEmitter::afterExecute(graph, Opcode::Thickness, fid, eval, req);
        }
        return App::DocumentObject::StdReturn;
    }
    catch (Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }
    catch (Standard_Failure& e) {
        FC_ERR("Exception on making thick solid: " << e.GetMessageString());
        return new App::DocumentObjectExecReturn("Failed to make thick solid");
    }
}
