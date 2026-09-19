// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2010 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
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

#include <limits>

#include <deque>
#include <BRepAlgo.hxx>
#include <BRepFilletAPI_MakeChamfer.hxx>
#include <BRep_Tool.hxx>
#include <TopExp.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopTools_IndexedDataMapOfShapeListOfShape.hxx>
#include <TopTools_ListOfShape.hxx>
#include <ShapeFix_Shape.hxx>
#include <ShapeFix_ShapeTolerance.hxx>
#include <Standard_Version.hxx>


#include <App/Document.h>
#include <App/PropertyLinks.h>
#include <App/SemanticDocumentState.h>
#include <Base/Exception.h>
#include <Base/Reader.h>
#include <Base/Tools.h>
#include <Mod/Part/App/SignalException.h>
#include <Mod/Part/App/TopoShape.h>
#include <Mod/Part/App/TopoShapeOpCode.h>
#include <Mod/Part/App/SemanticHistoryAdapter.h>

#include "FeatureChamfer.h"
#include "SemanticOpcode.h"

#include <Base/ProgramVersion.h>


using namespace PartDesign;


PROPERTY_SOURCE(PartDesign::Chamfer, PartDesign::DressUp)

const char* ChamferTypeEnums[] = {"Equal distance", "Two distances", "Distance and Angle", nullptr};
const App::PropertyQuantityConstraint::Constraints Chamfer::floatSize
    = {0.0, std::numeric_limits<float>::max(), 0.1};
const App::PropertyAngle::Constraints Chamfer::floatAngle = {0.0, 180.0, 1.0};

static App::DocumentObjectExecReturn* validateParameters(
    int chamferType,
    double size,
    double size2,
    double angle
);

Chamfer::Chamfer()
{
    ADD_PROPERTY_TYPE(ChamferType, (0L), "Chamfer", App::Prop_None, "Type of chamfer");
    ChamferType.setEnums(ChamferTypeEnums);

    ADD_PROPERTY_TYPE(Size, (1.0), "Chamfer", App::Prop_None, "Size of chamfer");
    Size.setUnit(Base::Unit::Length);
    Size.setConstraints(&floatSize);

    ADD_PROPERTY_TYPE(Size2, (1.0), "Chamfer", App::Prop_None, "Second size of chamfer");
    Size2.setUnit(Base::Unit::Length);
    Size2.setConstraints(&floatSize);

    ADD_PROPERTY_TYPE(Angle, (45.0), "Chamfer", App::Prop_None, "Angle of chamfer");
    Angle.setUnit(Base::Unit::Angle);
    Angle.setConstraints(&floatAngle);

    ADD_PROPERTY_TYPE(FlipDirection, (false), "Chamfer", App::Prop_None, "Flip direction");
    ADD_PROPERTY_TYPE(
        UseAllEdges,
        (false),
        "Chamfer",
        App::Prop_None,
        "Chamfer all edges if true, else use only those edges in Base property.\n"
        "If true, then this overrides any edge changes made to the Base property or in the "
        "dialog.\n"
    );

    updateProperties();
}

short Chamfer::mustExecute() const
{
    bool touched = false;

    auto chamferType = ChamferType.getValue();

    switch (chamferType) {
        case 0:  // "Equal distance"
            touched = Size.isTouched() || ChamferType.isTouched();
            break;
        case 1:  // "Two distances"
            touched = Size.isTouched() || ChamferType.isTouched() || Size2.isTouched();
            break;
        case 2:  // "Distance and Angle"
            touched = Size.isTouched() || ChamferType.isTouched() || Angle.isTouched();
            break;
    }

    if (Placement.isTouched() || touched) {
        return 1;
    }

    // See Fillet::mustExecute(): restore only schedules a semantic publisher
    // whose valid cached shape has no durable STG1 rows of its own.
    if (isSemanticRepublishPass(SemanticEmitter::graphFor(this))) {
        return 1;
    }
    return DressUp::mustExecute();
}

