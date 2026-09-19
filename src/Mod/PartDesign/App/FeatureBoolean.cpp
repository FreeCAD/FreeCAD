// SPDX-License-Identifier: LGPL-2.1-or-later

/******************************************************************************
 *   Copyright (c) 2013 Jan Rheinländer <jrheinlaender@users.sourceforge.net> *
 *                                                                            *
 *   This file is part of the FreeCAD CAx development system.                 *
 *                                                                            *
 *   This library is free software; you can redistribute it and/or            *
 *   modify it under the terms of the GNU Library General Public              *
 *   License as published by the Free Software Foundation; either             *
 *   version 2 of the License, or (at your option) any later version.         *
 *                                                                            *
 *   This library  is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *   GNU Library General Public License for more details.                     *
 *                                                                            *
 *   You should have received a copy of the GNU Library General Public        *
 *   License along with this library; see the file COPYING.LIB. If not,       *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,            *
 *   Suite 330, Boston, MA  02111-1307, USA                                   *
 *                                                                            *
 ******************************************************************************/


#include <Mod/Part/App/FCBRepAlgoAPI_Common.h>
#include <Mod/Part/App/FCBRepAlgoAPI_Cut.h>
#include <Mod/Part/App/FCBRepAlgoAPI_Fuse.h>
#include <Standard_Failure.hxx>

#include <algorithm>
#include <deque>
#include <memory>
#include <optional>
#include <unordered_set>
#include <vector>

#include <BRepAdaptor_Surface.hxx>
#include <BRepAlgoAPI_BooleanOperation.hxx>
#include <BRepBndLib.hxx>
#include <BRep_Tool.hxx>
#include <Bnd_Box.hxx>
#include <Precision.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_ListOfShape.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Vertex.hxx>
#include <gp_Pln.hxx>
#include <gp_Pnt.hxx>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/SemanticDocumentState.h>
#include <App/SemanticReference.h>
#include <Base/Console.h>
#include <Base/Tools.h>
#include <Mod/Part/App/FuzzyHelper.h>
#include <Mod/Part/App/modelRefine.h>
#include <Mod/Part/App/SemanticHistoryAdapter.h>
#include <Mod/Part/App/SemanticSourceCollector.h>
#include <Mod/Part/App/TopoShapeOpCode.h>

#include "FeatureBoolean.h"
#include "Body.h"
#include "SemanticOpcode.h"

FC_LOG_LEVEL_INIT("PartDesign", true, true);

using namespace PartDesign;

