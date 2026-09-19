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

#include <limits>
#include <algorithm>
#include <cmath>
#include <memory>
#include <utility>
#include <unordered_set>

#include <BRepPrim_Cylinder.hxx>
#include <Mod/Part/App/FCBRepAlgoAPI_Cut.h>
#include <Mod/Part/App/FCBRepAlgoAPI_Fuse.h>
#include <BRepAlgoAPI_BooleanOperation.hxx>
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <TopTools_ListOfShape.hxx>
#include <Mod/Part/App/FuzzyHelper.h>
#include <BRepBuilderAPI_GTransform.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakePolygon.hxx>
#include <BRepBuilderAPI_MakeSolid.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeCone.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepPrimAPI_MakeSphere.hxx>
#include <BRepPrimAPI_MakeTorus.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRepPrim_Wedge.hxx>

#include <App/DocumentObject.h>
#include <Base/Exception.h>
#include <Base/Tools.h>
#include <App/FeaturePythonPyImp.h>

#include "FeaturePrimitive.h"
#include "FeaturePy.h"
#include "Mod/Part/App/TopoShapeOpCode.h"

#include <deque>
#include <string>
#include <optional>
#include <vector>

#include <BRepAdaptor_Surface.hxx>
#include <BRep_Tool.hxx>
#include <Precision.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Vertex.hxx>
#include <gp_Pln.hxx>
#include <gp_Pnt.hxx>

#include <App/Document.h>
#include <App/SemanticDocumentState.h>
#include <App/SemanticReference.h>
#include <Base/Console.h>
#include <Mod/Part/App/SemanticHistoryAdapter.h>
#include <Mod/Part/App/SemanticSourceCollector.h>

#include "SemanticOpcode.h"

using namespace PartDesign;

namespace PartDesign
{

namespace
{


struct LocatedPrimitiveCandidate
{
    TopoDS_Shape shape;
    App::ElementIndex index;
    App::SemanticKind kind = App::SemanticKind::Face;
};

/// Locate primitive maker images before allocating semantic identities. A
/// fallback locate that maps two distinct maker images to one published slot
/// is ambiguous; reject every owner of that slot instead of letting map order
/// mint one identity and leave the other unnamed (I13).
std::vector<LocatedPrimitiveCandidate> uniqueLocatedPrimitiveCandidates(
    const TopoDS_Shape& makerSolid,
    const Part::TopoShape& published
)
{
    std::vector<LocatedPrimitiveCandidate> candidates;
    auto consider = [&](const TopoDS_Shape& sub) {
        if (sub.IsNull() || (sub.ShapeType() != TopAbs_FACE && sub.ShapeType() != TopAbs_EDGE)) {
            return;
        }
        for (const auto& candidate : candidates) {
            if (candidate.shape.IsSame(sub) || candidate.shape.IsPartner(sub)) {
                return;
            }
        }
        const App::ElementIndex index = Part::indexOnPublished(published, sub);
        if (!Part::isNamedIndex(index)) {
            return;
        }
        LocatedPrimitiveCandidate candidate;
        candidate.shape = sub;
        candidate.index = index;
        candidate.kind = sub.ShapeType() == TopAbs_FACE ? App::SemanticKind::Face
                                                        : App::SemanticKind::Edge;
        candidates.push_back(std::move(candidate));
    };
    for (TopExp_Explorer ex(makerSolid, TopAbs_FACE); ex.More(); ex.Next()) {
        consider(ex.Current());
    }
    for (TopExp_Explorer ex(makerSolid, TopAbs_EDGE); ex.More(); ex.Next()) {
        consider(ex.Current());
    }

    std::vector<LocatedPrimitiveCandidate> unique;
    unique.reserve(candidates.size());
    for (std::size_t i = 0; i < candidates.size(); ++i) {
        bool ownsSlotAlone = true;
        for (std::size_t j = 0; j < candidates.size(); ++j) {
            if (i != j && candidates[i].index == candidates[j].index) {
                ownsSlotAlone = false;
                break;
            }
        }
        if (ownsSlotAlone) {
            unique.push_back(std::move(candidates[i]));
        }
    }
    return unique;
}

/// Locate/tool mint gate uses App::shouldRefuseBoundAt (shared I13 policy:
/// conflict or uniquely published owner). Unique published → reuse via
/// uniqueIdentityAtIndex; conflicted → leave unnamed; empty slot → mint.
/// Additive Ellipsoid/Wedge locate publishers also consume this helper
/// (no reliable fromMaker history), matching Box/Cylinder empty-history
/// fallback policy. Subtractive Cut considerTool uses the same gate.

std::vector<std::pair<App::SemanticId, const void*>> collectLocatedPrimitiveInputs(
    App::SemanticGraph* graph,
    App::ObjectId feature,
    App::EvalSerial eval,
    const char* opcode,
    const std::vector<LocatedPrimitiveCandidate>& candidates,
    std::deque<TopoDS_Shape>& held
)
{
    std::vector<std::pair<App::SemanticId, const void*>> inputs;
    if (!graph || feature == 0 || !opcode) {
        return inputs;
    }
    inputs.reserve(candidates.size());
    for (const auto& candidate : candidates) {
        // shouldRefuseBoundAt = conflict || unique published. Unique → reuse;
        // conflict (refuse without unique) → leave unnamed; empty → mint.
        App::SemanticId seed;
        if (App::shouldRefuseBoundAt(graph, feature, candidate.index)) {
            seed = App::uniqueIdentityAtIndex(graph, feature, candidate.index);
            if (!seed.valid()) {
                continue;
            }
        }
        else {
            seed = graph->recordGenerated(candidate.kind, opcode, feature, eval, App::SemanticRole::None);
        }
        if (!seed.valid()) {
            continue;
        }
        held.push_back(candidate.shape);
        inputs.push_back({seed, &held.back()});
    }
    return inputs;
}

/// Same LNK2019 lesson as FeatureBoolean: do not call FCBRepAlgoAPIHelper.
void setCutAutoFuzzy(
    FCBRepAlgoAPI_Cut* fcCut,
    const TopTools_ListOfShape& args,
    const TopTools_ListOfShape& tools
)
{
    Bnd_Box bounds;
    for (TopTools_ListOfShape::Iterator it(args); it.More(); it.Next()) {
        BRepBndLib::Add(it.Value(), bounds);
    }
    for (TopTools_ListOfShape::Iterator it(tools); it.More(); it.Next()) {
        BRepBndLib::Add(it.Value(), bounds);
    }
    fcCut->SetFuzzyValue(
        Part::FuzzyHelper::getBooleanFuzzy() * sqrt(bounds.SquareExtent()) * Precision::Confusion()
    );
}

}  // namespace


const App::PropertyQuantityConstraint::Constraints torusRangeV = {-180.0, 180.0, 1.0};
const App::PropertyQuantityConstraint::Constraints angleRangeU = {0.0, 360.0, 1.0};
const App::PropertyQuantityConstraint::Constraints angleRangeV = {-90.0, 90.0, 1.0};
// it turned out that OCC cannot e.g. create a box with a width of Precision::Confusion()
// with two times Precision::Confusion() all geometric primitives can be created
const App::PropertyQuantityConstraint::Constraints quantityRange
    = {2 * Precision::Confusion(), std::numeric_limits<float>::max(), 0.1};
const App::PropertyQuantityConstraint::Constraints quantityRangeZero
    = {0.0, std::numeric_limits<float>::max(), 0.1};

PROPERTY_SOURCE_WITH_EXTENSIONS(PartDesign::FeaturePrimitive, PartDesign::FeatureAddSub)

FeaturePrimitive::FeaturePrimitive()
{
    Part::AttachExtension::initExtension(this);
}

App::DocumentObjectExecReturn* FeaturePrimitive::execute(const TopoDS_Shape& primitive)
{
    if (onlyHaveRefined()) {
        return App::DocumentObject::StdReturn;
    }

    try {
        // transform the primitive in the correct coordinance
        FeatureAddSub::execute();

        // if we have no base we just add the standard primitive shape
        TopoShape primitiveShape;
        primitiveShape.setShape(primitive);

        TopoShape base;
        try {
            // if we have a base shape we need to make sure that it does not get our transformation
            // to
            base = getBaseTopoShape().moved(getLocation().Inverted());
            primitiveShape.Tag = -this->getID();
        }

        catch (const Base::Exception&) {

            // as we use this for preview we can add it even if useless for subtractive
            AddSubShape.setValue(primitiveShape);

            if (getAddSubType() == FeatureAddSub::Type::Additive) {
                Shape.setValue(getSolid(primitiveShape));
            }
            else {
                return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP(
                    "Exception",
                    "Cannot subtract primitive feature without base feature"
                ));
            }

            return App::DocumentObject::StdReturn;
        }
        AddSubShape.setValue(primitiveShape);

        TopoShape boolOp(0);
        std::unique_ptr<BRepAlgoAPI_BooleanOperation> mkCut;

        const char* maker;
        switch (getAddSubType()) {
            case FeatureAddSub::Type::Additive:
                maker = Part::OpCodes::Fuse;
                break;
            case FeatureAddSub::Type::Subtractive:
                maker = Part::OpCodes::Cut;
                break;
            default:
                return new App::DocumentObjectExecReturn(
                    QT_TRANSLATE_NOOP("Exception", "Unknown operation type")
                );
        }
        try {
            // Subtractive primitives: live FCBRepAlgoAPI_Cut (shared Cut pattern).
            // Local setCutAutoFuzzy — NEVER FCBRepAlgoAPIHelper (LNK2019).
            // The shared publisher runs fromMaker -> applyHistory -> afterExecute.
            if (getAddSubType() == FeatureAddSub::Type::Subtractive) {
                auto* fcCut = new FCBRepAlgoAPI_Cut;
                mkCut.reset(fcCut);
                TopTools_ListOfShape args;
                TopTools_ListOfShape toolList;
                args.Append(base.getShape());
                toolList.Append(primitiveShape.getShape());
                fcCut->SetArguments(args);
                fcCut->SetTools(toolList);
                const double fuzzy = FuzzyTolerance.getValue();
                if (fuzzy > 0.0) {
                    fcCut->SetFuzzyValue(fuzzy);
                }
                else {
                    setCutAutoFuzzy(fcCut, args, toolList);
                }
                fcCut->Build();
                if (!fcCut->IsDone()) {
                    return new App::DocumentObjectExecReturn(
                        QT_TRANSLATE_NOOP("Exception", "Failed to perform boolean operation")
                    );
                }
                boolOp.makeElementShape(*fcCut, {base, primitiveShape}, maker);
            }
            else {
                boolOp.makeElementBoolean(
                    maker,
                    {base, primitiveShape},
                    nullptr,
                    FuzzyTolerance.getValue()
                );
            }
        }
        catch (Standard_Failure&) {
            return new App::DocumentObjectExecReturn(
                QT_TRANSLATE_NOOP("Exception", "Failed to perform boolean operation")
            );
        }

        TopoShape solidBoolOp = getSolid(boolOp);
        // lets check if the result is a solid
        if (solidBoolOp.isNull()) {
            return new App::DocumentObjectExecReturn(
                QT_TRANSLATE_NOOP("Exception", "Resulting shape is not a solid")
            );
        }
        // store shape before refinement
        this->rawShape = boolOp;

        App::DocumentObject* baseObj = getBaseObject(/* silent = */ true);

        if (solidBoolOp == base) {
            // solidBoolOp is misplaced but boolOp is ok
            Shape.setValue(boolOp);
            if (mkCut) {
                onSubtractiveCutDone(mkCut.get(), primitive, base.getShape(), baseObj);
            }
            return App::DocumentObject::StdReturn;
        }
        // Refine replaces TShapes; fromMaker images would miss. Skip when live Cut.
        if (!mkCut) {
            solidBoolOp = refineShapeIfActive(solidBoolOp);
        }
        Shape.setValue(getSolid(solidBoolOp));
        if (mkCut) {
            onSubtractiveCutDone(mkCut.get(), primitive, base.getShape(), baseObj);
        }
    }
    catch (Standard_Failure& e) {

        return new App::DocumentObjectExecReturn(e.GetMessageString());
    }

