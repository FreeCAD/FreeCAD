// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2009 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include <BRepAdaptor_Curve.hxx>
#include <BRepPrimAPI_MakeRevol.hxx>
#include <gp_Ax1.hxx>
#include <gp_Circ.hxx>
#include <gp_Lin.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>

#include <deque>
#include <memory>


#include <App/Document.h>
#include <App/SemanticDocumentState.h>
#include <App/SemanticReference.h>
#include <Base/Tools.h>
#include "FeatureRevolution.h"
#include "FaceMaker.h"
#include "SemanticHistoryAdapter.h"
#include "SemanticSourceCollector.h"
#include "TopoShapeOpCode.h"


using namespace Part;
namespace
{
void publishRevolutionSemanticHistory(
    Revolution* self,
    BRepPrimAPI_MakeRevol* maker,
    const TopoShape& published
)
{
    if (!self || !maker || !maker->IsDone() || published.isNull()) {
        return;
    }
    App::SemanticGraph* graph = App::SemanticDocumentState::graphFor(self);
    if (!graph && self->getDocument()) {
        graph = &self->getDocument()->semanticGraph();
    }
    if (!graph) {
        return;
    }
    const App::ObjectId selfId = static_cast<App::ObjectId>(self->getID());
    App::EvalSerial eval = 0;
    if (App::Document* doc = self->getDocument()) {
        eval = doc->semanticState().currentEval();
    }
    std::deque<TopoDS_Shape> held;
    struct Candidate
    {
        App::ElementIndex index;
        App::SemanticKind kind = App::SemanticKind::Face;
    };
    std::vector<Candidate> candidates;
    auto boundAt = [graph, selfId](const App::ElementIndex& index) {
        return App::shouldRefuseBoundAt(graph, selfId, index);
    };
    auto addImage = [&](const TopoDS_Shape& image) {
        if (image.IsNull() || (image.ShapeType() != TopAbs_FACE && image.ShapeType() != TopAbs_EDGE)) {
            return;
        }
        for (const auto& prior : held) {
            if (prior.IsSame(image) || prior.IsPartner(image)) {
                return;
            }
        }
        const App::ElementIndex index = Part::uniqueNamedIndexOnPublished(published, image);
        if (!isNamedIndex(index)) {
            return;
        }
        if (boundAt(index)) {
            return;
        }
        // A3: defer recordGenerated until after fromMaker / uniqueOneImageGenerated
        // / supplementLocatedInputs (Mirroring deferred-mint pattern). Provisional
        // handles below are bookkeeping only - never written to the durable graph.
        held.push_back(image);
        Candidate candidate;
        candidate.index = index;
        candidate.kind = image.ShapeType() == TopAbs_FACE ? App::SemanticKind::Face
                                                          : App::SemanticKind::Edge;
        candidates.push_back(candidate);
    };
    for (TopExp_Explorer ex(maker->Shape(), TopAbs_FACE); ex.More(); ex.Next()) {
        addImage(ex.Current());
    }
    for (TopExp_Explorer ex(maker->Shape(), TopAbs_EDGE); ex.More(); ex.Next()) {
        addImage(ex.Current());
    }
    if (candidates.empty()) {
        // A4: TESTS *Diag gated (default off). Exact historical strings via
        // testsPublishDiagSkip/Bound — enable FREECAD_TESTS_DIAG=1 or
        // FreeCAD.setLogLevel('PartTestsDiag','Message'). Emit/I13 unchanged.
        Part::testsPublishDiagSkip("revolutionDiag", "no named maker images");
        return;
    }
    // Provisional SemanticIds (handle = 1..N) drive fromMaker uniqueness without
    // minting orphans. Real seeds are allocated only for surviving unique slots.
    std::vector<std::pair<App::SemanticId, const void*>> inputs;
    inputs.reserve(candidates.size());
    for (std::size_t i = 0; i < candidates.size(); ++i) {
        App::SemanticId provisional;
        provisional.handle = static_cast<App::SemanticHandle>(i + 1);
        provisional.kind = candidates[i].kind;
        inputs.push_back({provisional, static_cast<const void*>(&held[i])});
    }
    auto indexOf = [&published](const void* occ) -> App::ElementIndex {
        if (!occ) {
            return App::ElementIndex();
        }
        const TopoDS_Shape& shape = *static_cast<const TopoDS_Shape*>(occ);
        return Part::uniqueNamedIndexOnPublished(published, shape);
    };
    const HistoryTable makerHistory = SemanticHistoryAdapter::fromMaker(maker, inputs, indexOf);
    HistoryTable unique = SemanticHistoryAdapter::uniqueOneImageGenerated(makerHistory);
    // A non-empty but unusable maker history is an I13 failure, not permission
    // to mint a fresh binding from the published-shape fallback.
    if (makerHistory.empty()) {
        for (const auto& pair : inputs) {
            HistoryRecord record;
            record.fromSeed = pair.first;
            record.kind = App::EventKind::Generated;
            record.toIndex = indexOf(pair.second);
            record.outputKind = record.toIndex.type == "Edge" ? App::SemanticKind::Edge
                                                              : App::SemanticKind::Face;
            if (isNamedIndex(record.toIndex)) {
                unique.push_back(record);
            }
        }
        unique = SemanticHistoryAdapter::uniqueOneImageGenerated(unique);
    }
    else if (!unique.empty()) {
        // MakeRevol fromMaker often uniquely keeps Faces while vertical Edges
        // are unmodified survivors only present in locate inputs (Vertex-rooted
        // ElementMap). Supplement uncovered Edge slots without undoing Pass-32
        // refuse when unique is empty.
        unique = SemanticHistoryAdapter::supplementLocatedInputs(unique, inputs, indexOf);
    }
    HistoryTable toApply;
    std::vector<App::SemanticId> seeds;
    for (const HistoryRecord& record : unique) {
        if (!isNamedIndex(record.toIndex)) {
            continue;
        }
        App::SemanticKind kind = record.outputKind;
        if (kind != App::SemanticKind::Face && kind != App::SemanticKind::Edge) {
            kind = record.toIndex.type == "Edge" ? App::SemanticKind::Edge : App::SemanticKind::Face;
        }
        // A3 deferred mint: allocate only for unique surviving Face/Edge slots.
        const App::SemanticId seed
            = graph->recordGenerated(kind, Part::OpCodes::Revolve, selfId, eval, App::SemanticRole::None);
        if (!seed.valid()) {
            continue;
        }
        HistoryRecord minted = record;
        minted.fromSeed = seed;
        minted.kind = App::EventKind::Generated;
        minted.outputKind = kind;
        toApply.push_back(minted);
        seeds.push_back(seed);
    }
    if (toApply.empty()) {
        Part::testsPublishDiagSkip("revolutionDiag", "no unique images");
        return;
    }
    SemanticHistoryAdapter::applyHistory(graph, selfId, eval, Part::OpCodes::Revolve, seeds, toApply);
    Part::testsPublishDiagBound("revolutionDiag", toApply.size(), toApply.size());
}

}  // namespace