namespace PartDesign
{
extern bool getPDRefineModelParameter();

namespace
{

/// PartDesign cannot link FCBRepAlgoAPI_BooleanOperation::setAutoFuzzy (LNK2019).
/// Mirror FCBRepAlgoAPIHelper via exported FuzzyHelper + OCCT SetFuzzyValue.
void setFuseAutoFuzzy(
    FCBRepAlgoAPI_Fuse* fcFuse,
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
    fcFuse->SetFuzzyValue(
        Part::FuzzyHelper::getBooleanFuzzy() * sqrt(bounds.SquareExtent()) * Precision::Confusion()
    );
}

/// Same LNK2019 lesson as Fuse: do not call FCBRepAlgoAPIHelper from PartDesign.
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

/// Same LNK2019 lesson as Fuse/Cut: do not call FCBRepAlgoAPIHelper from PartDesign.
void setCommonAutoFuzzy(
    FCBRepAlgoAPI_Common* fcCommon,
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
    fcCommon->SetFuzzyValue(
        Part::FuzzyHelper::getBooleanFuzzy() * sqrt(bounds.SquareExtent()) * Precision::Confusion()
    );
}


/// C1 reuse / I13 source seeds: Part::collectUniqueSourceSeeds (shared
/// Boolean / Primitive / Transformed collector). Body Tip is walked when
/// `obj` is a Body so Tip Face/Edge Bindings reach maker inputs.
void collectFromObject(
    App::SemanticGraph* graph,
    App::DocumentObject* obj,
    const Part::TopoShape& sourceShape,
    std::deque<TopoDS_Shape>& held,
    std::vector<std::pair<App::SemanticId, const void*>>& inputs,
    std::unordered_set<App::SemanticHandle>& seen
)
{
    if (!obj) {
        return;
    }
    Part::collectUniqueSourceSeeds(graph, obj, sourceShape, held, inputs, seen);
    if (auto* body = freecad_cast<PartDesign::Body*>(obj)) {
        Part::collectUniqueSourceSeeds(graph, body->Tip.getValue(), sourceShape, held, inputs, seen);
    }
}

}  // namespace


PROPERTY_SOURCE_WITH_EXTENSIONS(PartDesign::Boolean, PartDesign::FeatureRefine)

const char* Boolean::TypeEnums[] = {"Fuse", "Cut", "Common", nullptr};

Boolean::Boolean()
{
    ADD_PROPERTY(Type, ((long)0));
    Type.setEnums(TypeEnums);
    ADD_PROPERTY_TYPE(
        UseLegacyBodyPlacement,
        (App::GetApplication().isRestoring()),
        "Compatibility",
        App::Prop_Hidden,
        "Use legacy PartDesign Boolean body placement handling"
    );

    App::GeoFeatureGroupExtension::initExtension(this);
    // Boolean tools are references, not owned coordinate-system children.
    Group.setScope(App::LinkScope::Global);
}

short Boolean::mustExecute() const
{
    if (Group.isTouched() || UseLegacyBodyPlacement.isTouched()) {
        return 1;
    }
    return PartDesign::Feature::mustExecute();
}

TopoShape Boolean::getBooleanTopoShape(const App::DocumentObject* object) const
{
    if (UseLegacyBodyPlacement.getValue()) {
        return getTopoShape(object, Part::ShapeOption::ResolveLink | Part::ShapeOption::Transform);
    }
    return getTopoShapeInLocalCoordinates(object);
}

std::vector<App::DocumentObject*> Boolean::addObject(App::DocumentObject* object)
{
    return addObjects({object});
}

std::vector<App::DocumentObject*> Boolean::addObjects(std::vector<App::DocumentObject*> objects)
{
    auto tools = Group.getValues();
    std::vector<App::DocumentObject*> added;

    for (auto object : objects) {
        if (!object || std::ranges::find(tools, object) != tools.end()) {
            continue;
        }

        tools.push_back(object);
        added.push_back(object);
    }

    Base::ObjectStatusLocker<App::Property::Status, App::Property> guard(App::Property::User3, &Group);
    Group.setValues(tools);
    return added;
}

std::vector<App::DocumentObject*> Boolean::setObjects(std::vector<App::DocumentObject*> objects)
{
    std::vector<App::DocumentObject*> tools;
    std::vector<App::DocumentObject*> added;

    for (auto object : objects) {
        if (!object || std::ranges::find(tools, object) != tools.end()) {
            continue;
        }

        tools.push_back(object);
        added.push_back(object);
    }

    Base::ObjectStatusLocker<App::Property::Status, App::Property> guard(App::Property::User3, &Group);
    Group.setValues(tools);
    return added;
}

std::vector<App::DocumentObject*> Boolean::removeObject(App::DocumentObject* object)
{
    return removeObjects({object});
}

std::vector<App::DocumentObject*> Boolean::removeObjects(std::vector<App::DocumentObject*> objects)
{
    auto tools = Group.getValues();
    std::vector<App::DocumentObject*> removed;

    for (auto object : objects) {
        auto it = std::ranges::find(tools, object);
        if (it == tools.end()) {
            continue;
        }

        removed.push_back(object);
        tools.erase(it);
    }

    Base::ObjectStatusLocker<App::Property::Status, App::Property> guard(App::Property::User3, &Group);
    Group.setValues(tools);
    return removed;
}

bool Boolean::hasObject(const App::DocumentObject* /*object*/, bool /*recursive*/) const
{
    // Boolean tools are references, not owned children. Returning false keeps
    // GeoFeatureGroupExtension from using this object as their coordinate system.
    return false;
}


void Boolean::publishBooleanSemanticHistory(
    BRepAlgoAPI_BooleanOperation* mkBool,
    const Part::TopoShape& published,
    App::DocumentObject* baseObj,
    const Part::TopoShape& baseShape,
    const std::vector<App::DocumentObject*>& toolObjs,
    const std::vector<Part::TopoShape>& toolShapes,
    const char* diagTag
)
{
    if (!mkBool || !mkBool->IsDone() || published.isNull()) {
        return;
    }
    App::SemanticGraph* graph = SemanticEmitter::graphFor(this);
    if (!graph && getDocument()) {
        graph = &getDocument()->semanticGraph();
    }
    if (!graph) {
        return;
    }

    std::deque<TopoDS_Shape> held;
    std::vector<std::pair<App::SemanticId, const void*>> inputs;
    std::unordered_set<App::SemanticHandle> seen;
    collectFromObject(graph, baseObj, baseShape, held, inputs, seen);
    const std::size_t nTools = std::min(toolObjs.size(), toolShapes.size());
    for (std::size_t i = 0; i < nTools; ++i) {
        collectFromObject(graph, toolObjs[i], toolShapes[i], held, inputs, seen);
    }
    if (inputs.empty()) {
        SemanticEmitter::afterExecute(graph, Opcode::Boolean, static_cast<App::ObjectId>(getID()), 0, {});
        SemanticEmitter::appendAfterExecuteNote("skip emit: no unique Boolean source seeds");
        Base::Console().message(
            "TESTS %s %s\n",
            diagTag ? diagTag : "booleanDiag",
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

    const Part::HistoryTable raw = Part::SemanticHistoryAdapter::fromMaker(mkBool, inputs, indexOf);
    const Part::HistoryTable unique = Part::SemanticHistoryAdapter::uniqueOneImageGenerated(raw);

    const App::ObjectId selfId = static_cast<App::ObjectId>(getID());
    Part::HistoryTable toApply;
    std::vector<App::SemanticId> seeds;
    AfterExecuteRequest req;
    req.allowSequentialFaceN = false;
    toApply.reserve(unique.size());
    seeds.reserve(unique.size());
    for (const Part::HistoryRecord& rec : unique) {
        // Match Part::Boolean / Extrusion/Revolution/Mirroring: refuse unnamed
        // slots before apply (applyHistory would skip; seeds must not orphan).
        if (!Part::isNamedIndex(rec.toIndex)) {
            continue;
        }
        // I13 leave-unnamed / C1: already bound, conflicted slot, or unique owner.
        if (App::shouldRefuseGeneratedMint(graph, rec.fromSeed, selfId, rec.toIndex)) {
            continue;
        }
        toApply.push_back(rec);
        seeds.push_back(rec.fromSeed);
        if (rec.toIndex.type == "Face") {
            req.namedFaceIndices.push_back(rec.toIndex);
        }
        else if (rec.toIndex.type == "Edge") {
            req.namedEdgeIndices.push_back(rec.toIndex);
        }
    }

    App::EvalSerial eval = 0;
    if (App::Document* doc = getDocument()) {
        eval = doc->semanticState().currentEval();
    }
    if (!toApply.empty()) {
        Part::SemanticHistoryAdapter::applyHistory(
            graph,
            selfId,
            eval,
            opcodeName(Opcode::Boolean),
            seeds,
            toApply
        );
    }
    SemanticEmitter::afterExecute(graph, Opcode::Boolean, selfId, eval, req);
    SemanticEmitter::appendAfterExecuteNote(Part::SemanticHistoryAdapter::lastApplyNote());
    Base::Console().message(
        "TESTS %s %s\n",
        diagTag ? diagTag : "booleanDiag",
        SemanticEmitter::lastAfterExecuteNote().c_str()
    );
}

App::DocumentObjectExecReturn* Boolean::execute()
{
    // Get the operation type
    std::string type = Type.getValueAsString();

    // Check the parameters
    const Part::Feature* baseFeature = this->getBaseObject(/* silent = */ true);

    if (!baseFeature && type == "Cut") {
        return new App::DocumentObjectExecReturn(
            QT_TRANSLATE_NOOP("Exception", "Cannot do boolean cut without BaseFeature")
        );
    }

    std::vector<App::DocumentObject*> tools = Group.getValues();
    if (tools.empty()) {
        return App::DocumentObject::StdReturn;
    }

    // Get the base shape to operate on
    App::DocumentObject* baseObj = nullptr;
    Part::TopoShape baseTopShape;
    if (baseFeature) {
        baseObj = const_cast<Part::Feature*>(baseFeature);
        if (UseLegacyBodyPlacement.getValue()) {
            baseTopShape = baseFeature->Shape.getShape();
        }
        else {
            baseTopShape = getBooleanTopoShape(baseFeature);
        }
    }
    else {
        auto feature = tools.back();
        if (!feature->isDerivedFrom<Part::Feature>()) {
            return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP(
                "Exception",
                "Cannot do boolean with anything but Part::Feature and its derivatives"
            ));
        }

        baseObj = feature;
        if (UseLegacyBodyPlacement.getValue()) {
            baseTopShape = static_cast<Part::Feature*>(feature)->Shape.getShape();
        }
        else {
            baseTopShape = getBooleanTopoShape(feature);
        }
        tools.pop_back();
    }

    if (baseTopShape.getShape().IsNull()) {
        return new App::DocumentObjectExecReturn(
            QT_TRANSLATE_NOOP("Exception", "Cannot do boolean operation with invalid base shape")
        );
    }

    std::vector<TopoShape> shapes;
    shapes.push_back(baseTopShape);
    for (auto it = tools.begin(); it < tools.end(); ++it) {
        auto shape = getBooleanTopoShape(*it);
        if (shape.isNull()) {
            return new App::DocumentObjectExecReturn(
                QT_TRANSLATE_NOOP("Exception", "Tool shape is null")
            );
        }
        shapes.push_back(shape);
    }
    TopoShape result(baseTopShape);
    std::unique_ptr<BRepAlgoAPI_BooleanOperation> mkBool;

    if (!tools.empty()) {
        const char* op = nullptr;

        if (type == "Fuse") {
            op = Part::OpCodes::Fuse;
        }
        else if (type == "Cut") {
            op = Part::OpCodes::Cut;
        }
        else if (type == "Common") {
            op = Part::OpCodes::Common;
        }
        // LinkStage3 defines these other types of Boolean operations.  Removed for now pending
        // decision to bring them in or not.
        // else if(type == "Compound")
        //     op = Part::OpCodes::Compound;
        // else if(type == "Section")
        //     op = Part::OpCodes::Section;
        else {
            return new App::DocumentObjectExecReturn(
                QT_TRANSLATE_NOOP("Exception", "Unsupported boolean operation")
            );
        }

        try {
            if (type == "Fuse") {
                // Live FCBRepAlgoAPI_Fuse so fromMaker TShapes match the published
                // solid (Part Boolean / Transformed clone). Never FCBRepAlgoAPIHelper
                // from PartDesign (LNK2019).
                auto* fcFuse = new FCBRepAlgoAPI_Fuse;
                mkBool.reset(fcFuse);
                TopTools_ListOfShape args;
                TopTools_ListOfShape toolList;
                args.Append(shapes[0].getShape());
                for (std::size_t i = 1; i < shapes.size(); ++i) {
                    toolList.Append(shapes[i].getShape());
                }
                if (args.IsEmpty() || toolList.IsEmpty()) {
                    mkBool.reset();
                }
                else {
                    fcFuse->SetArguments(args);
                    fcFuse->SetTools(toolList);
                    const double fuzzy = FuzzyTolerance.getValue();
                    if (fuzzy > 0.0) {
                        fcFuse->SetFuzzyValue(fuzzy);
                    }
                    else {
                        setFuseAutoFuzzy(fcFuse, args, toolList);
                    }
                    fcFuse->Build();
                    if (!fcFuse->IsDone()) {
                        FC_ERR("Boolean Fuse maker failed");
                        return new App::DocumentObjectExecReturn(
                            QT_TRANSLATE_NOOP("Exception", "Boolean operation failed")
                        );
                    }
                    result.makeElementShape(*fcFuse, shapes, op);
                }
            }
            else if (type == "Cut") {
                // Live FCBRepAlgoAPI_Cut clone of Fuse path. Local setCutAutoFuzzy
                // (LNK2019 — never FCBRepAlgoAPIHelper).
                auto* fcCut = new FCBRepAlgoAPI_Cut;
                mkBool.reset(fcCut);
                TopTools_ListOfShape args;
                TopTools_ListOfShape toolList;
                args.Append(shapes[0].getShape());
                for (std::size_t i = 1; i < shapes.size(); ++i) {
                    toolList.Append(shapes[i].getShape());
                }
                if (args.IsEmpty() || toolList.IsEmpty()) {
                    mkBool.reset();
                }
                else {
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
                        FC_ERR("Boolean Cut maker failed");
                        return new App::DocumentObjectExecReturn(
                            QT_TRANSLATE_NOOP("Exception", "Boolean operation failed")
                        );
                    }
                    result.makeElementShape(*fcCut, shapes, op);
                }
            }
            else if (type == "Common") {
                // Live FCBRepAlgoAPI_Common clone of Cut/Fuse. Local setCommonAutoFuzzy
                // (LNK2019 — never FCBRepAlgoAPIHelper). Reuses Opcode::Boolean=15 PBF.
                auto* fcCommon = new FCBRepAlgoAPI_Common;
                mkBool.reset(fcCommon);
                TopTools_ListOfShape args;
                TopTools_ListOfShape toolList;
                args.Append(shapes[0].getShape());
                for (std::size_t i = 1; i < shapes.size(); ++i) {
                    toolList.Append(shapes[i].getShape());
                }
                if (args.IsEmpty() || toolList.IsEmpty()) {
                    mkBool.reset();
                }
                else {
                    fcCommon->SetArguments(args);
                    fcCommon->SetTools(toolList);
                    const double fuzzy = FuzzyTolerance.getValue();
                    if (fuzzy > 0.0) {
                        fcCommon->SetFuzzyValue(fuzzy);
                    }
                    else {
                        setCommonAutoFuzzy(fcCommon, args, toolList);
                    }
                    fcCommon->Build();
                    if (!fcCommon->IsDone()) {
                        FC_ERR("Boolean Common maker failed");
                        return new App::DocumentObjectExecReturn(
                            QT_TRANSLATE_NOOP("Exception", "Boolean operation failed")
                        );
                    }
                    result.makeElementShape(*fcCommon, shapes, op);
                }
            }
            if (!mkBool) {
                result.makeElementBoolean(op, shapes, nullptr, FuzzyTolerance.getValue());
            }
        }
        catch (Standard_Failure& e) {
            FC_ERR("Boolean operation failed: " << e.GetMessageString());
            return new App::DocumentObjectExecReturn(
                QT_TRANSLATE_NOOP("Exception", "Boolean operation failed")
            );
        }
    }

    // Refine replaces TShapes; fromMaker images would miss. Skip when we have
    // a live Fuse/Cut/Common maker. Fuse/Cut/Common-fallback still refine.
    if (!mkBool) {
        result = refineShapeIfActive(result);
    }

    if (!isSingleSolidRuleSatisfied(result.getShape())) {
        return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP(
            "Exception",
            "Result has multiple solids: enable 'Allow Compound' in the active body."
        ));
    }