    return App::DocumentObject::StdReturn;
}

void FeaturePrimitive::onChanged(const App::Property* prop)
{
    if (prop == &AttachmentOffset) {
        this->recompute();
        return;
    }

    FeatureAddSub::onChanged(prop);
}

// suppress warning about tp_print for Py3.8
#if defined(__clang__)
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wmissing-field-initializers"
#endif

PYTHON_TYPE_DEF(PrimitivePy, PartDesign::FeaturePy)  // explicit bombs
PYTHON_TYPE_IMP(PrimitivePy, PartDesign::FeaturePy)

#if defined(__clang__)
# pragma clang diagnostic pop
#endif

PyObject* FeaturePrimitive::getPyObject()
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new PrimitivePy(this), true);
    }
    return Py::new_reference_to(PythonObject);
}

PROPERTY_SOURCE(PartDesign::Box, PartDesign::FeaturePrimitive)

Box::Box()
{
    ADD_PROPERTY_TYPE(Length, (10.0f), "Box", App::Prop_None, "The length of the box");
    ADD_PROPERTY_TYPE(Width, (10.0f), "Box", App::Prop_None, "The width of the box");
    ADD_PROPERTY_TYPE(Height, (10.0f), "Box", App::Prop_None, "The height of the box");
    Length.setConstraints(&quantityRange);
    Width.setConstraints(&quantityRange);
    Height.setConstraints(&quantityRange);

    primitiveType = FeaturePrimitive::Box;
}


void FeaturePrimitive::onSubtractiveCutDone(
    BRepAlgoAPI_BooleanOperation* /*mkCut*/,
    const TopoDS_Shape& /*toolShape*/,
    const TopoDS_Shape& /*baseShape*/,
    App::DocumentObject* /*baseObj*/
)
{}

void Box::onSubtractiveCutDone(
    BRepAlgoAPI_BooleanOperation* mkCut,
    const TopoDS_Shape& toolShape,
    const TopoDS_Shape& baseShape,
    App::DocumentObject* baseObj
)
{
    if (getAddSubType() != FeatureAddSub::Type::Subtractive) {
        return;
    }
    publishSubtractiveCutSemanticHistory(
        mkCut,
        toolShape,
        baseShape,
        baseObj,
        Opcode::SubtractiveBox,
        "subBoxDiag"
    );
}

void FeaturePrimitive::publishSubtractiveCutSemanticHistory(
    BRepAlgoAPI_BooleanOperation* mkCut,
    const TopoDS_Shape& toolShape,
    const TopoDS_Shape& baseShape,
    App::DocumentObject* baseObj,
    Opcode opcode,
    const char* diagName
)
{
    SemanticEmitter::publishSubtractiveCutHistory(
        this,
        mkCut,
        &toolShape,
        &baseShape,
        baseObj,
        opcode,
        diagName
    );
}

void Box::publishAdditiveBoxSemanticHistory(BRepPrimAPI_MakeBox* mkBox)
{
    // First-solid AdditiveBox only. SubtractiveBox with-base uses live Cut emit.
    if (!mkBox || getAddSubType() != FeatureAddSub::Type::Additive) {
        return;
    }
    if (getBaseObject(/* silent = */ true)) {
        return;
    }
    const Part::TopoShape published = this->Shape.getShape();
    if (published.isNull()) {
        return;
    }
    App::SemanticGraph* graph = SemanticEmitter::graphFor(this);
    if (!graph && getDocument()) {
        graph = &getDocument()->semanticGraph();
    }
    if (!graph) {
        return;
    }

    const App::ObjectId selfId = static_cast<App::ObjectId>(getID());
    App::EvalSerial eval = 0;
    if (App::Document* doc = getDocument()) {
        eval = doc->semanticState().currentEval();
    }

    const std::vector<LocatedPrimitiveCandidate> candidates
        = uniqueLocatedPrimitiveCandidates(mkBox->Shape(), published);
    std::deque<TopoDS_Shape> held;
    const std::vector<std::pair<App::SemanticId, const void*>> inputs = collectLocatedPrimitiveInputs(
        graph,
        selfId,
        eval,
        opcodeName(Opcode::AdditiveBox),
        candidates,
        held
    );


    AfterExecuteRequest req;
    req.allowSequentialFaceN = false;
    if (inputs.empty()) {
        SemanticEmitter::afterExecute(graph, Opcode::AdditiveBox, selfId, eval, req);
        SemanticEmitter::appendAfterExecuteNote("skip emit: no unique AdditiveBox maker images");
        Base::Console().message("TESTS boxDiag %s\n", SemanticEmitter::lastAfterExecuteNote().c_str());
        return;
    }

    auto indexOf = [&published](const void* occ) -> App::ElementIndex {
        App::ElementIndex idx;
        if (!occ) {
            return idx;
        }
        return Part::indexOnPublished(published, *static_cast<const TopoDS_Shape*>(occ));
    };

    const Part::HistoryTable raw = Part::SemanticHistoryAdapter::fromMaker(mkBox, inputs, indexOf);
    Part::HistoryTable unique = Part::SemanticHistoryAdapter::uniqueOneImageGenerated(raw);
    // MakeBox has no Generated/Modified arguments. Unmodified survivor of the
    // live maker FACE/EDGE is the unique 1-image. If fromMaker is empty
    // (TShape miss), bind the already-located maker images (I13 unnamed skip).
    if (unique.empty()) {
        for (const auto& pair : inputs) {
            if (!pair.second) {
                continue;
            }
            Part::HistoryRecord rec;
            rec.fromSeed = pair.first;
            rec.kind = App::EventKind::Generated;
            rec.toIndex
                = Part::indexOnPublished(published, *static_cast<const TopoDS_Shape*>(pair.second));
            if (rec.toIndex.type == "Edge") {
                rec.outputKind = App::SemanticKind::Edge;
            }
            else if (rec.toIndex.type == "Face") {
                rec.outputKind = App::SemanticKind::Face;
            }
            else {
                continue;
            }
            unique.push_back(rec);
        }
        unique = Part::SemanticHistoryAdapter::uniqueOneImageGenerated(unique);
    }

    // Bind the 0-to-1 AdditiveBox identities (fromSeed). Do not applyHistory
    // remint children — that would double-bind FaceN/EdgeN (I13). allocatedBy
    // is this AdditiveBox. Fillet.Base consumes the unique Edge Binding.
    std::size_t nBound = 0;
    for (const Part::HistoryRecord& rec : unique) {
        if (!Part::isNamedIndex(rec.toIndex) || !rec.fromSeed.valid()) {
            continue;
        }
        // bind() appends. Re-execute must not add a second row (I13).
        // I13: refuse remint-bind when slot is conflicted or already uniquely published.
        if (!App::shouldRefuseBoundAt(graph, selfId, rec.toIndex)) {
            SemanticEmitter::bind(graph, rec.fromSeed, selfId, eval, rec.toIndex);
        }
        ++nBound;
        if (rec.toIndex.type == "Face") {
            req.namedFaceIndices.push_back(rec.toIndex);
        }
        else if (rec.toIndex.type == "Edge") {
            req.namedEdgeIndices.push_back(rec.toIndex);
        }
    }
    SemanticEmitter::afterExecute(graph, Opcode::AdditiveBox, selfId, eval, req);
    SemanticEmitter::appendAfterExecuteNote(
        std::string("bound=") + std::to_string(nBound) + " named=" + std::to_string(nBound) + " unnamed=0"
    );
    Base::Console().message("TESTS boxDiag %s\n", SemanticEmitter::lastAfterExecuteNote().c_str());
}

App::DocumentObjectExecReturn* Box::execute()
{
    double L = Length.getValue();
    double W = Width.getValue();
    double H = Height.getValue();

    if (L < Precision::Confusion()) {
        return new App::DocumentObjectExecReturn(
            QT_TRANSLATE_NOOP("Exception", "Length of box too small")
        );
    }
    if (W < Precision::Confusion()) {
        return new App::DocumentObjectExecReturn(
            QT_TRANSLATE_NOOP("Exception", "Width of box too small")
        );
    }
    if (H < Precision::Confusion()) {
        return new App::DocumentObjectExecReturn(
            QT_TRANSLATE_NOOP("Exception", "Height of box too small")
        );
    }

    try {
        // Keep the live maker. FeaturePrimitive::execute(Shape()) used to
        // discard BRepPrimAPI_MakeBox; first-solid AdditiveBox emit needs it.
        BRepPrimAPI_MakeBox mkBox(L, W, H);
        App::DocumentObjectExecReturn* ret = FeaturePrimitive::execute(mkBox.Shape());
        if (ret == App::DocumentObject::StdReturn) {
            publishAdditiveBoxSemanticHistory(&mkBox);
        }
        return ret;
    }
    catch (Standard_Failure& e) {
        return new App::DocumentObjectExecReturn(e.GetMessageString());
    }
}

short int Box::mustExecute() const
{
    if (Length.isTouched() || Height.isTouched() || Width.isTouched()) {
        return 1;
    }

    return FeaturePrimitive::mustExecute();
}