App::PropertyFloatConstraint::Constraints Revolution::angleRangeU = {-360.0, 360.0, 1.0};

PROPERTY_SOURCE(Part::Revolution, Part::Feature)

Revolution::Revolution()
{
    ADD_PROPERTY_TYPE(Source, (nullptr), "Revolve", App::Prop_None, "Shape to revolve");
    ADD_PROPERTY_TYPE(
        Base,
        (Base::Vector3d(0.0, 0.0, 0.0)),
        "Revolve",
        App::Prop_None,
        "Base point of revolution axis"
    );
    ADD_PROPERTY_TYPE(
        Axis,
        (Base::Vector3d(0.0, 0.0, 1.0)),
        "Revolve",
        App::Prop_None,
        "Direction of revolution axis"
    );
    ADD_PROPERTY_TYPE(
        AxisLink,
        (nullptr),
        "Revolve",
        App::Prop_None,
        "Link to edge to use as revolution axis."
    );
    ADD_PROPERTY_TYPE(
        Angle,
        (360.0),
        "Revolve",
        App::Prop_None,
        "Angle span of revolution. If angle is zero, and an arc is used for axis link, angle span "
        "of arc will be used."
    );
    Angle.setConstraints(&angleRangeU);
    ADD_PROPERTY_TYPE(
        Symmetric,
        (false),
        "Revolve",
        App::Prop_None,
        "Extend revolution symmetrically from the profile."
    );
    ADD_PROPERTY_TYPE(Solid, (false), "Revolve", App::Prop_None, "Make revolution a solid if possible");
    ADD_PROPERTY_TYPE(
        FaceMakerClass,
        (""),
        "Revolve",
        App::Prop_None,
        "Facemaker to use if Solid is true."
    );  // default for old documents. For default for new objects, refer to setupObject().
}

short Revolution::mustExecute() const
{
    if (Base.isTouched() || Axis.isTouched() || Angle.isTouched() || Source.isTouched()
        || Solid.isTouched() || AxisLink.isTouched() || Symmetric.isTouched()
        || FaceMakerClass.isTouched()) {
        return 1;
    }
    return 0;
}

void Revolution::onChanged(const App::Property* prop)
{
    if (!this->isRestoring()) {
        if (prop == &AxisLink) {
            Base.setReadOnly(AxisLink.getValue() != nullptr);
            Axis.setReadOnly(AxisLink.getValue() != nullptr);
        }
    }
    Part::Feature::onChanged(prop);
}

