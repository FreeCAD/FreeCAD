// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2008 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
#include <BRepFilletAPI_MakeFillet.hxx>
#include <BRep_Tool.hxx>
#include <Geom_Circle.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_ListOfShape.hxx>
#include <ShapeFix_Shape.hxx>
#include <ShapeFix_ShapeTolerance.hxx>

#include <App/Document.h>
#include <App/PropertyLinks.h>
#include <App/SemanticDocumentState.h>
#include <Base/Exception.h>
#include <Base/Reader.h>
#include <Mod/Part/App/TopoShape.h>
#include <Mod/Part/App/TopoShapeOpCode.h>

#include "FeatureFillet.h"
#include "SemanticOpcode.h"
#include <Mod/Part/App/SemanticHistoryAdapter.h>


using namespace PartDesign;


PROPERTY_SOURCE(PartDesign::Fillet, PartDesign::DressUp)

const App::PropertyQuantityConstraint::Constraints floatRadius
    = {0.0, std::numeric_limits<float>::max(), 0.1};

Fillet::Fillet()
{
    ADD_PROPERTY_TYPE(Radius, (1.0), "Fillet", App::Prop_None, "Fillet radius.");
    Radius.setUnit(Base::Unit::Length);
    Radius.setConstraints(&floatRadius);
    ADD_PROPERTY_TYPE(
        UseAllEdges,
        (false),
        "Fillet",
        App::Prop_None,
        "Fillet all edges if true, else use only those edges in Base property.\n"
        "If true, then this overrides any edge changes made to the Base property or in the "
        "dialog.\n"
    );
}

short Fillet::mustExecute() const
{
    if (Placement.isTouched() || Radius.isTouched()) {
        return 1;
    }

    // Old FCStd files can restore a valid cached shape while their STG1 has no
    // durable rows for this publisher. Ask the normal scheduler to execute
    // this feature once; do not touch unrelated document objects.
    if (isSemanticRepublishPass(SemanticEmitter::graphFor(this))) {
        return 1;
    }
    return DressUp::mustExecute();
}