App::DocumentObjectExecReturn* Chamfer::execute()
{
    // See Fillet::execute(): restore can mark Refine touched even though the
    // valid cached result is precisely the shape that needs semantic history
    // republished.  Do not take the refine-only early return in that case.
    App::SemanticGraph* graph = SemanticEmitter::graphFor(this);
    const App::ObjectId fid = static_cast<App::ObjectId>(getID());
    const bool semanticRepublish = isSemanticRepublishPass(graph);
    if (!semanticRepublish && onlyHaveRefined()) {
        return App::DocumentObject::StdReturn;
    }

    // NOTE: Normally the Base property and the BaseFeature property should point to the same object.
    // The only difference is that the Base property also stores the edges that are to be chamfered
    Part::TopoShape TopShape;
    try {
        TopShape = getBaseTopoShape();
    }
    catch (Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }

    TopShape.setTransform(Base::Matrix4D());

    App::EvalSerial eval = 0;
    AfterExecuteRequest req;
    if (App::Document* doc = getDocument()) {
        eval = doc->semanticState().currentEval();
    }
    // Chamfer reuses filletEdges / filletAdjacentFaces (dress-up seeds).
    collectDressUpBaseSeeds(req);
    // R2: resolve ChamferEdge before the maker. Missing / Incompatible
    // skips the maker. Do not pick a similar-length neighbour (I10).
    const Part::FilletPreflight pre =
        Part::SemanticHistoryAdapter::preflightFillet(graph, req.filletEdges);
    // A restored semantic seed has no live Binding until its producer has
    // executed in this evaluation.  For the narrow valid-cached-shape
    // republish pass, rebuild from the cached Base subname rather than turning
    // that transient Missing resolution into an execution error.
    if (pre.makerSkipped) {
        if (Part::SemanticHistoryAdapter::canUseCachedGeometry(pre, semanticRepublish)) {
            req.filletEdges.clear();
        }
        else {
            SemanticEmitter::afterExecute(graph, Opcode::Chamfer, fid, eval, req);
            return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP(
                "Exception",
                "Chamfer edge Missing; maker skipped. No similar-length neighbour substitution."
            ));
        }
    }

    if (graph && graph->hasBindings() && Base.getValue()) {
        const App::ObjectId baseFeature =
            static_cast<App::ObjectId>(Base.getValue()->semanticProjectionFeatureId());
        retainResolvedDressUpSeeds(
            graph, baseFeature, App::SemanticKind::Edge, req.filletEdges);
        retainResolvedDressUpSeeds(
            graph, baseFeature, App::SemanticKind::Face, req.filletAdjacentFaces);
    }

    // Candidate A/B (manual-3-dressup-relink-a): see Fillet::execute.
    std::vector<TopoShape> edges;
    try {
        edges = UseAllEdges.getValue() ? TopShape.getSubTopoShapes(TopAbs_EDGE)
                                       : getContinuousEdges(TopShape, graph);
    }
    catch (Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }

    if (edges.empty()) {
        return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "No edges specified"));
    }
    const int chamferType = ChamferType.getValue();
    const double size = Size.getValue();
    double size2 = Size2.getValue();
    const double angle = Angle.getValue();
    const bool flipDirection = FlipDirection.getValue();

    auto res = validateParameters(chamferType, size, size2, angle);
    if (res != App::DocumentObject::StdReturn) {
        return res;
    }

    this->positionByBaseFeature();

    if (static_cast<Part::ChamferType>(chamferType) == Part::ChamferType::distanceAngle) {
        size2 = angle;
    }
    try {
        TopoShape shape(0);
        Part::SignalException sig;
        // Keep the maker alive for fromMaker (makeElementChamfer discards it).
        BRepFilletAPI_MakeChamfer mkChamfer(TopShape.getShape());
        const auto type = static_cast<Part::ChamferType>(chamferType);
        const Part::Flip flip = flipDirection ? Part::Flip::flip : Part::Flip::none;
        for (auto& e : edges) {
            const auto& edge = e.getShape();
            if (e.isNull()) {
                continue;
            }
            if (!TopShape.findShape(edge)) {
                return new App::DocumentObjectExecReturn(
                    QT_TRANSLATE_NOOP("Exception", "Chamfer edge does not belong to the shape")
                );
            }
            if (BRep_Tool::Degenerated(TopoDS::Edge(edge))) {
                return new App::DocumentObjectExecReturn(
                    QT_TRANSLATE_NOOP("Exception", "Chamfer edge is degenerated")
                );
            }
            TopoDS_Shape face;
            if (flip == Part::Flip::flip) {
                const auto faces = TopShape.findAncestorsShapes(edge, TopAbs_FACE);
                if (faces.empty()) {
                    return new App::DocumentObjectExecReturn(
                        QT_TRANSLATE_NOOP("Exception", "Chamfer edge has no adjacent face")
                    );
                }
                face = faces.back();
            }
            else {
                face = TopShape.findAncestorShape(edge, TopAbs_FACE);
            }
            if (face.IsNull()) {
                return new App::DocumentObjectExecReturn(
                    QT_TRANSLATE_NOOP("Exception", "Chamfer edge has no adjacent face")
                );
            }
            switch (type) {
                case Part::ChamferType::equalDistance:
                    mkChamfer.Add(size, size, TopoDS::Edge(edge), TopoDS::Face(face));
                    break;
                case Part::ChamferType::twoDistances:
                    mkChamfer.Add(size, size2, TopoDS::Edge(edge), TopoDS::Face(face));
                    break;
                case Part::ChamferType::distanceAngle:
                    mkChamfer.AddDA(
                        size, Base::toRadians(size2), TopoDS::Edge(edge), TopoDS::Face(face)
                    );
                    break;
            }
        }
        shape.makeElementShape(mkChamfer, TopShape, Part::OpCodes::Chamfer);
        if (shape.isNull()) {
            return new App::DocumentObjectExecReturn(
                QT_TRANSLATE_NOOP("Exception", "Failed to create chamfer")
            );
        }

        TopTools_ListOfShape aLarg;
        aLarg.Append(TopShape.getShape());
        if (!BRepAlgo::IsValid(aLarg, shape.getShape(), Standard_False, Standard_False)) {
            ShapeFix_ShapeTolerance aSFT;
            aSFT.LimitTolerance(
                shape.getShape(),
                Precision::Confusion(),
                Precision::Confusion(),
                TopAbs_SHAPE
            );
        }

        // store shape before refinement
        this->rawShape = shape;
        shape = refineShapeIfActive(shape);
        if (!isSingleSolidRuleSatisfied(shape.getShape())) {
            return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP(
                "Exception",
                "Result has multiple solids: enable 'Allow Compound' in the active body."
            ));
        }

        shape = getSolid(shape);
        this->Shape.setValue(shape);
        // Bind Generated chamfer faces from the maker history. Empty / unnamed
        // -> leave Binding empty (I10). Never sequential FaceN.
        // Does not resetElementMap. Does not replace Shape.setValue.
        std::deque<TopoDS_Shape> held;
        std::vector<std::pair<App::SemanticId, const void*>> inputs;
        if (graph && Base.getValue()) {
            const App::ObjectId baseFeature =
                static_cast<App::ObjectId>(Base.getValue()->semanticProjectionFeatureId());
            for (const App::SemanticId& edge : req.filletEdges) {
                const auto unique = uniqueResolvedDressUpBinding(
                    graph, edge, baseFeature, App::SemanticKind::Edge);
                if (!unique) {
                    continue;
                }
                TopoDS_Shape es = TopShape.findShape(TopAbs_EDGE, unique->index.index);
                if (es.IsNull()) {
                    continue;
                }
                held.push_back(es);
                inputs.push_back({edge, &held.back()});
            }
        }
        // Inspect selected edges even when their Base LinkSub has no seed.
        for (const TopoShape& selected : edges) {
            const TopoDS_Shape& selectedShape = selected.getShape();
            bool alreadyCaptured = false;
            for (const auto& input : inputs) {
                if (input.second
                    && static_cast<const TopoDS_Shape*>(input.second)->IsSame(selectedShape)) {
                    alreadyCaptured = true;
                    break;
                }
            }
            if (!alreadyCaptured) {
                held.push_back(selectedShape);
                inputs.push_back({App::SemanticId{}, &held.back()});
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
        // fromMaker only: BRepFilletAPI_MakeChamfer has no History() on this OCCT.
        // Empty table -> applyHistory half-map, no sequential FaceN (I13).
        Part::HistoryTable hist = Part::SemanticHistoryAdapter::fromMaker(&mkChamfer, inputs, indexOf);
        for (auto& rec : hist) {
            rec.extraSeeds = req.filletAdjacentFaces;
        }
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
            graph, fid, eval, "Chamfer", req.filletEdges, toApply);
        if (applied.boundCount == 0) {
            SemanticEmitter::afterExecute(graph, Opcode::Chamfer, fid, eval, req);
        }
        return App::DocumentObject::StdReturn;
    }
    catch (Standard_Failure& e) {
        return new App::DocumentObjectExecReturn(e.GetMessageString());
    }
    catch (...) {
        return new App::DocumentObjectExecReturn(
            QT_TRANSLATE_NOOP("Exception", "Chamfer failed: OCC kernel error in chamfer computation")
        );
    }
}