bool Revolution::fetchAxisLink(
    const App::PropertyLinkSub& axisLink,
    Base::Vector3d& center,
    Base::Vector3d& dir,
    double& angle
)
{
    if (!axisLink.getValue()) {
        return false;
    }

    auto linked = axisLink.getValue();

    TopoDS_Shape axEdge;
    if (!axisLink.getSubValues().empty() && axisLink.getSubValues()[0].length() > 0) {
        axEdge = Feature::getTopoShape(
                     linked,
                     ShapeOption::NeedSubElement | ShapeOption::ResolveLink | ShapeOption::Transform,
                     axisLink.getSubValues()[0].c_str()
        )
                     .getShape();
    }
    else {
        axEdge = Feature::getShape(linked, ShapeOption::ResolveLink | ShapeOption::Transform);
    }

    if (axEdge.IsNull()) {
        throw Base::ValueError("AxisLink shape is null");
    }
    if (axEdge.ShapeType() != TopAbs_EDGE) {
        throw Base::TypeError("AxisLink shape is not an edge");
    }

    BRepAdaptor_Curve crv(TopoDS::Edge(axEdge));
    gp_Pnt base;
    gp_Dir occdir;
    bool reversed = axEdge.Orientation() == TopAbs_REVERSED;
    if (crv.GetType() == GeomAbs_Line) {
        base = crv.Value(reversed ? crv.FirstParameter() : crv.LastParameter());
        occdir = crv.Line().Direction();
    }
    else if (crv.GetType() == GeomAbs_Circle) {
        base = crv.Circle().Axis().Location();
        occdir = crv.Circle().Axis().Direction();
        angle = crv.LastParameter() - crv.FirstParameter();
    }
    else {
        throw Base::TypeError("AxisLink edge is neither line nor arc of circle.");
    }
    if (reversed) {
        occdir.Reverse();
    }
    center.Set(base.X(), base.Y(), base.Z());
    dir.Set(occdir.X(), occdir.Y(), occdir.Z());
    return true;
}

App::DocumentObjectExecReturn* Revolution::execute()
{
    App::DocumentObject* link = Source.getValue();
    if (!link) {
        return new App::DocumentObjectExecReturn("No object linked");
    }

    try {
        // read out axis link
        double angle_edge = 0;
        Base::Vector3d b = Base.getValue();
        Base::Vector3d v = Axis.getValue();
        bool linkFetched = this->fetchAxisLink(this->AxisLink, b, v, angle_edge);
        if (linkFetched) {
            this->Base.setValue(b);
            this->Axis.setValue(v);
        }

        gp_Pnt pnt(b.x, b.y, b.z);
        gp_Dir dir(v.x, v.y, v.z);
        gp_Ax1 revAx(pnt, dir);

        // read out revolution angle
        double angle = Base::toRadians(Angle.getValue());
        if (fabs(angle) < Precision::Angular()) {
            angle = angle_edge;
        }

        // apply "midplane" symmetry
        TopoShape sourceShape
            = Feature::getTopoShape(link, ShapeOption::ResolveLink | ShapeOption::Transform);

        TopoDS_Shape shape = sourceShape.getShape();
        if (shape.IsNull()) {
            return new App::DocumentObjectExecReturn("Cannot revolve null shape");
        }
        if (shape.Infinite()) {
            return new App::DocumentObjectExecReturn("Cannot revolve infinite shape");
        }

        if (Symmetric.getValue()) {
            // rotate source shape backwards by half angle, to make resulting revolution symmetric
            // to the profile
            gp_Trsf mov;
            mov.SetRotation(revAx, angle * (-0.5));
            TopLoc_Location loc(mov);
            sourceShape.setShape(sourceShape.getShape().Moved(loc));
        }
        TopoShape revolve(0, getDocument()->getStringHasher());
        if (Solid.getValue() && !sourceShape.hasSubShape(TopAbs_FACE)) {
            if (!sourceShape.hasSubShape(TopAbs_WIRE)) {
                sourceShape = sourceShape.makeElementWires();
            }
            sourceShape = sourceShape.makeElementFace(nullptr, FaceMakerClass.getValue());
        }
        auto liveRevol = std::make_unique<BRepPrimAPI_MakeRevol>(sourceShape.getShape(), revAx, angle);
        if (!liveRevol->IsDone() || liveRevol->Shape().IsNull()) {
            return new App::DocumentObjectExecReturn("Resulting shape is null");
        }
        revolve.makeElementShape(*liveRevol, sourceShape, Part::OpCodes::Revolve);
        if (revolve.isNull()) {
            return new App::DocumentObjectExecReturn("Resulting shape is null");
        }
        this->Shape.setValue(revolve);
        publishRevolutionSemanticHistory(this, liveRevol.get(), revolve);
        return App::DocumentObject::StdReturn;
    }
    catch (Standard_Failure& e) {
        return new App::DocumentObjectExecReturn(e.GetMessageString());
    }
}

void Part::Revolution::setupObject()
{
    Part::Feature::setupObject();
    this->FaceMakerClass.setValue("Part::FaceMakerUnified");  // default for newly created features
}