PROPERTY_SOURCE(PartDesign::AdditiveBox, PartDesign::Box)
PROPERTY_SOURCE(PartDesign::SubtractiveBox, PartDesign::Box)


PROPERTY_SOURCE(PartDesign::Cylinder, PartDesign::FeaturePrimitive)

Cylinder::Cylinder()
{
    ADD_PROPERTY_TYPE(Radius, (10.0f), "Cylinder", App::Prop_None, "The radius of the cylinder");
    ADD_PROPERTY_TYPE(Angle, (360.0f), "Cylinder", App::Prop_None, "The closing angle of the cylinder ");
    ADD_PROPERTY_TYPE(Height, (10.0f), "Cylinder", App::Prop_None, "The height of the cylinder");
    Angle.setConstraints(&angleRangeU);
    Radius.setConstraints(&quantityRange);
    Height.setConstraints(&quantityRange);

    Part::PrismExtension::initExtension(this);

    primitiveType = FeaturePrimitive::Cylinder;
}

void Cylinder::onSubtractiveCutDone(
    BRepAlgoAPI_BooleanOperation* mkCut,
    const TopoDS_Shape& toolShape,
    const TopoDS_Shape& baseShape,
    App::DocumentObject* baseObj
)
{
    if (getAddSubType() != FeatureAddSub::Type::Subtractive) {
        return;
    }
    publishSubtractiveCutSemanticHistory(
        mkCut,
        toolShape,
        baseShape,
        baseObj,
        Opcode::SubtractiveCylinder,
        "subCylinderDiag"
    );
}

void Cylinder::publishAdditiveCylinderSemanticHistory(BRepPrimAPI_MakeCylinder* mkCylr)
{
    // First-solid AdditiveCylinder only; with-base subtractive uses the shared Cut publisher.
    if (!mkCylr || getAddSubType() != FeatureAddSub::Type::Additive) {
        return;
    }
    if (getBaseObject(/* silent = */ true)) {
        return;
    }
    const Part::TopoShape published = this->Shape.getShape();
    if (published.isNull()) {
        return;
    }
    App::SemanticGraph* graph = SemanticEmitter::graphFor(this);
    if (!graph && getDocument()) {
        graph = &getDocument()->semanticGraph();
    }
    if (!graph) {
        return;
    }

    const App::ObjectId selfId = static_cast<App::ObjectId>(getID());
    App::EvalSerial eval = 0;
    if (App::Document* doc = getDocument()) {
        eval = doc->semanticState().currentEval();
    }

    const std::vector<LocatedPrimitiveCandidate> candidates
        = uniqueLocatedPrimitiveCandidates(mkCylr->Shape(), published);
    std::deque<TopoDS_Shape> held;
    const std::vector<std::pair<App::SemanticId, const void*>> inputs = collectLocatedPrimitiveInputs(
        graph,
        selfId,
        eval,
        opcodeName(Opcode::AdditiveCylinder),
        candidates,
        held
    );


    AfterExecuteRequest req;
    req.allowSequentialFaceN = false;
    if (inputs.empty()) {
        SemanticEmitter::afterExecute(graph, Opcode::AdditiveCylinder, selfId, eval, req);
        SemanticEmitter::appendAfterExecuteNote("skip emit: no unique AdditiveCylinder maker images");
        Base::Console().message("TESTS cylDiag %s\n", SemanticEmitter::lastAfterExecuteNote().c_str());
        return;
    }

    auto indexOf = [&published](const void* occ) -> App::ElementIndex {
        App::ElementIndex idx;
        if (!occ) {
            return idx;
        }
        return Part::indexOnPublished(published, *static_cast<const TopoDS_Shape*>(occ));
    };

    const Part::HistoryTable raw = Part::SemanticHistoryAdapter::fromMaker(mkCylr, inputs, indexOf);
    Part::HistoryTable unique = Part::SemanticHistoryAdapter::uniqueOneImageGenerated(raw);
    // MakeCylinder has no Generated/Modified arguments. Unmodified survivor of
    // the live maker FACE/EDGE is the unique 1-image. If fromMaker is empty
    // (TShape miss / makePrism taper), bind already-located maker images
    // (I13 unnamed skip).
    if (unique.empty()) {
        for (const auto& pair : inputs) {
            if (!pair.second) {
                continue;
            }
            Part::HistoryRecord rec;
            rec.fromSeed = pair.first;
            rec.kind = App::EventKind::Generated;
            rec.toIndex
                = Part::indexOnPublished(published, *static_cast<const TopoDS_Shape*>(pair.second));
            if (rec.toIndex.type == "Edge") {
                rec.outputKind = App::SemanticKind::Edge;
            }
            else if (rec.toIndex.type == "Face") {
                rec.outputKind = App::SemanticKind::Face;
            }
            else {
                continue;
            }
            unique.push_back(rec);
        }
        unique = Part::SemanticHistoryAdapter::uniqueOneImageGenerated(unique);
    }

    // Bind the 0-to-1 AdditiveCylinder identities (fromSeed). Do not
    // applyHistory remint children — that would double-bind FaceN/EdgeN (I13).
    // allocatedBy is this AdditiveCylinder. Fillet.Base consumes the unique
    // Edge Binding. Skip bind if that index already has a unique Binding.
    std::size_t nBound = 0;
    for (const Part::HistoryRecord& rec : unique) {
        if (!Part::isNamedIndex(rec.toIndex) || !rec.fromSeed.valid()) {
            continue;
        }
        // bind() appends. Re-execute must not add a second row (I13).
        // I13: refuse remint-bind when slot is conflicted or already uniquely published.
        if (!App::shouldRefuseBoundAt(graph, selfId, rec.toIndex)) {
            SemanticEmitter::bind(graph, rec.fromSeed, selfId, eval, rec.toIndex);
        }
        ++nBound;
        if (rec.toIndex.type == "Face") {
            req.namedFaceIndices.push_back(rec.toIndex);
        }
        else if (rec.toIndex.type == "Edge") {
            req.namedEdgeIndices.push_back(rec.toIndex);
        }
    }
    SemanticEmitter::afterExecute(graph, Opcode::AdditiveCylinder, selfId, eval, req);
    SemanticEmitter::appendAfterExecuteNote(
        std::string("bound=") + std::to_string(nBound) + " named=" + std::to_string(nBound) + " unnamed=0"
    );
    Base::Console().message("TESTS cylDiag %s\n", SemanticEmitter::lastAfterExecuteNote().c_str());
}

App::DocumentObjectExecReturn* Cylinder::execute()
{
    // Build a cylinder
    if (Radius.getValue() < Precision::Confusion()) {
        return new App::DocumentObjectExecReturn(
            QT_TRANSLATE_NOOP("Exception", "Radius of cylinder too small")
        );
    }
    if (Height.getValue() < Precision::Confusion()) {
        return new App::DocumentObjectExecReturn(
            QT_TRANSLATE_NOOP("Exception", "Height of cylinder too small")
        );
    }
    if (Angle.getValue() < Precision::Confusion()) {
        return new App::DocumentObjectExecReturn(
            QT_TRANSLATE_NOOP("Exception", "Rotation angle of cylinder too small")
        );
    }
    try {
        // Keep the live maker. FeaturePrimitive::execute used to discard
        // BRepPrimAPI_MakeCylinder after Shape(); first-solid AdditiveCylinder
        // emit needs it. Zero-taper uses maker.Shape() so TShapes match the
        // published solid (Box clone). Non-zero First/SecondAngle keeps
        // makePrism; locate fallback then binds unique 1-image only (I13).
        BRepPrimAPI_MakeCylinder mkCylr(
            Radius.getValue(),
            Height.getValue(),
            Base::toRadians<double>(Angle.getValue())
        );

        TopoDS_Shape result;
        const double fa = FirstAngle.getValue();
        const double sa = SecondAngle.getValue();
        const bool taper = fa > Precision::Angular() || fa < -Precision::Angular()
            || sa > Precision::Angular() || sa < -Precision::Angular();
        if (taper) {
            BRepPrim_Cylinder prim = mkCylr.Cylinder();
            result = makePrism(Height.getValue(), prim.BottomFace());
        }
        else {
            result = mkCylr.Shape();
        }

        App::DocumentObjectExecReturn* ret = FeaturePrimitive::execute(result);
        if (ret == App::DocumentObject::StdReturn) {
            publishAdditiveCylinderSemanticHistory(&mkCylr);
        }
        return ret;
    }
    catch (Standard_Failure& e) {
        return new App::DocumentObjectExecReturn(e.GetMessageString());
    }

    return App::DocumentObject::StdReturn;
}

short int Cylinder::mustExecute() const
{
    if (Radius.isTouched() || Height.isTouched() || Angle.isTouched()) {
        return 1;
    }

    return FeaturePrimitive::mustExecute();
}

PROPERTY_SOURCE(PartDesign::AdditiveCylinder, PartDesign::Cylinder)
PROPERTY_SOURCE(PartDesign::SubtractiveCylinder, PartDesign::Cylinder)


PROPERTY_SOURCE(PartDesign::Sphere, PartDesign::FeaturePrimitive)

Sphere::Sphere()
{
    ADD_PROPERTY_TYPE(Radius, (5.0), "Sphere", App::Prop_None, "The radius of the sphere");
    Radius.setConstraints(&quantityRange);
    ADD_PROPERTY_TYPE(Angle1, (-90.0f), "Sphere", App::Prop_None, "The angle of the sphere");
    Angle1.setConstraints(&angleRangeV);
    ADD_PROPERTY_TYPE(Angle2, (90.0f), "Sphere", App::Prop_None, "The angle of the sphere");
    Angle2.setConstraints(&angleRangeV);
    ADD_PROPERTY_TYPE(Angle3, (360.0f), "Sphere", App::Prop_None, "The angle of the sphere");
    Angle3.setConstraints(&angleRangeU);

    primitiveType = FeaturePrimitive::Sphere;
}

void Sphere::onSubtractiveCutDone(
    BRepAlgoAPI_BooleanOperation* mkCut,
    const TopoDS_Shape& toolShape,
    const TopoDS_Shape& baseShape,
    App::DocumentObject* baseObj
)
{
    if (getAddSubType() != FeatureAddSub::Type::Subtractive) {
        return;
    }
    publishSubtractiveCutSemanticHistory(
        mkCut,
        toolShape,
        baseShape,
        baseObj,
        Opcode::SubtractiveSphere,
        "subSphereDiag"
    );
}