void Chamfer::Restore(Base::XMLReader& reader)
{
    DressUp::Restore(reader);

    migrateFlippedProperties(reader);
}

void Chamfer::handleChangedPropertyType(Base::XMLReader& reader, const char* TypeName, App::Property* prop)
{
    if (prop && strcmp(TypeName, "App::PropertyFloatConstraint") == 0
        && prop->getTypeId().getName() == "App::PropertyQuantityConstraint") {
        App::PropertyFloatConstraint p;
        p.Restore(reader);
        static_cast<App::PropertyQuantityConstraint*>(prop)->setValue(p.getValue());
    }
    else {
        DressUp::handleChangedPropertyType(reader, TypeName, prop);
    }
}

bool Chamfer::requiresSizeSwapping(const Base::XMLReader& reader) const
{
    return Base::getVersion(reader.ProgramVersion) < Base::Version::v1_0
        && (ChamferType.getValue() == 1 || ChamferType.getValue() == 2);
}

void Chamfer::migrateFlippedProperties(const Base::XMLReader& reader)
{
    if (!requiresSizeSwapping(reader)) {
        return;
    }

    Base::Console().warning(
        "The 'FlipDirection' property of the chamfer of %s is being adjusted to maintain"
        "the same geometry in this FreeCAD version. If the re-saved file is later opened "
        "in FreeCAD 0.21.x the chamfer result may differ due to the changed parameter "
        "interpretation.\n",
        getFullName()
    );

    FlipDirection.setValue(!FlipDirection.getValue());
}

