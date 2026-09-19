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

#include <algorithm>

#include <BRep_Builder.hxx>
#include <BRep_Tool.hxx>
#include <TopExp.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopTools_IndexedDataMapOfShapeListOfShape.hxx>
#include <TopExp_Explorer.hxx>


#include <boost/algorithm/string/predicate.hpp>

#include "FeatureDressUp.h"
#include "SemanticOpcode.h"
#include <Base/Console.h>
#include <App/Document.h>
#include <App/GeoFeature.h>
#include <Base/Exception.h>
#include "Mod/Part/App/TopoShapeMapper.h"
#include <Mod/Part/App/SemanticHistoryAdapter.h>

FC_LOG_LEVEL_INIT("PartDesign", true, true)

using namespace PartDesign;

namespace PartDesign
{


PROPERTY_SOURCE(PartDesign::DressUp, PartDesign::FeatureAddSub)

DressUp::DressUp()
{
    ADD_PROPERTY(Base, (nullptr));
    Placement.setStatus(App::Property::ReadOnly, true);

    ADD_PROPERTY_TYPE(
        SupportTransform,
        (false),
        "Base",
        App::Prop_None,
        "Include the base additive/subtractive shape when used in pattern features.\n"
        "If disabled, only the dressed part of the shape is used for patterning."
    );

    AddSubShape.setStatus(App::Property::Output, true);
    Operation.setStatus(App::Property::Hidden, true);
}

short DressUp::mustExecute() const
{
    if (Base.getValue() && Base.getValue()->isTouched()) {
        return 1;
    }
    return PartDesign::FeatureAddSub::mustExecute();
}


bool DressUp::isSemanticRepublishPass(const App::SemanticGraph* graph) const
{
    // Old FCStd restore can keep a valid cached Shape while STG1 has no durable
    // rows for this publisher. Shared Fillet/Chamfer gate for mustExecute/execute.
    return graph && isValid() && !Shape.getShape().isNull()
        && SemanticEmitter::needsSemanticRepublish(graph, static_cast<App::ObjectId>(getID()));
}

void DressUp::collectDressUpBaseSeeds(AfterExecuteRequest& req) const
{
    // Same Edge/Face kind checks Fillet and Chamfer used inline.
    auto appendUnique = [](std::vector<App::SemanticId>& seeds, const App::SemanticId& seed) {
        for (const App::SemanticId& existing : seeds) {
            if (existing == seed) {
                return;
            }
        }
        seeds.push_back(seed);
    };
    for (const App::SemanticReference& ref : Base.getSemanticRefs()) {
        if (!ref.seed.valid()) {
            continue;
        }
        if (ref.seed.kind == App::SemanticKind::Edge || ref.kind == App::SemanticKind::Edge) {
            appendUnique(req.filletEdges, ref.seed);
        }
        else if (ref.seed.kind == App::SemanticKind::Face) {
            appendUnique(req.filletAdjacentFaces, ref.seed);
        }
    }
}

std::optional<App::SemanticBinding> DressUp::uniqueResolvedDressUpBinding(
    const App::SemanticGraph* graph,
    const App::SemanticId& seed,
    App::ObjectId linkedFeature,
    App::SemanticKind expectedKind
) const
{
    if (!graph || !graph->hasBindings() || !seed.valid() || linkedFeature == 0
        || seed.kind != expectedKind) {
        return std::nullopt;
    }

    const char* expectedType = expectedKind == App::SemanticKind::Edge ? "Edge" : "Face";
    std::optional<App::SemanticBinding> found;
    for (const App::SemanticReference& ref : Base.getSemanticRefs()) {
        if (ref.seed != seed || ref.kind != expectedKind) {
            continue;
        }

        // The stored Base policy is authoritative. Require the live resolver
        // to produce one result under that policy before consuming its index;
        // a direct bindingsOf(seed) lookup would bypass filters/reducers.
        App::ReferenceRequirement requirement;
        requirement.expectedKind = expectedKind;
        requirement.acceptedCardinality = App::AcceptedCardinality::One;
        requirement.acceptedReducers = {ref.reducer};
        const App::ResolutionResult result = App::SemanticResolver::resolve(ref, *graph, &requirement);
        if (result.state != App::ResolutionState::Resolved || result.bindings.size() != 1) {
            continue;
        }

        const App::SemanticBinding& binding = result.bindings.front();
        if (binding.stid.handle != seed.handle || binding.stid.kind != seed.kind
            || binding.feature != linkedFeature || !Part::isNamedIndex(binding.index)  // PD32-D1
            || binding.index.type != expectedType) {
            continue;
        }
        if (found) {
            return std::nullopt;
        }
        found = binding;
    }
    return found;
}

void DressUp::retainResolvedDressUpSeeds(
    const App::SemanticGraph* graph,
    App::ObjectId linkedFeature,
    App::SemanticKind expectedKind,
    std::vector<App::SemanticId>& seeds
) const
{
    if (!graph || !graph->hasBindings() || linkedFeature == 0) {
        return;
    }
    std::vector<App::SemanticId> retained;
    retained.reserve(seeds.size());
    for (const App::SemanticId& seed : seeds) {
        if (uniqueResolvedDressUpBinding(graph, seed, linkedFeature, expectedKind)) {
            retained.push_back(seed);
        }
    }
    seeds = std::move(retained);
}

/// DressUp fromMaker mint refusal uses App::shouldRefuseDressUpMintKind
/// (Generated/Intersection/Split require a named conflict-free slot; I13
/// leave-unnamed otherwise). Non-minting kinds pass through. Ownership
/// details still compose hasOutputOwnershipConflict. Fillet/Chamfer/Draft/
/// Thickness call sites use the shared App helper.


void DressUp::positionByBaseFeature()
{
    Part::Feature* base = static_cast<Part::Feature*>(BaseFeature.getValue());
    if (base && base->isDerivedFrom<Part::Feature>()) {
        this->Placement.setValue(base->Placement.getValue());
    }
}

Part::Feature* DressUp::getBaseObject(bool silent) const
{
    Part::Feature* rv = Feature::getBaseObject(/* silent = */ true);
    if (rv) {
        return rv;
    }

    const char* err = nullptr;
    App::DocumentObject* base = Base.getValue();
    if (base) {
        if (base->isDerivedFrom<Part::Feature>()) {
            rv = static_cast<Part::Feature*>(base);
        }
        else {
            err = "Linked object is not a Part object";
        }
    }
    else {
        err = "No Base object linked";
    }

    if (!silent && err) {
        throw Base::RuntimeError(err);
    }

    return rv;
}

void DressUp::getContinuousEdges(Part::TopoShape TopShape, std::vector<std::string>& SubNames)
{

    std::vector<std::string> FaceNames;

    getContinuousEdges(TopShape, SubNames, FaceNames);
}

void DressUp::getContinuousEdges(
    Part::TopoShape TopShape,
    std::vector<std::string>& SubNames,
    std::vector<std::string>& FaceNames
)
{

    TopTools_IndexedMapOfShape mapOfEdges;
    TopTools_IndexedDataMapOfShapeListOfShape mapEdgeFace;
    TopExp::MapShapesAndAncestors(TopShape.getShape(), TopAbs_EDGE, TopAbs_FACE, mapEdgeFace);
    TopExp::MapShapes(TopShape.getShape(), TopAbs_EDGE, mapOfEdges);

    unsigned int i = 0;
    while (i < SubNames.size()) {
        std::string aSubName = static_cast<std::string>(SubNames.at(i));

        if (aSubName.compare(0, 4, "Edge") == 0) {
            TopoDS_Edge edge = TopoDS::Edge(TopShape.getSubShape(aSubName.c_str()));
            const TopTools_ListOfShape& los = mapEdgeFace.FindFromKey(edge);

            if (los.Extent() != 2) {
                SubNames.erase(SubNames.begin() + i);
                continue;
            }

            const TopoDS_Shape& face1 = los.First();
            const TopoDS_Shape& face2 = los.Last();
            GeomAbs_Shape cont
                = BRep_Tool::Continuity(TopoDS::Edge(edge), TopoDS::Face(face1), TopoDS::Face(face2));
            if (cont != GeomAbs_C0) {
                SubNames.erase(SubNames.begin() + i);
                continue;
            }

            i++;
        }
        else if (aSubName.compare(0, 4, "Face") == 0) {
            TopoDS_Face face = TopoDS::Face(TopShape.getSubShape(aSubName.c_str()));

            TopTools_IndexedMapOfShape mapOfFaces;
            TopExp::MapShapes(face, TopAbs_EDGE, mapOfFaces);

            for (int j = 1; j <= mapOfFaces.Extent(); ++j) {
                TopoDS_Edge edge = TopoDS::Edge(mapOfFaces.FindKey(j));

                int id = mapOfEdges.FindIndex(edge);

                std::stringstream buf;
                buf << "Edge";
                buf << id;

                if (std::ranges::find(SubNames, buf.str()) == SubNames.end()) {
                    SubNames.push_back(buf.str());
                }
            }

            FaceNames.emplace_back(aSubName.c_str());
            SubNames.erase(SubNames.begin() + i);
        }
        // empty name or any other sub-element
        else {
            SubNames.erase(SubNames.begin() + i);
        }
    }
}

void DressUp::refreshBaseElementReferences()
{
    App::DocumentObject* linked = Base.getValue();
    if (!linked || !linked->isAttachedToDocument()) {
        return;
    }
    auto* geo = freecad_cast<App::GeoFeature*>(linked);
    if (!geo) {
        return;
    }
    // Same App remapping GeoFeature runs when support Shape changes: ElementMap
    // shadow + unique geometry-cache search (PropertyLinkSub::updateElementReference).
    // Exactly one match rewrites EdgeN/FaceN; 0 or many leave Invalid/Missing (I13).
    // Does not mint stSeed from raw EdgeN / FaceN.
    Base.updateElementReference(geo, /*reverse*/ false, /*notify*/ true);
}

std::vector<TopoShape> DressUp::getContinuousEdges(const TopoShape& shape, const App::SemanticGraph* graph)
{
    // Candidate A (manual-3-dressup-relink-a): refresh Base LinkSub before
    // resolving EdgeN so in-place support reshape can unique-rematch.
    refreshBaseElementReferences();

    std::vector<TopoShape> ret;
    std::unordered_set<TopoDS_Shape, Part::ShapeHasher, Part::ShapeHasher> shapeSet;

    auto addEdge = [&](const TopoDS_Shape& subshape, const std::string& ref) {
        if (!shapeSet.insert(subshape).second) {
            return;
        }

        auto faces = shape.findAncestorsShapes(subshape, TopAbs_FACE);
        if (faces.size() != 2) {
            FC_WARN(getFullName() << ": skip edge " << ref << " with less two attaching faces");
            return;
        }
        const TopoDS_Shape& face1 = faces.front();
        const TopoDS_Shape& face2 = faces.back();
        GeomAbs_Shape cont
            = BRep_Tool::Continuity(TopoDS::Edge(subshape), TopoDS::Face(face1), TopoDS::Face(face2));
        if (cont != GeomAbs_C0) {
            FC_WARN(getFullName() << ": skip edge " << ref << " that is not C0 continuous");
            return;
        }
        ret.push_back(subshape);
    };

    for (const auto& ref : getEdgeSubValues(graph)) {
        TopoDS_Shape subshape;
        subshape = shape.getSubShape(ref.c_str(), true);
        if (subshape.IsNull()) {
            // Option B: callers (Fillet/Chamfer) catch Base::Exception and return
            // DocumentObjectExecReturn so Document::_recomputeFeature does not
            // reportException-spam for already-broken '?Edge*' links. Still
            // Invalid — never claim success or invent an edge.
            FC_THROWM(Base::CADKernelError, "Invalid edge link: " << ref);
        }

        if (subshape.ShapeType() == TopAbs_EDGE) {
            addEdge(subshape, ref);
        }
        else if (subshape.ShapeType() == TopAbs_FACE || subshape.ShapeType() == TopAbs_WIRE) {
            for (TopExp_Explorer exp(subshape, TopAbs_EDGE); exp.More(); exp.Next()) {
                addEdge(exp.Current(), std::string());
            }
        }
        else {
            FC_WARN(
                getFullName() << ": skip invalid shape '" << ref << "' with type "
                              << TopoShape::shapeName(subshape.ShapeType())
            );
        }
    }
    return ret;
}

std::vector<TopoShape> DressUp::getFaces(const TopoShape& shape, const App::SemanticGraph* graph)
{
    refreshBaseElementReferences();

    std::vector<TopoShape> ret;
    for (const auto& ref : getFaceSubValues(graph)) {
        TopoShape subshape;
        try {
            subshape = shape.getSubTopoShape(ref.c_str());
        }
        catch (...) {
        }

        if (subshape.isNull()) {
            FC_ERR(getFullName() << ": invalid face reference '" << ref << "'");
            throw Part::NullShapeException("Invalid Invalid face link");
        }

        if (subshape.shapeType() != TopAbs_FACE) {
            FC_WARN(
                getFullName() << ": skip invalid shape '" << ref << "' with type "
                              << subshape.shapeName()
            );
            continue;
        }
        ret.push_back(subshape);
    }
    return ret;
}

std::vector<std::string> DressUp::getFaceSubValues(const App::SemanticGraph* graph) const
{
    // D9-S1: uniquely resolved Binding rewrites FaceN into the maker. When a seed
    // is present but uniqueResolvedFaceReference fails (0/many), skip the stale
    // FaceN (fail-closed / silence - match Facebinder I13). Seedless FaceN-only
    // selections still pass through (preserve Face UX).
    std::vector<std::string> ret;
    const std::vector<std::string> vals = Base.getSubValues(false);
    const auto& refs = Base.getSemanticRefs();
    const App::ObjectId baseFeature = Base.getValue()
        ? static_cast<App::ObjectId>(Base.getValue()->semanticProjectionFeatureId())
        : 0;
    ret.reserve(vals.size());
    for (size_t i = 0; i < vals.size(); ++i) {
        const char* element = Data::findElementName(vals[i].c_str());
        if (!element || !boost::starts_with(element, "Face")) {
            continue;
        }
        std::string ref = element;
        if (graph && baseFeature != 0 && i < refs.size()) {
            if (const auto unique = uniqueResolvedFaceReference(graph, refs[i], baseFeature)) {
                ref = unique->index.toString();
            }
            else if (refs[i].seed.valid()) {
                // Ambiguous / Missing / Incompatible with a seed: do not invent.
                continue;
            }
        }
        ret.push_back(std::move(ref));
    }
    return ret;
}

std::vector<std::string> DressUp::getEdgeSubValues(const App::SemanticGraph* graph) const
{
    // D9-S1 sibling (Edge path): unique Binding rewrites EdgeN/FaceN; seed present
    // but non-unique → skip (fail-closed / silence). Seedless EdgeN/FaceN-only
    // selections still pass through (preserve Edge/Face UX).
    std::vector<std::string> ret;
    const std::vector<std::string> vals = Base.getSubValues(false);
    const auto& refs = Base.getSemanticRefs();
    const App::ObjectId baseFeature = Base.getValue()
        ? static_cast<App::ObjectId>(Base.getValue()->semanticProjectionFeatureId())
        : 0;
    ret.reserve(vals.size());
    for (size_t i = 0; i < vals.size(); ++i) {
        const char* element = Data::findElementName(vals[i].c_str());
        std::string ref = element ? element : vals[i];
        if (graph && baseFeature != 0 && i < refs.size()) {
            if (boost::starts_with(ref, "Edge")) {
                if (const auto unique = uniqueResolvedDressUpBinding(
                        graph,
                        refs[i].seed,
                        baseFeature,
                        App::SemanticKind::Edge
                    )) {
                    ref = unique->index.toString();
                }
                else if (refs[i].seed.valid()) {
                    continue;
                }
            }
            else if (boost::starts_with(ref, "Face")) {
                // Face-selected continuous edges expand from the live Face slot.
                if (const auto unique = uniqueResolvedFaceReference(graph, refs[i], baseFeature)) {
                    ref = unique->index.toString();
                }
                else if (refs[i].seed.valid()) {
                    continue;
                }
            }
        }
        ret.push_back(std::move(ref));
    }
    return ret;
}

void DressUp::onChanged(const App::Property* prop)
{
    if (prop == &Base) {
        if (BaseFeature.getValue() && Base.getValue() != BaseFeature.getValue()) {
            BaseFeature.setValue(Base.getValue());
        }
    }
    else if (prop == &Shape || prop == &SupportTransform) {
        if (!getDocument()->testStatus(App::Document::Restoring)
            && !getDocument()->isPerformingTransaction()) {
            // AddSubShape acts as a shape cache; invalidate so Transformed
            // can rebuild it lazily via getAddSubShape().
            AddSubShape.setValue(Part::TopoShape());
        }
    }

    Feature::onChanged(prop);
}

void DressUp::onBaseFeatureRerouted(App::DocumentObject* oldBase, App::DocumentObject* newBase)
{
    relinkToMatchingSubelements(Base, oldBase, newBase);
}

void DressUp::getAddSubShape(Part::TopoShape& addShape, Part::TopoShape& subShape)
{
    Part::TopoShape res = AddSubShape.getShape();

    if (res.isNull()) {
        try {
            std::vector<Part::TopoShape> shapes;
            Part::TopoShape shape = Shape.getShape();
            shape.setPlacement(Base::Placement());

            FeatureAddSub* base = nullptr;
            if (SupportTransform.getValue()) {
                // SupportTransform means transform the support together with
                // the dressing. So we need to find the previous support
                // feature (which must be of type FeatureAddSub), and skipping
                // any consecutive DressUp in-between.
                for (Feature* current = this;; current = static_cast<DressUp*>(base)) {
                    base = freecad_cast<FeatureAddSub*>(current->getBaseObject(true));
                    if (!base) {
                        FC_THROWM(
                            Base::CADKernelError,
                            "Cannot find additive or subtractive support for " << getFullName()
                        );
                    }
                    if (!base->isDerivedFrom<DressUp>()) {
                        break;
                    }
                }
            }

            Part::TopoShape baseShape;
            if (base) {
                baseShape = base->getBaseTopoShape(true);
                baseShape.move(base->getLocation().Inverted());
                if (base->getAddSubType() == FeatureAddSub::Type::Additive) {
                    if (!baseShape.isNull() && baseShape.hasSubShape(TopAbs_SOLID)) {
                        shapes.emplace_back(shape.makeElementCut(baseShape.getShape()));
                    }
                    else {
                        shapes.push_back(shape);
                    }
                }
                else {
                    BRep_Builder builder;
                    TopoDS_Compound comp;
                    builder.MakeCompound(comp);
                    // push an empty compound to indicate null additive shape
                    shapes.emplace_back(comp);
                    if (!baseShape.isNull() && baseShape.hasSubShape(TopAbs_SOLID)) {
                        shapes.emplace_back(baseShape.makeElementCut(shape.getShape()));
                    }
                    else {
                        shapes.push_back(shape);
                    }
                }
            }
            else {
                baseShape = getBaseTopoShape();
                baseShape.move(getLocation().Inverted());
                shapes.emplace_back(shape.makeElementCut(baseShape.getShape()));
                shapes.emplace_back(baseShape.makeElementCut(shape.getShape()));
            }

            // Make a compound to contain both additive and subtractive shape,
            // bceause a dressing (e.g. a fillet) can either be additive or
            // subtractive. And the dressup feature can contain mixture of both.
            AddSubShape.setValue(Part::TopoShape().makeElementCompound(shapes));
        }
        catch (Standard_Failure& e) {
            FC_THROWM(
                Base::CADKernelError,
                "Failed to calculate AddSub shape: " << e.GetMessageString()
            );
        }
        res = AddSubShape.getShape();
    }

    if (res.isNull()) {
        throw Part::NullShapeException("Null AddSub shape");
    }

    if (res.getShape().ShapeType() != TopAbs_COMPOUND) {
        addShape = res;
    }
    else {
        int count = res.countSubShapes(TopAbs_SHAPE);
        if (!count) {
            throw Part::NullShapeException("Null AddSub shape");
        }
        if (count) {
            Part::TopoShape s = res.getSubTopoShape(TopAbs_SHAPE, 1);
            if (!s.isNull() && s.hasSubShape(TopAbs_SOLID)) {
                addShape = s;
            }
        }
        if (count > 1) {
            Part::TopoShape s = res.getSubTopoShape(TopAbs_SHAPE, 2);
            if (!s.isNull() && s.hasSubShape(TopAbs_SOLID)) {
                subShape = s;
            }
        }
    }
}

void DressUp::updatePreviewShape()
{
    auto shape = Shape.getShape();
    auto baseFeature = freecad_cast<Feature*>(BaseFeature.getValue());

    if (!baseFeature || baseFeature->Shape.getShape().isNull()) {
        PreviewShape.setValue(Shape.getShape());
        return;
    }

    std::vector<int> faces, edges, vertices;
    getGeneratedShapes(faces, edges, vertices);

    if (faces.empty()) {
        PreviewShape.setValue(TopoDS_Shape());
        return;
    }

    shape.setPlacement(Base::Placement());

    BRep_Builder builder;
    TopoDS_Compound comp;
    builder.MakeCompound(comp);

    for (int faceId : faces) {
        builder.Add(comp, shape.getSubShape(TopAbs_FACE, faceId));
    }

    Part::TopoShape preview(comp);
    preview.mapSubElement(shape);

    PreviewShape.setValue(preview);
}

}  // namespace PartDesign