void Sphere::publishAdditiveSphereSemanticHistory(BRepPrimAPI_MakeSphere* mkSphere)
{
    // First-solid AdditiveSphere only; with-base subtractive uses the shared Cut publisher.
    if (!mkSphere || getAddSubType() != FeatureAddSub::Type::Additive) {
        return;
    }
    if (getBaseObject(/* silent = */ true)) {
        return;
    }
    const Part::TopoShape published = this->Shape.getShape();
    if (published.isNull()) {
        return;
    }
    App::SemanticGraph* graph = SemanticEmitter::graphFor(this);
    if (!graph && getDocument()) {
        graph = &getDocument()->semanticGraph();
    }
    if (!graph) {
        return;
    }

    const App::ObjectId selfId = static_cast<App::ObjectId>(getID());
    App::EvalSerial eval = 0;
    if (App::Document* doc = getDocument()) {
        eval = doc->semanticState().currentEval();
    }

    const std::vector<LocatedPrimitiveCandidate> candidates
        = uniqueLocatedPrimitiveCandidates(mkSphere->Shape(), published);
    std::deque<TopoDS_Shape> held;
    const std::vector<std::pair<App::SemanticId, const void*>> inputs = collectLocatedPrimitiveInputs(
        graph,
        selfId,
        eval,
        opcodeName(Opcode::AdditiveSphere),
        candidates,
        held
    );


    AfterExecuteRequest req;
    req.allowSequentialFaceN = false;
    if (inputs.empty()) {
        SemanticEmitter::afterExecute(graph, Opcode::AdditiveSphere, selfId, eval, req);
        SemanticEmitter::appendAfterExecuteNote("skip emit: no unique AdditiveSphere maker images");
        Base::Console().message(
            "TESTS sphereDiag %s\n",
            SemanticEmitter::lastAfterExecuteNote().c_str()
        );
        return;
    }

    auto indexOf = [&published](const void* occ) -> App::ElementIndex {
        App::ElementIndex idx;
        if (!occ) {
            return idx;
        }
        return Part::indexOnPublished(published, *static_cast<const TopoDS_Shape*>(occ));
    };

    const Part::HistoryTable raw = Part::SemanticHistoryAdapter::fromMaker(mkSphere, inputs, indexOf);
    Part::HistoryTable unique = Part::SemanticHistoryAdapter::uniqueOneImageGenerated(raw);
    // MakeSphere has no Generated/Modified arguments. Unmodified survivor of
    // the live maker FACE/EDGE is the unique 1-image. If fromMaker is empty
    // (TShape miss / seam+pole ambiguity), bind already-located maker images
    // (I13 unnamed skip).
    if (unique.empty()) {
        for (const auto& pair : inputs) {
            if (!pair.second) {
                continue;
            }
            Part::HistoryRecord rec;
            rec.fromSeed = pair.first;
            rec.kind = App::EventKind::Generated;
            rec.toIndex
                = Part::indexOnPublished(published, *static_cast<const TopoDS_Shape*>(pair.second));
            if (rec.toIndex.type == "Edge") {
                rec.outputKind = App::SemanticKind::Edge;
            }
            else if (rec.toIndex.type == "Face") {
                rec.outputKind = App::SemanticKind::Face;
            }
            else {
                continue;
            }
            unique.push_back(rec);
        }
        unique = Part::SemanticHistoryAdapter::uniqueOneImageGenerated(unique);
    }

    // Bind the 0-to-1 AdditiveSphere identities (fromSeed). Do not
    // applyHistory remint children — that would double-bind FaceN/EdgeN (I13).
    // allocatedBy is this AdditiveSphere. Fillet.Base consumes the unique
    // Edge Binding. Skip bind if that index already has a unique Binding.
    std::size_t nBound = 0;
    for (const Part::HistoryRecord& rec : unique) {
        if (!Part::isNamedIndex(rec.toIndex) || !rec.fromSeed.valid()) {
            continue;
        }
        // bind() appends. Re-execute must not add a second row (I13).
        // I13: refuse remint-bind when slot is conflicted or already uniquely published.
        if (!App::shouldRefuseBoundAt(graph, selfId, rec.toIndex)) {
            SemanticEmitter::bind(graph, rec.fromSeed, selfId, eval, rec.toIndex);
        }
        ++nBound;
        if (rec.toIndex.type == "Face") {
            req.namedFaceIndices.push_back(rec.toIndex);
        }
        else if (rec.toIndex.type == "Edge") {
            req.namedEdgeIndices.push_back(rec.toIndex);
        }
    }
    SemanticEmitter::afterExecute(graph, Opcode::AdditiveSphere, selfId, eval, req);
    SemanticEmitter::appendAfterExecuteNote(
        std::string("bound=") + std::to_string(nBound) + " named=" + std::to_string(nBound) + " unnamed=0"
    );
    Base::Console().message("TESTS sphereDiag %s\n", SemanticEmitter::lastAfterExecuteNote().c_str());
}

App::DocumentObjectExecReturn* Sphere::execute()
{
    // Build a sphere
    if (Radius.getValue() < Precision::Confusion()) {
        return new App::DocumentObjectExecReturn(
            QT_TRANSLATE_NOOP("Exception", "Radius of sphere too small")
        );
    }
    try {
        // Keep the live maker. FeaturePrimitive::execute used to discard
        // BRepPrimAPI_MakeSphere after Shape(); first-solid AdditiveSphere
        // emit needs it (Cylinder/Box clone). Meridian seam + poles may be
        // non-unique (I13); locate fallback binds unique 1-image only.
        BRepPrimAPI_MakeSphere mkSphere(
            Radius.getValue(),
            Base::toRadians<double>(Angle1.getValue()),
            Base::toRadians<double>(Angle2.getValue()),
            Base::toRadians<double>(Angle3.getValue())
        );
        App::DocumentObjectExecReturn* ret = FeaturePrimitive::execute(mkSphere.Shape());
        if (ret == App::DocumentObject::StdReturn) {
            publishAdditiveSphereSemanticHistory(&mkSphere);
        }
        return ret;
    }
    catch (Standard_Failure& e) {
        return new App::DocumentObjectExecReturn(e.GetMessageString());
    }

    return App::DocumentObject::StdReturn;
}

short int Sphere::mustExecute() const
{
    if (Radius.isTouched() || Angle1.isTouched() || Angle2.isTouched() || Angle3.isTouched()) {
        return 1;
    }

    return FeaturePrimitive::mustExecute();
}

PROPERTY_SOURCE(PartDesign::AdditiveSphere, PartDesign::Sphere)
PROPERTY_SOURCE(PartDesign::SubtractiveSphere, PartDesign::Sphere)


PROPERTY_SOURCE(PartDesign::Cone, PartDesign::FeaturePrimitive)

Cone::Cone()
{
    ADD_PROPERTY_TYPE(Radius1, (2.0), "Cone", App::Prop_None, "The radius of the cone");
    ADD_PROPERTY_TYPE(Radius2, (4.0), "Cone", App::Prop_None, "The radius of the cone");
    ADD_PROPERTY_TYPE(Height, (10.0), "Cone", App::Prop_None, "The height of the cone");
    ADD_PROPERTY_TYPE(Angle, (360.0), "Cone", App::Prop_None, "The angle of the cone");
    Angle.setConstraints(&angleRangeU);
    Radius1.setConstraints(&quantityRangeZero);
    Radius2.setConstraints(&quantityRangeZero);
    Height.setConstraints(&quantityRange);

    primitiveType = FeaturePrimitive::Cone;
}

void Cone::onSubtractiveCutDone(
    BRepAlgoAPI_BooleanOperation* mkCut,
    const TopoDS_Shape& toolShape,
    const TopoDS_Shape& baseShape,
    App::DocumentObject* baseObj
)
{
    if (getAddSubType() == FeatureAddSub::Type::Subtractive) {
        publishSubtractiveCutSemanticHistory(
            mkCut,
            toolShape,
            baseShape,
            baseObj,
            Opcode::SubtractiveCone,
            "subConeDiag"
        );
    }
}


void Cone::publishAdditiveConeSemanticHistory(BRepPrimAPI_MakeCone* mkCone)
{
    // First-solid AdditiveCone only. SubtractiveCone uses the shared Cut publisher.
    if (!mkCone || getAddSubType() != FeatureAddSub::Type::Additive) {
        return;
    }
    if (getBaseObject(/* silent = */ true)) {
        return;
    }
    const Part::TopoShape published = this->Shape.getShape();
    if (published.isNull()) {
        return;
    }
    App::SemanticGraph* graph = SemanticEmitter::graphFor(this);
    if (!graph && getDocument()) {
        graph = &getDocument()->semanticGraph();
    }
    if (!graph) {
        return;
    }

    const App::ObjectId selfId = static_cast<App::ObjectId>(getID());
    App::EvalSerial eval = 0;
    if (App::Document* doc = getDocument()) {
        eval = doc->semanticState().currentEval();
    }

    const std::vector<LocatedPrimitiveCandidate> candidates
        = uniqueLocatedPrimitiveCandidates(mkCone->Shape(), published);
    std::deque<TopoDS_Shape> held;
    const std::vector<std::pair<App::SemanticId, const void*>> inputs = collectLocatedPrimitiveInputs(
        graph,
        selfId,
        eval,
        opcodeName(Opcode::AdditiveCone),
        candidates,
        held
    );


    AfterExecuteRequest req;
    req.allowSequentialFaceN = false;
    if (inputs.empty()) {
        SemanticEmitter::afterExecute(graph, Opcode::AdditiveCone, selfId, eval, req);
        SemanticEmitter::appendAfterExecuteNote("skip emit: no unique AdditiveCone maker images");
        Base::Console().message("TESTS coneDiag %s\n", SemanticEmitter::lastAfterExecuteNote().c_str());
        return;
    }

    auto indexOf = [&published](const void* occ) -> App::ElementIndex {
        App::ElementIndex idx;
        if (!occ) {
            return idx;
        }
        return Part::indexOnPublished(published, *static_cast<const TopoDS_Shape*>(occ));
    };

    const Part::HistoryTable raw = Part::SemanticHistoryAdapter::fromMaker(mkCone, inputs, indexOf);
    Part::HistoryTable unique = Part::SemanticHistoryAdapter::uniqueOneImageGenerated(raw);
    // MakeCone may lack Generated/Modified arguments. Unmodified survivor of
    // the live maker FACE/EDGE is the unique 1-image. If fromMaker is empty
    // (TShape miss / seam ambiguity), bind already-located maker images
    // (I13 unnamed skip). Prefer unique base/top circular rims for Fillet.
    if (unique.empty()) {
        for (const auto& pair : inputs) {
            if (!pair.second) {
                continue;
            }
            Part::HistoryRecord rec;
            rec.fromSeed = pair.first;
            rec.kind = App::EventKind::Generated;
            rec.toIndex
                = Part::indexOnPublished(published, *static_cast<const TopoDS_Shape*>(pair.second));
            if (rec.toIndex.type == "Edge") {
                rec.outputKind = App::SemanticKind::Edge;
            }
            else if (rec.toIndex.type == "Face") {
                rec.outputKind = App::SemanticKind::Face;
            }
            else {
                continue;
            }
            unique.push_back(rec);
        }
        unique = Part::SemanticHistoryAdapter::uniqueOneImageGenerated(unique);
    }

    // Bind the 0-to-1 AdditiveCone identities (fromSeed). Do not
    // applyHistory remint children — that would double-bind FaceN/EdgeN (I13).
    // allocatedBy is this AdditiveCone. Fillet.Base consumes the unique
    // Edge Binding. Skip bind if that index already has a unique Binding.
    std::size_t nBound = 0;
    for (const Part::HistoryRecord& rec : unique) {
        if (!Part::isNamedIndex(rec.toIndex) || !rec.fromSeed.valid()) {
            continue;
        }
        // bind() appends. Re-execute must not add a second row (I13).
        // I13: refuse remint-bind when slot is conflicted or already uniquely published.
        if (!App::shouldRefuseBoundAt(graph, selfId, rec.toIndex)) {
            SemanticEmitter::bind(graph, rec.fromSeed, selfId, eval, rec.toIndex);
        }
        ++nBound;
        if (rec.toIndex.type == "Face") {
            req.namedFaceIndices.push_back(rec.toIndex);
        }
        else if (rec.toIndex.type == "Edge") {
            req.namedEdgeIndices.push_back(rec.toIndex);
        }
    }
    SemanticEmitter::afterExecute(graph, Opcode::AdditiveCone, selfId, eval, req);
    SemanticEmitter::appendAfterExecuteNote(
        std::string("bound=") + std::to_string(nBound) + " named=" + std::to_string(nBound) + " unnamed=0"
    );
    Base::Console().message("TESTS coneDiag %s\n", SemanticEmitter::lastAfterExecuteNote().c_str());
}