void Chamfer::onChanged(const App::Property* prop)
{
    if (prop == &ChamferType) {
        updateProperties();
    }

    DressUp::onChanged(prop);
}

void Chamfer::updateProperties()
{
    auto chamferType = ChamferType.getValue();

    auto disableproperty = [](App::Property* prop, bool on) {
        prop->setStatus(App::Property::ReadOnly, on);
    };

    switch (chamferType) {
        case 0:  // "Equal distance"
            disableproperty(&this->Angle, true);
            disableproperty(&this->Size2, true);
            break;
        case 1:  // "Two distances"
            disableproperty(&this->Angle, true);
            disableproperty(&this->Size2, false);
            break;
        case 2:  // "Distance and Angle"
            disableproperty(&this->Angle, false);
            disableproperty(&this->Size2, true);
            break;
    }
}

static App::DocumentObjectExecReturn* validateParameters(
    int chamferType,
    double size,
    double size2,
    double angle
)
{
    // Size is common to all chamfer types.
    if (size <= 0) {
        return new App::DocumentObjectExecReturn(
            QT_TRANSLATE_NOOP("Exception", "Size must be greater than zero")
        );
    }

    switch (chamferType) {
        case 0:  // Equal distance
            // Nothing to do.
            break;
        case 1:  // Two distances
            if (size2 <= 0) {
                return new App::DocumentObjectExecReturn(
                    QT_TRANSLATE_NOOP("Exception", "Size2 must be greater than zero")
                );
            }
            break;
        case 2:  // Distance and angle
            if (angle <= 0 || angle >= 180.0) {
                return new App::DocumentObjectExecReturn(
                    QT_TRANSLATE_NOOP("Exception", "Angle must be greater than 0 and less than 180")
                );
            }
            break;
    }

    return App::DocumentObject::StdReturn;
}