App::DocumentObjectExecReturn* Fillet::execute()
{
    // A restored document may have a valid cached result and a touched Refine
    // property, while the durable semantic history for this feature is absent.
    // The normal Refine fast path would then return before the maker exists,
    // leaving mustExecute sticky and giving downstream links no Face Binding.
    // Force the full maker path for this narrowly scoped republish pass.
    App::SemanticGraph* graph = SemanticEmitter::graphFor(this);
    const App::ObjectId fid = static_cast<App::ObjectId>(getID());
    const bool semanticRepublish = isSemanticRepublishPass(graph);
    if (!semanticRepublish && onlyHaveRefined()) {
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

    App::EvalSerial eval = 0;
    AfterExecuteRequest req;
    if (App::Document* doc = getDocument()) {
        eval = doc->semanticState().currentEval();
    }
    collectDressUpBaseSeeds(req);
    // R2: resolve FilletEdge before makeElementFillet. Missing / Incompatible
    // skips the maker. Do not pick a similar-length neighbour (I10).
    const Part::FilletPreflight pre
        = Part::SemanticHistoryAdapter::preflightFillet(graph, req.filletEdges);
    // During a restore-only republish the graph has no live Bindings yet, so a
    // valid restored seed resolves as Missing even though its cached Base
    // subname is still authoritative for rebuilding the shape.
    if (pre.makerSkipped) {
        if (Part::SemanticHistoryAdapter::canUseCachedGeometry(pre, semanticRepublish)) {
            req.filletEdges.clear();
        }
        else {
            SemanticEmitter::afterExecute(graph, Opcode::Fillet, fid, eval, req);
            return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP(
                "Exception",
                "Fillet edge Missing; maker skipped. No similar-length neighbour substitution."
            ));
        }
    }

    if (graph && graph->hasBindings() && Base.getValue()) {
        const App::ObjectId baseFeature = static_cast<App::ObjectId>(
            Base.getValue()->semanticProjectionFeatureId()
        );
        retainResolvedDressUpSeeds(graph, baseFeature, App::SemanticKind::Edge, req.filletEdges);
        retainResolvedDressUpSeeds(graph, baseFeature, App::SemanticKind::Face, req.filletAdjacentFaces);
    }

    // Candidate A/B (manual-3-dressup-relink-a): getContinuousEdges refreshes
    // Base LinkSub first; already-broken '?Edge*' throws CADKernelError which
    // we convert to ExecReturn here (no Document reportException spam).
    std::vector<TopoShape> edges;
    try {
        edges = UseAllEdges.getValue() ? baseShape.getSubTopoShapes(TopAbs_EDGE)
                                       : getContinuousEdges(baseShape, graph);
    }
    catch (Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }
    if (edges.empty()) {
        return new App::DocumentObjectExecReturn(
            QT_TRANSLATE_NOOP("Exception", "Fillet not possible on selected shapes")
        );
    }

    double radius = Radius.getValue();

    if (radius <= 0) {
        return new App::DocumentObjectExecReturn(
            QT_TRANSLATE_NOOP("Exception", "Fillet radius must be greater than zero")
        );
    }

    this->positionByBaseFeature();

    try {
        TopoShape shape(0);  //,getDocument()->getStringHasher());

        // Add signal handler for segfault protection
#if defined(__GNUC__) && defined(FC_OS_LINUX)
        Base::SignalException se;
#endif

        BRepFilletAPI_MakeFillet mkFillet(baseShape.getShape());
        for (auto& e : edges) {
            if (e.isNull()) {
                continue;
            }
            mkFillet.Add(Radius.getValue(), Radius.getValue(), TopoDS::Edge(e.getShape()));
        }
        shape.makeElementShape(mkFillet, baseShape, Part::OpCodes::Fillet);
        if (shape.isNull()) {
            return new App::DocumentObjectExecReturn(
                QT_TRANSLATE_NOOP("Exception", "Resulting shape is null")
            );
        }

        TopTools_ListOfShape aLarg;
        aLarg.Append(baseShape.getShape());
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
        // Bind Generated fillet faces from the maker history. Empty / unnamed
        // → leave Binding empty (I10). Never sequential FaceN.
        // Does not resetElementMap. Does not replace Shape.setValue.
        std::deque<TopoDS_Shape> held;
        std::vector<std::pair<App::SemanticId, const void*>> inputs;
        if (graph && Base.getValue()) {
            const App::ObjectId baseFeature = static_cast<App::ObjectId>(
                Base.getValue()->semanticProjectionFeatureId()
            );
            for (const App::SemanticId& edge : req.filletEdges) {
                const auto unique
                    = uniqueResolvedDressUpBinding(graph, edge, baseFeature, App::SemanticKind::Edge);
                if (!unique) {
                    continue;
                }
                TopoDS_Shape es = baseShape.findShape(TopAbs_EDGE, unique->index.index);
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
                inputs.push_back({App::SemanticId {}, &held.back()});
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
        // fromMaker only: BRepFilletAPI_MakeFillet has no History() on this OCCT.
        // Empty table -> applyHistory half-map, no sequential FaceN (I13).
        Part::HistoryTable hist = Part::SemanticHistoryAdapter::fromMaker(&mkFillet, inputs, indexOf);
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
            graph,
            fid,
            eval,
            "Fillet",
            req.filletEdges,
            toApply
        );
        if (applied.boundCount == 0) {
            SemanticEmitter::afterExecute(graph, Opcode::Fillet, fid, eval, req);
        }
        if (semanticRepublish) {
            AfterExecuteRequest profileReq;
            SemanticEmitter::appendInverseProfileFaceIndices(this, shape, profileReq.namedFaceIndices);
            if (!profileReq.namedFaceIndices.empty()) {
                SemanticEmitter::afterExecute(graph, Opcode::Fillet, fid, eval, profileReq);
            }
        }
        return App::DocumentObject::StdReturn;
    }
    catch (Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }
    catch (Standard_Failure& e) {
        return new App::DocumentObjectExecReturn(e.GetMessageString());
    }
    catch (...) {
        return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP(
            "Exception",
            "Fillet operation failed. The selected edges may contain geometry that cannot be "
            "filleted together. "
            "Try filleting edges individually or with a smaller radius."
        ));
    }
}

void Fillet::Restore(Base::XMLReader& reader)
{
    DressUp::Restore(reader);
}

void Fillet::handleChangedPropertyType(Base::XMLReader& reader, const char* TypeName, App::Property* prop)
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