    this->Shape.setValue(getSolid(result));

    if (mkBool) {
        std::vector<Part::TopoShape> toolShapes;
        if (shapes.size() > 1) {
            toolShapes.assign(shapes.begin() + 1, shapes.end());
        }
        const char* diag = (type == "Cut") ? "cutDiag"
            : (type == "Common")           ? "commonDiag"
                                           : "booleanDiag";
        publishBooleanSemanticHistory(
            mkBool.get(),
            this->Shape.getShape(),
            baseObj,
            shapes.empty() ? baseTopShape : shapes[0],
            tools,
            toolShapes,
            diag
        );
    }

    return StdReturn;
}

void Boolean::updatePreviewShape()
{
    if (strcmp(Type.getValueAsString(), "Cut") == 0) {
        TopoShape base;
        if (UseLegacyBodyPlacement.getValue()) {
            base = getBaseTopoShape(true);
            base.move(getLocation().Inverted());
        }
        else {
            base = getBooleanTopoShape(BaseFeature.getValue());
        }
        TopoShape result = Shape.getShape();

        try {
            PreviewShape.setValue(base.makeElementCut(result.getShape()));
        }
        catch (Standard_Failure& e) {
            FC_ERR("Boolean preview failed: " << e.GetMessageString());
            PreviewShape.setValue(base);
        }
        catch (Base::Exception& e) {
            FC_ERR("Boolean preview failed: " << e.what());
            PreviewShape.setValue(base);
        }
        return;
    }

    if (strcmp(Type.getValueAsString(), "Fuse") == 0) {
        // if there are no other shapes to fuse just return itself
        if (Group.getValues().empty()) {
            PreviewShape.setValue(Shape.getShape());
            return;
        }

        std::vector<TopoShape> shapes;

        for (auto& obj : Group.getValues()) {
            shapes.push_back(getBooleanTopoShape(obj));
        }

        TopoShape result;
        result.makeCompound(shapes);

        PreviewShape.setValue(result.getShape());
        return;
    }

    PreviewShape.setValue(Shape.getShape());
}

void Boolean::onChanged(const App::Property* prop)
{

    if (strcmp(prop->getName(), "Group") == 0) {
        touch();
    }

    Feature::onChanged(prop);
}

void Boolean::handleChangedPropertyName(Base::XMLReader& reader, const char* TypeName, const char* PropName)
{
    // The App::PropertyLinkList property was Bodies in the past
    Base::Type type = Base::Type::fromName(TypeName);

    if (Group.getClassTypeId() == type && strcmp(PropName, "Bodies") == 0) {
        Group.Restore(reader);
    }
}

}  // namespace PartDesign