App::DocumentObjectExecReturn* Cone::execute()
{
    if (Radius1.getValue() < 0.0) {
        return new App::DocumentObjectExecReturn(
            QT_TRANSLATE_NOOP("Exception", "Radius of cone cannot be negative")
        );
    }
    if (Radius2.getValue() < 0.0) {
        return new App::DocumentObjectExecReturn(
            QT_TRANSLATE_NOOP("Exception", "Radius of cone cannot be negative")
        );
    }
    if (Height.getValue() < Precision::Confusion()) {
        return new App::DocumentObjectExecReturn(
            QT_TRANSLATE_NOOP("Exception", "Height of cone too small")
        );
    }
    try {
        if (std::abs(Radius1.getValue() - Radius2.getValue()) < Precision::Confusion()) {
            // Build a cylinder (equal radii). No MakeCone — AdditiveCone emit
            // stays silent on this path (truncated frustum uses MakeCone below).
            BRepPrimAPI_MakeCylinder mkCylr(
                Radius1.getValue(),
                Height.getValue(),
                Base::toRadians<double>(Angle.getValue())
            );
            return FeaturePrimitive::execute(mkCylr.Shape());
        }
        // Keep the live maker. FeaturePrimitive::execute used to discard
        // BRepPrimAPI_MakeCone after Shape(); first-solid AdditiveCone
        // emit needs it (Sphere/Cylinder/Box clone). Truncated cone
        // (Radius1 != Radius2) has unique base/top circular rims; 360°
        // side seam may be non-unique or not fillet-able (I13).
        BRepPrimAPI_MakeCone mkCone(
            Radius1.getValue(),
            Radius2.getValue(),
            Height.getValue(),
            Base::toRadians<double>(Angle.getValue())
        );
        App::DocumentObjectExecReturn* ret = FeaturePrimitive::execute(mkCone.Shape());
        if (ret == App::DocumentObject::StdReturn) {
            publishAdditiveConeSemanticHistory(&mkCone);
        }
        return ret;
    }
    catch (Standard_Failure& e) {
        return new App::DocumentObjectExecReturn(e.GetMessageString());
    }

    return App::DocumentObject::StdReturn;
}

short int Cone::mustExecute() const
{
    if (Radius1.isTouched()) {
        return 1;
    }
    if (Radius2.isTouched()) {
        return 1;
    }
    if (Height.isTouched()) {
        return 1;
    }
    if (Angle.isTouched()) {
        return 1;
    }
    return FeaturePrimitive::mustExecute();
}

PROPERTY_SOURCE(PartDesign::AdditiveCone, PartDesign::Cone)
PROPERTY_SOURCE(PartDesign::SubtractiveCone, PartDesign::Cone)

PROPERTY_SOURCE(PartDesign::Ellipsoid, PartDesign::FeaturePrimitive)

Ellipsoid::Ellipsoid()
{
    ADD_PROPERTY_TYPE(Radius1, (2.0), "Ellipsoid", App::Prop_None, "Radius in local Z-direction");
    Radius1.setConstraints(&quantityRange);
    ADD_PROPERTY_TYPE(Radius2, (4.0), "Ellipsoid", App::Prop_None, "Radius in local X-direction");
    Radius2.setConstraints(&quantityRange);
    ADD_PROPERTY_TYPE(
        Radius3,
        (0.0),
        "Ellipsoid",
        App::Prop_None,
        "Radius in local Y-direction\nIf zero, it is equal to Radius2"
    );
    Radius3.setConstraints(&quantityRangeZero);
    ADD_PROPERTY_TYPE(Angle1, (-90.0f), "Ellipsoid", App::Prop_None, "The angle of the ellipsoid");
    Angle1.setConstraints(&angleRangeV);
    ADD_PROPERTY_TYPE(Angle2, (90.0f), "Ellipsoid", App::Prop_None, "The angle of the ellipsoid");
    Angle2.setConstraints(&angleRangeV);
    ADD_PROPERTY_TYPE(Angle3, (360.0f), "Ellipsoid", App::Prop_None, "The angle of the ellipsoid");
    Angle3.setConstraints(&angleRangeU);

    primitiveType = FeaturePrimitive::Ellipsoid;
}

void Ellipsoid::onSubtractiveCutDone(
    BRepAlgoAPI_BooleanOperation* mkCut,
    const TopoDS_Shape& toolShape,
    const TopoDS_Shape& baseShape,
    App::DocumentObject* baseObj
)
{
    if (getAddSubType() == FeatureAddSub::Type::Subtractive) {
        publishSubtractiveCutSemanticHistory(
            mkCut,
            toolShape,
            baseShape,
            baseObj,
            Opcode::SubtractiveEllipsoid,
            "subEllipsoidDiag"
        );
    }
}

App::DocumentObjectExecReturn* Ellipsoid::execute()
{
    // Build a sphere, then scale to ellipsoid (GTransform).
    if (Radius1.getValue() < Precision::Confusion()) {
        return new App::DocumentObjectExecReturn(
            QT_TRANSLATE_NOOP("Exception", "Radius of ellipsoid too small")
        );
    }
    if (Radius2.getValue() < Precision::Confusion()) {
        return new App::DocumentObjectExecReturn(
            QT_TRANSLATE_NOOP("Exception", "Radius of ellipsoid too small")
        );
    }

    try {
        gp_Pnt pnt(0.0, 0.0, 0.0);
        gp_Dir dir(0.0, 0.0, 1.0);
        gp_Ax2 ax2(pnt, dir);
        // Keep live makers. MakeSphere alone is not the published solid —
        // GTransform scales it. First-solid AdditiveEllipsoid emit needs the
        // transformed solid for locate/bind (Wedge class; fromMaker on the
        // sphere maker would miss the published TShapes).
        BRepPrimAPI_MakeSphere mkSphere(
            ax2,
            Radius2.getValue(),
            Base::toRadians<double>(Angle1.getValue()),
            Base::toRadians<double>(Angle2.getValue()),
            Base::toRadians<double>(Angle3.getValue())
        );
        Standard_Real scaleX = 1.0;
        Standard_Real scaleZ = Radius1.getValue() / Radius2.getValue();
        // issue #1798: A third radius has been introduced. To be backward
        // compatible if Radius3 is 0.0 (default) it's handled to be the same
        // as Radius2
        Standard_Real scaleY = 1.0;
        if (Radius3.getValue() >= Precision::Confusion()) {
            scaleY = Radius3.getValue() / Radius2.getValue();
        }
        gp_GTrsf mat;
        mat.SetValue(1, 1, scaleX);
        mat.SetValue(2, 1, 0.0);
        mat.SetValue(3, 1, 0.0);
        mat.SetValue(1, 2, 0.0);
        mat.SetValue(2, 2, scaleY);
        mat.SetValue(3, 2, 0.0);
        mat.SetValue(1, 3, 0.0);
        mat.SetValue(2, 3, 0.0);
        mat.SetValue(3, 3, scaleZ);
        BRepBuilderAPI_GTransform mkTrsf(mkSphere.Shape(), mat);
        const TopoDS_Shape ellipsoidSolid = mkTrsf.Shape();
        App::DocumentObjectExecReturn* ret = FeaturePrimitive::execute(ellipsoidSolid);
        if (ret == App::DocumentObject::StdReturn) {
            publishAdditiveEllipsoidSemanticHistory(ellipsoidSolid);
        }
        return ret;
    }
    catch (Standard_Failure& e) {
        return new App::DocumentObjectExecReturn(e.GetMessageString());
    }

    return App::DocumentObject::StdReturn;
}

short int Ellipsoid::mustExecute() const
{
    if (Radius1.isTouched()) {
        return 1;
    }
    if (Radius2.isTouched()) {
        return 1;
    }
    if (Radius3.isTouched()) {
        return 1;
    }
    if (Angle1.isTouched()) {
        return 1;
    }
    if (Angle2.isTouched()) {
        return 1;
    }
    if (Angle3.isTouched()) {
        return 1;
    }

    return FeaturePrimitive::mustExecute();
}

void Ellipsoid::publishAdditiveEllipsoidSemanticHistory(const TopoDS_Shape& makerSolid)
{
    // First-solid AdditiveEllipsoid only. SubtractiveEllipsoid uses the shared Cut publisher.
    if (makerSolid.IsNull() || getAddSubType() != FeatureAddSub::Type::Additive) {
        return;
    }
    if (getBaseObject(/* silent = */ true)) {
        return;
    }
    const Part::TopoShape published = this->Shape.getShape();
    if (published.isNull()) {
        return;
    }
    App::SemanticGraph* graph = SemanticEmitter::graphFor(this);
    if (!graph && getDocument()) {
        graph = &getDocument()->semanticGraph();
    }
    if (!graph) {
        return;
    }

    const App::ObjectId selfId = static_cast<App::ObjectId>(getID());
    App::EvalSerial eval = 0;
    if (App::Document* doc = getDocument()) {
        eval = doc->semanticState().currentEval();
    }

    const std::vector<LocatedPrimitiveCandidate> candidates
        = uniqueLocatedPrimitiveCandidates(makerSolid, published);
    std::deque<TopoDS_Shape> held;
    const std::vector<std::pair<App::SemanticId, const void*>> inputs = collectLocatedPrimitiveInputs(
        graph,
        selfId,
        eval,
        opcodeName(Opcode::AdditiveEllipsoid),
        candidates,
        held
    );

    AfterExecuteRequest req;
    req.allowSequentialFaceN = false;
    // MakeSphere + GTransform: no reliable fromMaker Generated history. Bind
    // already-located maker images via shared collectLocatedPrimitiveInputs
    // (I13 unnamed skip). Hemisphere Angle2=0 rim is Fillet target —
    // circular/elliptical equator (Sphere pick class).
    if (inputs.empty()) {
        SemanticEmitter::afterExecute(graph, Opcode::AdditiveEllipsoid, selfId, eval, req);
        SemanticEmitter::appendAfterExecuteNote("skip emit: no unique AdditiveEllipsoid maker images");
        Base::Console().message(
            "TESTS ellipsoidDiag %s\n",
            SemanticEmitter::lastAfterExecuteNote().c_str()
        );
        return;
    }

    Part::HistoryTable unique;
    for (const auto& pair : inputs) {
        if (!pair.second) {
            continue;
        }
        Part::HistoryRecord rec;
        rec.fromSeed = pair.first;
        rec.kind = App::EventKind::Generated;
        rec.toIndex = Part::indexOnPublished(published, *static_cast<const TopoDS_Shape*>(pair.second));
        if (rec.toIndex.type == "Edge") {
            rec.outputKind = App::SemanticKind::Edge;
        }
        else if (rec.toIndex.type == "Face") {
            rec.outputKind = App::SemanticKind::Face;
        }
        else {
            continue;
        }
        unique.push_back(rec);
    }
    unique = Part::SemanticHistoryAdapter::uniqueOneImageGenerated(unique);

    // Bind the 0-to-1 AdditiveEllipsoid identities (fromSeed). Do not
    // applyHistory remint children — that would double-bind FaceN/EdgeN (I13).
    // allocatedBy is this AdditiveEllipsoid. Fillet.Base consumes the unique
    // Edge Binding. Skip bind if that index already has a unique Binding.
    std::size_t nBound = 0;
    for (const Part::HistoryRecord& rec : unique) {
        if (!Part::isNamedIndex(rec.toIndex) || !rec.fromSeed.valid()) {
            continue;
        }
        // bind() appends. Re-execute must not add a second row (I13).
        // I13: refuse remint-bind when slot is conflicted or already uniquely published.
        if (!App::shouldRefuseBoundAt(graph, selfId, rec.toIndex)) {
            SemanticEmitter::bind(graph, rec.fromSeed, selfId, eval, rec.toIndex);
        }
        ++nBound;
        if (rec.toIndex.type == "Face") {
            req.namedFaceIndices.push_back(rec.toIndex);
        }
        else if (rec.toIndex.type == "Edge") {
            req.namedEdgeIndices.push_back(rec.toIndex);
        }
    }
    SemanticEmitter::afterExecute(graph, Opcode::AdditiveEllipsoid, selfId, eval, req);
    SemanticEmitter::appendAfterExecuteNote(
        std::string("bound=") + std::to_string(nBound) + " named=" + std::to_string(nBound) + " unnamed=0"
    );
    Base::Console().message("TESTS ellipsoidDiag %s\n", SemanticEmitter::lastAfterExecuteNote().c_str());
}

PROPERTY_SOURCE(PartDesign::AdditiveEllipsoid, PartDesign::Ellipsoid)
PROPERTY_SOURCE(PartDesign::SubtractiveEllipsoid, PartDesign::Ellipsoid)


PROPERTY_SOURCE(PartDesign::Torus, PartDesign::FeaturePrimitive)

Torus::Torus()
{
    ADD_PROPERTY_TYPE(Radius1, (10.0), "Torus", App::Prop_None, "Radius in local XY-plane");
    Radius1.setConstraints(&quantityRange);
    ADD_PROPERTY_TYPE(Radius2, (2.0), "Torus", App::Prop_None, "Radius in local XZ-plane");
    Radius2.setConstraints(&quantityRange);
    ADD_PROPERTY_TYPE(Angle1, (-180.0), "Torus", App::Prop_None, "The angle of the torus");
    Angle1.setConstraints(&torusRangeV);
    ADD_PROPERTY_TYPE(Angle2, (180.0), "Torus", App::Prop_None, "The angle of the torus");
    Angle2.setConstraints(&torusRangeV);
    ADD_PROPERTY_TYPE(Angle3, (360.0), "Torus", App::Prop_None, "The angle of the torus");
    Angle3.setConstraints(&angleRangeU);

    primitiveType = FeaturePrimitive::Torus;
}

void Torus::onSubtractiveCutDone(
    BRepAlgoAPI_BooleanOperation* mkCut,
    const TopoDS_Shape& toolShape,
    const TopoDS_Shape& baseShape,
    App::DocumentObject* baseObj
)
{
    if (getAddSubType() == FeatureAddSub::Type::Subtractive) {
        publishSubtractiveCutSemanticHistory(
            mkCut,
            toolShape,
            baseShape,
            baseObj,
            Opcode::SubtractiveTorus,
            "subTorusDiag"
        );
    }
}

void Torus::publishAdditiveTorusSemanticHistory(BRepPrimAPI_MakeTorus* mkTorus)
{
    // First-solid AdditiveTorus only. SubtractiveTorus uses the shared Cut publisher.
    if (!mkTorus || getAddSubType() != FeatureAddSub::Type::Additive) {
        return;
    }
    if (getBaseObject(/* silent = */ true)) {
        return;
    }
    const Part::TopoShape published = this->Shape.getShape();
    if (published.isNull()) {
        return;
    }
    App::SemanticGraph* graph = SemanticEmitter::graphFor(this);
    if (!graph && getDocument()) {
        graph = &getDocument()->semanticGraph();
    }
    if (!graph) {
        return;
    }

    const App::ObjectId selfId = static_cast<App::ObjectId>(getID());
    App::EvalSerial eval = 0;
    if (App::Document* doc = getDocument()) {
        eval = doc->semanticState().currentEval();
    }

    const std::vector<LocatedPrimitiveCandidate> candidates
        = uniqueLocatedPrimitiveCandidates(mkTorus->Shape(), published);
    std::deque<TopoDS_Shape> held;
    const std::vector<std::pair<App::SemanticId, const void*>> inputs = collectLocatedPrimitiveInputs(
        graph,
        selfId,
        eval,
        opcodeName(Opcode::AdditiveTorus),
        candidates,
        held
    );


    AfterExecuteRequest req;
    req.allowSequentialFaceN = false;
    if (inputs.empty()) {
        SemanticEmitter::afterExecute(graph, Opcode::AdditiveTorus, selfId, eval, req);
        SemanticEmitter::appendAfterExecuteNote("skip emit: no unique AdditiveTorus maker images");
        Base::Console().message("TESTS torusDiag %s\n", SemanticEmitter::lastAfterExecuteNote().c_str());
        return;
    }

    auto indexOf = [&published](const void* occ) -> App::ElementIndex {
        App::ElementIndex idx;
        if (!occ) {
            return idx;
        }
        return Part::indexOnPublished(published, *static_cast<const TopoDS_Shape*>(occ));
    };

    const Part::HistoryTable raw = Part::SemanticHistoryAdapter::fromMaker(mkTorus, inputs, indexOf);
    Part::HistoryTable unique = Part::SemanticHistoryAdapter::uniqueOneImageGenerated(raw);
    // MakeTorus may lack Generated/Modified arguments. Unmodified survivor of
    // the live maker FACE/EDGE is the unique 1-image. If fromMaker is empty
    // (TShape miss / seam ambiguity), bind already-located maker images
    // (I13 unnamed skip). Prefer unique tube cross-section end circles for
    // Fillet (partial Angle3 < 360); meridian seam on full 360 may be non-unique
    // or not fillet-able (I13).
    if (unique.empty()) {
        for (const auto& pair : inputs) {
            if (!pair.second) {
                continue;
            }
            Part::HistoryRecord rec;
            rec.fromSeed = pair.first;
            rec.kind = App::EventKind::Generated;
            rec.toIndex
                = Part::indexOnPublished(published, *static_cast<const TopoDS_Shape*>(pair.second));
            if (rec.toIndex.type == "Edge") {
                rec.outputKind = App::SemanticKind::Edge;
            }
            else if (rec.toIndex.type == "Face") {
                rec.outputKind = App::SemanticKind::Face;
            }
            else {
                continue;
            }
            unique.push_back(rec);
        }
        unique = Part::SemanticHistoryAdapter::uniqueOneImageGenerated(unique);
    }

    // Bind the 0-to-1 AdditiveTorus identities (fromSeed). Do not
    // applyHistory remint children — that would double-bind FaceN/EdgeN (I13).
    // allocatedBy is this AdditiveTorus. Fillet.Base consumes the unique
    // Edge Binding. Skip bind if that index already has a unique Binding.
    std::size_t nBound = 0;
    for (const Part::HistoryRecord& rec : unique) {
        if (!Part::isNamedIndex(rec.toIndex) || !rec.fromSeed.valid()) {
            continue;
        }
        // bind() appends. Re-execute must not add a second row (I13).
        // I13: refuse remint-bind when slot is conflicted or already uniquely published.
        if (!App::shouldRefuseBoundAt(graph, selfId, rec.toIndex)) {
            SemanticEmitter::bind(graph, rec.fromSeed, selfId, eval, rec.toIndex);
        }
        ++nBound;
        if (rec.toIndex.type == "Face") {
            req.namedFaceIndices.push_back(rec.toIndex);
        }
        else if (rec.toIndex.type == "Edge") {
            req.namedEdgeIndices.push_back(rec.toIndex);
        }
    }
    SemanticEmitter::afterExecute(graph, Opcode::AdditiveTorus, selfId, eval, req);
    SemanticEmitter::appendAfterExecuteNote(
        std::string("bound=") + std::to_string(nBound) + " named=" + std::to_string(nBound) + " unnamed=0"
    );
    Base::Console().message("TESTS torusDiag %s\n", SemanticEmitter::lastAfterExecuteNote().c_str());
}

App::DocumentObjectExecReturn* Torus::execute()
{
    if (Radius1.getValue() < Precision::Confusion()) {
        return new App::DocumentObjectExecReturn(
            QT_TRANSLATE_NOOP("Exception", "Radius of torus too small")
        );
    }
    if (Radius2.getValue() < Precision::Confusion()) {
        return new App::DocumentObjectExecReturn(
            QT_TRANSLATE_NOOP("Exception", "Radius of torus too small")
        );
    }
    try {
        // Keep the live maker. FeaturePrimitive::execute used to discard
        // BRepPrimAPI_MakeTorus after Shape()/makeTorus; first-solid
        // AdditiveTorus emit needs it (Cone/Sphere/Cylinder/Box clone).
        // Partial Angle3 < 360 yields unique tube end-circle edges for
        // Fillet; 360° meridian seam may be non-unique or not fillet-able (I13).
        BRepPrimAPI_MakeTorus mkTorus(
            Radius1.getValue(),
            Radius2.getValue(),
            Base::toRadians<double>(Angle1.getValue()),
            Base::toRadians<double>(Angle2.getValue()),
            Base::toRadians<double>(Angle3.getValue())
        );
        App::DocumentObjectExecReturn* ret = FeaturePrimitive::execute(mkTorus.Shape());
        if (ret == App::DocumentObject::StdReturn) {
            publishAdditiveTorusSemanticHistory(&mkTorus);
        }
        return ret;
    }
    catch (Standard_Failure& e) {
        return new App::DocumentObjectExecReturn(e.GetMessageString());
    }

    return App::DocumentObject::StdReturn;
}

short int Torus::mustExecute() const
{
    if (Radius1.isTouched()) {
        return 1;
    }
    if (Radius2.isTouched()) {
        return 1;
    }
    if (Angle1.isTouched()) {
        return 1;
    }
    if (Angle2.isTouched()) {
        return 1;
    }
    if (Angle3.isTouched()) {
        return 1;
    }

    return FeaturePrimitive::mustExecute();
}

PROPERTY_SOURCE(PartDesign::AdditiveTorus, PartDesign::Torus)
PROPERTY_SOURCE(PartDesign::SubtractiveTorus, PartDesign::Torus)


PROPERTY_SOURCE(PartDesign::Prism, PartDesign::FeaturePrimitive)

Prism::Prism()
{
    ADD_PROPERTY_TYPE(
        Polygon,
        (6.0),
        "Prism",
        App::Prop_None,
        "Number of sides in the polygon, of the prism"
    );
    ADD_PROPERTY_TYPE(
        Circumradius,
        (2.0),
        "Prism",
        App::Prop_None,
        "Circumradius (centre to vertex) of the polygon, of the prism"
    );
    ADD_PROPERTY_TYPE(Height, (10.0f), "Prism", App::Prop_None, "The height of the prism");

    Part::PrismExtension::initExtension(this);

    primitiveType = FeaturePrimitive::Prism;
}

void Prism::onSubtractiveCutDone(
    BRepAlgoAPI_BooleanOperation* mkCut,
    const TopoDS_Shape& toolShape,
    const TopoDS_Shape& baseShape,
    App::DocumentObject* baseObj
)
{
    if (getAddSubType() == FeatureAddSub::Type::Subtractive) {
        publishSubtractiveCutSemanticHistory(
            mkCut,
            toolShape,
            baseShape,
            baseObj,
            Opcode::SubtractivePrism,
            "subPrismDiag"
        );
    }
}

void Prism::publishAdditivePrismSemanticHistory(BRepPrimAPI_MakePrism* mkPrism)
{
    // First-solid AdditivePrism only. SubtractivePrism uses the shared Cut publisher.
    if (!mkPrism || getAddSubType() != FeatureAddSub::Type::Additive) {
        return;
    }
    if (getBaseObject(/* silent = */ true)) {
        return;
    }
    const Part::TopoShape published = this->Shape.getShape();
    if (published.isNull()) {
        return;
    }
    App::SemanticGraph* graph = SemanticEmitter::graphFor(this);
    if (!graph && getDocument()) {
        graph = &getDocument()->semanticGraph();
    }
    if (!graph) {
        return;
    }

    const App::ObjectId selfId = static_cast<App::ObjectId>(getID());
    App::EvalSerial eval = 0;
    if (App::Document* doc = getDocument()) {
        eval = doc->semanticState().currentEval();
    }

    const std::vector<LocatedPrimitiveCandidate> candidates
        = uniqueLocatedPrimitiveCandidates(mkPrism->Shape(), published);
    std::deque<TopoDS_Shape> held;
    const std::vector<std::pair<App::SemanticId, const void*>> inputs = collectLocatedPrimitiveInputs(
        graph,
        selfId,
        eval,
        opcodeName(Opcode::AdditivePrism),
        candidates,
        held
    );


    AfterExecuteRequest req;
    req.allowSequentialFaceN = false;
    if (inputs.empty()) {
        SemanticEmitter::afterExecute(graph, Opcode::AdditivePrism, selfId, eval, req);
        SemanticEmitter::appendAfterExecuteNote("skip emit: no unique AdditivePrism maker images");
        Base::Console().message("TESTS prismDiag %s\n", SemanticEmitter::lastAfterExecuteNote().c_str());
        return;
    }

    auto indexOf = [&published](const void* occ) -> App::ElementIndex {
        App::ElementIndex idx;
        if (!occ) {
            return idx;
        }
        return Part::indexOnPublished(published, *static_cast<const TopoDS_Shape*>(occ));
    };

    const Part::HistoryTable raw = Part::SemanticHistoryAdapter::fromMaker(mkPrism, inputs, indexOf);
    Part::HistoryTable unique = Part::SemanticHistoryAdapter::uniqueOneImageGenerated(raw);
    // MakePrism may lack Generated/Modified arguments for solid FACE/EDGE.
    // Unmodified survivor of the live maker FACE/EDGE is the unique 1-image
    // (Box clone). If fromMaker is empty (TShape miss), bind already-located
    // maker images (I13 unnamed skip). Polygonal outer vertical edges are
    // Fillet targets — same pick_vertical_outer_edge class as AdditiveBox.
    // After unique-slot counting, fromMaker may uniquely keep Faces only;
    // always supplement uncovered locate Edge slots (unique.empty() full
    // fallback is a special case of the same helper).
    unique = Part::SemanticHistoryAdapter::supplementLocatedInputs(unique, inputs, indexOf);

    // Bind the 0-to-1 AdditivePrism identities (fromSeed). Do not
    // applyHistory remint children — that would double-bind FaceN/EdgeN (I13).
    // allocatedBy is this AdditivePrism. Fillet.Base consumes the unique
    // Edge Binding. Skip bind if that index already has a unique Binding.
    std::size_t nBound = 0;
    for (const Part::HistoryRecord& rec : unique) {
        if (!Part::isNamedIndex(rec.toIndex) || !rec.fromSeed.valid()) {
            continue;
        }
        // bind() appends. Re-execute must not add a second row (I13).
        // I13: refuse remint-bind when slot is conflicted or already uniquely published.
        if (!App::shouldRefuseBoundAt(graph, selfId, rec.toIndex)) {
            SemanticEmitter::bind(graph, rec.fromSeed, selfId, eval, rec.toIndex);
        }
        ++nBound;
        if (rec.toIndex.type == "Face") {
            req.namedFaceIndices.push_back(rec.toIndex);
        }
        else if (rec.toIndex.type == "Edge") {
            req.namedEdgeIndices.push_back(rec.toIndex);
        }
    }
    SemanticEmitter::afterExecute(graph, Opcode::AdditivePrism, selfId, eval, req);
    SemanticEmitter::appendAfterExecuteNote(
        std::string("bound=") + std::to_string(nBound) + " named=" + std::to_string(nBound) + " unnamed=0"
    );
    Base::Console().message("TESTS prismDiag %s\n", SemanticEmitter::lastAfterExecuteNote().c_str());
}

App::DocumentObjectExecReturn* Prism::execute()
{
    // Build a prism
    if (Polygon.getValue() < 3) {
        return new App::DocumentObjectExecReturn(
            QT_TRANSLATE_NOOP("Exception", "Polygon of prism is invalid, must have 3 or more sides")
        );
    }
    if (Circumradius.getValue() < Precision::Confusion()) {
        return new App::DocumentObjectExecReturn(
            QT_TRANSLATE_NOOP("Exception", "Circumradius of the polygon, of the prism, is too small")
        );
    }
    if (Height.getValue() < Precision::Confusion()) {
        return new App::DocumentObjectExecReturn(
            QT_TRANSLATE_NOOP("Exception", "Height of prism is too small")
        );
    }
    try {
        long nodes = Polygon.getValue();

        Base::Matrix4D mat;
        mat.rotZ(Base::toRadians(360.0 / nodes));

        // create polygon
        BRepBuilderAPI_MakePolygon mkPoly;
        Base::Vector3d v(Circumradius.getValue(), 0, 0);
        for (long i = 0; i < nodes; i++) {
            mkPoly.Add(gp_Pnt(v.x, v.y, v.z));
            v = mat * v;
        }
        mkPoly.Add(gp_Pnt(v.x, v.y, v.z));
        BRepBuilderAPI_MakeFace mkFace(mkPoly.Wire());
        // Keep the live maker. PrismExtension::makePrism used to discard
        // BRepPrimAPI_MakePrism after Shape(); first-solid AdditivePrism emit
        // needs it (Box polygonal clone — unique FACE/EDGE locate + bind).
        const double height = Height.getValue();
        BRepPrimAPI_MakePrism mkPrism(
            mkFace.Face(),
            gp_Vec(
                height * tan(Base::toRadians<double>(FirstAngle.getValue())),
                height * tan(Base::toRadians<double>(SecondAngle.getValue())),
                height
            )
        );
        App::DocumentObjectExecReturn* ret = FeaturePrimitive::execute(mkPrism.Shape());
        if (ret == App::DocumentObject::StdReturn) {
            publishAdditivePrismSemanticHistory(&mkPrism);
        }
        return ret;
    }
    catch (Standard_Failure& e) {
        return new App::DocumentObjectExecReturn(e.GetMessageString());
    }

    return App::DocumentObject::StdReturn;
}

short int Prism::mustExecute() const
{
    if (Polygon.isTouched()) {
        return 1;
    }
    if (Circumradius.isTouched()) {
        return 1;
    }
    if (Height.isTouched()) {
        return 1;
    }

    return FeaturePrimitive::mustExecute();
}

PROPERTY_SOURCE(PartDesign::AdditivePrism, PartDesign::Prism)
PROPERTY_SOURCE(PartDesign::SubtractivePrism, PartDesign::Prism)


PROPERTY_SOURCE(PartDesign::Wedge, PartDesign::FeaturePrimitive)

Wedge::Wedge()
{
    ADD_PROPERTY_TYPE(Xmin, (0.0f), "Wedge", App::Prop_None, "Xmin of the wedge");
    ADD_PROPERTY_TYPE(Ymin, (0.0f), "Wedge", App::Prop_None, "Ymin of the wedge");
    ADD_PROPERTY_TYPE(Zmin, (0.0f), "Wedge", App::Prop_None, "Zmin of the wedge");
    ADD_PROPERTY_TYPE(X2min, (2.0f), "Wedge", App::Prop_None, "X2min of the wedge");
    ADD_PROPERTY_TYPE(Z2min, (2.0f), "Wedge", App::Prop_None, "Z2min of the wedge");
    ADD_PROPERTY_TYPE(Xmax, (10.0f), "Wedge", App::Prop_None, "Xmax of the wedge");
    ADD_PROPERTY_TYPE(Ymax, (10.0f), "Wedge", App::Prop_None, "Ymax of the wedge");
    ADD_PROPERTY_TYPE(Zmax, (10.0f), "Wedge", App::Prop_None, "Zmax of the wedge");
    ADD_PROPERTY_TYPE(X2max, (8.0f), "Wedge", App::Prop_None, "X2max of the wedge");
    ADD_PROPERTY_TYPE(Z2max, (8.0f), "Wedge", App::Prop_None, "Z2max of the wedge");

    primitiveType = FeaturePrimitive::Wedge;
}

void Wedge::onSubtractiveCutDone(
    BRepAlgoAPI_BooleanOperation* mkCut,
    const TopoDS_Shape& toolShape,
    const TopoDS_Shape& baseShape,
    App::DocumentObject* baseObj
)
{
    if (getAddSubType() == FeatureAddSub::Type::Subtractive) {
        publishSubtractiveCutSemanticHistory(
            mkCut,
            toolShape,
            baseShape,
            baseObj,
            Opcode::SubtractiveWedge,
            "subWedgeDiag"
        );
    }
}

App::DocumentObjectExecReturn* Wedge::execute()
{
    double xmin = Xmin.getValue();
    double ymin = Ymin.getValue();
    double zmin = Zmin.getValue();
    double z2min = Z2min.getValue();
    double x2min = X2min.getValue();
    double xmax = Xmax.getValue();
    double ymax = Ymax.getValue();
    double zmax = Zmax.getValue();
    double z2max = Z2max.getValue();
    double x2max = X2max.getValue();

    double dx = xmax - xmin;
    double dy = ymax - ymin;
    double dz = zmax - zmin;
    double dz2 = z2max - z2min;
    double dx2 = x2max - x2min;

    if (dx < Precision::Confusion()) {
        return new App::DocumentObjectExecReturn(
            QT_TRANSLATE_NOOP("Exception", "delta x of wedge too small")
        );
    }

    if (dy < Precision::Confusion()) {
        return new App::DocumentObjectExecReturn(
            QT_TRANSLATE_NOOP("Exception", "delta y of wedge too small")
        );
    }

    if (dz < Precision::Confusion()) {
        return new App::DocumentObjectExecReturn(
            QT_TRANSLATE_NOOP("Exception", "delta z of wedge too small")
        );
    }

    if (dz2 < 0) {
        return new App::DocumentObjectExecReturn(
            QT_TRANSLATE_NOOP("Exception", "delta z2 of wedge is negative")
        );
    }

    if (dx2 < 0) {
        return new App::DocumentObjectExecReturn(
            QT_TRANSLATE_NOOP("Exception", "delta x2 of wedge is negative")
        );
    }

    try {
        gp_Pnt pnt(0.0, 0.0, 0.0);
        gp_Dir dir(0.0, 0.0, 1.0);
        // Keep the live primitive. BRepPrimAPI_MakeWedge only exposes STEP
        // wedge (4/8 args), not FreeCAD's general 11-parameter wedge.
        // Publish uses unique FACE/EDGE locate + bind on the maker solid
        // (Box/Prism fallback when fromMaker is empty).
        BRepPrim_Wedge
            mkWedge(gp_Ax2(pnt, dir), xmin, ymin, zmin, z2min, x2min, xmax, ymax, zmax, z2max, x2max);
        BRepBuilderAPI_MakeSolid mkSolid;
        mkSolid.Add(mkWedge.Shell());
        if (!mkSolid.IsDone()) {
            return new App::DocumentObjectExecReturn(
                QT_TRANSLATE_NOOP("Exception", "Failed to build wedge solid")
            );
        }
        const TopoDS_Solid wedgeSolid = mkSolid.Solid();
        App::DocumentObjectExecReturn* ret = FeaturePrimitive::execute(wedgeSolid);
        if (ret == App::DocumentObject::StdReturn) {
            publishAdditiveWedgeSemanticHistory(wedgeSolid);
        }
        return ret;
    }
    catch (Standard_Failure& e) {
        return new App::DocumentObjectExecReturn(e.GetMessageString());
    }

    return App::DocumentObject::StdReturn;
}

short int Wedge::mustExecute() const
{
    if (Xmin.isTouched() || Ymin.isTouched() || Zmin.isTouched() || X2min.isTouched()
        || Z2min.isTouched() || Xmax.isTouched() || Ymax.isTouched() || Zmax.isTouched()
        || X2max.isTouched() || Z2max.isTouched()) {
        return 1;
    }

    return FeaturePrimitive::mustExecute();
}

void Wedge::publishAdditiveWedgeSemanticHistory(const TopoDS_Shape& makerSolid)
{
    // First-solid AdditiveWedge only. SubtractiveWedge uses the shared Cut publisher.
    if (makerSolid.IsNull() || getAddSubType() != FeatureAddSub::Type::Additive) {
        return;
    }
    if (getBaseObject(/* silent = */ true)) {
        return;
    }
    const Part::TopoShape published = this->Shape.getShape();
    if (published.isNull()) {
        return;
    }
    App::SemanticGraph* graph = SemanticEmitter::graphFor(this);
    if (!graph && getDocument()) {
        graph = &getDocument()->semanticGraph();
    }
    if (!graph) {
        return;
    }

    const App::ObjectId selfId = static_cast<App::ObjectId>(getID());
    App::EvalSerial eval = 0;
    if (App::Document* doc = getDocument()) {
        eval = doc->semanticState().currentEval();
    }

    const std::vector<LocatedPrimitiveCandidate> candidates
        = uniqueLocatedPrimitiveCandidates(makerSolid, published);
    std::deque<TopoDS_Shape> held;
    const std::vector<std::pair<App::SemanticId, const void*>> inputs = collectLocatedPrimitiveInputs(
        graph,
        selfId,
        eval,
        opcodeName(Opcode::AdditiveWedge),
        candidates,
        held
    );

    AfterExecuteRequest req;
    req.allowSequentialFaceN = false;
    // BRepPrim_Wedge has no BRepBuilderAPI_MakeShape history. Bind
    // already-located maker images via shared collectLocatedPrimitiveInputs
    // (I13 unnamed skip). Vertical outer edges are Fillet targets — same
    // pick_vertical_outer_edge class as Box/Prism.
    if (inputs.empty()) {
        SemanticEmitter::afterExecute(graph, Opcode::AdditiveWedge, selfId, eval, req);
        SemanticEmitter::appendAfterExecuteNote("skip emit: no unique AdditiveWedge maker images");
        Base::Console().message("TESTS wedgeDiag %s\n", SemanticEmitter::lastAfterExecuteNote().c_str());
        return;
    }

    Part::HistoryTable unique;
    for (const auto& pair : inputs) {
        if (!pair.second) {
            continue;
        }
        Part::HistoryRecord rec;
        rec.fromSeed = pair.first;
        rec.kind = App::EventKind::Generated;
        rec.toIndex = Part::indexOnPublished(published, *static_cast<const TopoDS_Shape*>(pair.second));
        if (rec.toIndex.type == "Edge") {
            rec.outputKind = App::SemanticKind::Edge;
        }
        else if (rec.toIndex.type == "Face") {
            rec.outputKind = App::SemanticKind::Face;
        }
        else {
            continue;
        }
        unique.push_back(rec);
    }
    unique = Part::SemanticHistoryAdapter::uniqueOneImageGenerated(unique);

    // Bind the 0-to-1 AdditiveWedge identities (fromSeed). Do not
    // applyHistory remint children — that would double-bind FaceN/EdgeN (I13).
    // allocatedBy is this AdditiveWedge. Fillet.Base consumes the unique
    // Edge Binding. Skip bind if that index already has a unique Binding.
    std::size_t nBound = 0;
    for (const Part::HistoryRecord& rec : unique) {
        if (!Part::isNamedIndex(rec.toIndex) || !rec.fromSeed.valid()) {
            continue;
        }
        // bind() appends. Re-execute must not add a second row (I13).
        // I13: refuse remint-bind when slot is conflicted or already uniquely published.
        if (!App::shouldRefuseBoundAt(graph, selfId, rec.toIndex)) {
            SemanticEmitter::bind(graph, rec.fromSeed, selfId, eval, rec.toIndex);
        }
        ++nBound;
        if (rec.toIndex.type == "Face") {
            req.namedFaceIndices.push_back(rec.toIndex);
        }
        else if (rec.toIndex.type == "Edge") {
            req.namedEdgeIndices.push_back(rec.toIndex);
        }
    }
    SemanticEmitter::afterExecute(graph, Opcode::AdditiveWedge, selfId, eval, req);
    SemanticEmitter::appendAfterExecuteNote(
        std::string("bound=") + std::to_string(nBound) + " named=" + std::to_string(nBound) + " unnamed=0"
    );
    Base::Console().message("TESTS wedgeDiag %s\n", SemanticEmitter::lastAfterExecuteNote().c_str());
}

PROPERTY_SOURCE(PartDesign::AdditiveWedge, PartDesign::Wedge)
PROPERTY_SOURCE(PartDesign::SubtractiveWedge, PartDesign::Wedge)
}  // namespace PartDesign
