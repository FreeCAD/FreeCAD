// SPDX-License-Identifier: LGPL-2.1-or-later

/******************************************************************************
 *   Copyright (c) 2012 Jan Rheinländer <jrheinlaender@users.sourceforge.net> *
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

#include <Bnd_Box.hxx>
#include <BRep_Builder.hxx>
#include <Mod/Part/App/FCBRepAlgoAPI_Cut.h>
#include <Mod/Part/App/FCBRepAlgoAPI_Fuse.h>
#include <BRepBndLib.hxx>
#include <BRepBuilderAPI_Copy.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <Precision.hxx>
#include <TopExp_Explorer.hxx>
#include <TopExp.hxx>
#include <BRep_Tool.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Vertex.hxx>


#include <array>
#include <unordered_map>
#include <algorithm>

#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Reader.h>
#include <Base/Sequencer.h>
#include <Mod/Part/App/modelRefine.h>

#include "FeatureTransformed.h"
#include "Body.h"
#include "FeatureAddSub.h"
#include "FeatureMultiTransform.h"
#include "FeatureScaled.h"
#include "FeatureMirrored.h"
#include "FeatureLinearPattern.h"
#include "FeaturePolarPattern.h"
#include "FeatureSketchBased.h"
#include "Mod/Part/App/TopoShapeOpCode.h"

#include <deque>
#include <memory>
#include <optional>
#include <unordered_set>
#include <vector>

#include <BRepAdaptor_Surface.hxx>
#include <BRepAlgoAPI_BooleanOperation.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <gp_Pln.hxx>
#include <gp_Pnt.hxx>

#include <App/Document.h>
#include <App/SemanticDocumentState.h>
#include <App/SemanticReference.h>
#include <Mod/Part/App/FCBRepAlgoAPI_BooleanOperation.h>
#include <Mod/Part/App/SemanticHistoryAdapter.h>
#include <Mod/Part/App/SemanticSourceCollector.h>
#include <TopTools_ListOfShape.hxx>

#include "SemanticOpcode.h"


using namespace PartDesign;

namespace
{


/// A maker history row is usable only when its output still resolves on the
/// exact shape that will be published. Keep stale or malformed rows out of
/// applyHistory; no semantic binding is safer than a binding to another
/// output (I13).
bool isUsableTransformedHistory(const Part::TopoShape& published, const Part::HistoryRecord& record)
{
    if (!record.fromSeed.valid() || published.isNull() || record.toIndex.index <= 0) {
        return false;
    }
    TopAbs_ShapeEnum type;
    if (record.toIndex.type == "Face") {
        type = TopAbs_FACE;
    }
    else if (record.toIndex.type == "Edge") {
        type = TopAbs_EDGE;
    }
    else {
        return false;
    }
    return !published.findShape(type, record.toIndex.index).IsNull();
}

const char* transformedOpcodeName(const App::DocumentObject* self)
{
    if (freecad_cast<const LinearPattern*>(self)) {
        return opcodeName(Opcode::LinearPattern);
    }
    if (freecad_cast<const PolarPattern*>(self)) {
        return opcodeName(Opcode::PolarPattern);
    }
    if (freecad_cast<const Mirrored*>(self)) {
        return opcodeName(Opcode::Mirrored);
    }
    if (freecad_cast<const Scaled*>(self)) {
        return opcodeName(Opcode::Scaled);
    }
    if (freecad_cast<const MultiTransform*>(self)) {
        return opcodeName(Opcode::MultiTransform);
    }
    return "Transformed";
}

/// Independent TopoDS for a boolean argument, tagged as the Transformed feature.
/// src.makeElementCopy() is TopoShape(src.Tag, Hasher).makeElementCopy(*this)
/// so the copy would keep Pad's Tag. Element-boolean / hasher caches key off Tag;
/// a Pad-tagged copy aliases back onto Pad.Shape (null support, smashed placement).
Part::TopoShape copyTopoForBoolean(const Part::TopoShape& src, long selfTag)
{
    if (src.isNull()) {
        return src;
    }
    Part::TopoShape dst(selfTag, src.Hasher);
    dst.makeElementCopy(src);
    return dst;
}

/// Side-maker for fromMaker only. makeElementBoolean does not keep the maker
/// (local unique_ptr, destroyed on return). Product solid stays makeElementFuse/Cut.
/// Arguments MUST already be retagged copies whose TopoDS is never live Pad.Shape.
/// Never call FCBRepAlgoAPIHelper from PartDesign (LNK2019).
std::unique_ptr<BRepAlgoAPI_BooleanOperation> sideMakerOnCopies(
    bool fuse,
    const std::vector<Part::TopoShape>& copies
)
{
    if (copies.size() < 2) {
        return nullptr;
    }
    std::unique_ptr<BRepAlgoAPI_BooleanOperation> mk;
    if (fuse) {
        mk.reset(new FCBRepAlgoAPI_Fuse);
    }
    else {
        mk.reset(new FCBRepAlgoAPI_Cut);
    }
    TopTools_ListOfShape args;
    TopTools_ListOfShape tools;
    for (std::size_t i = 0; i < copies.size(); ++i) {
        if (copies[i].isNull()) {
            continue;
        }
        if (args.IsEmpty()) {
            args.Append(copies[i].getShape());
        }
        else {
            tools.Append(copies[i].getShape());
        }
    }
    if (args.IsEmpty() || tools.IsEmpty()) {
        return nullptr;
    }
    mk->SetRunParallel(Standard_True);
    mk->SetArguments(args);
    mk->SetTools(tools);
    mk->Build();
    if (!mk->IsDone()) {
        return nullptr;
    }
    return mk;
}

App::DocumentObjectExecReturn* applyTransformedBoolean(
    Part::TopoShape& supportShape,
    bool fuse,
    const std::vector<Part::TopoShape>& shapes,
    std::unique_ptr<BRepAlgoAPI_BooleanOperation>& lastMk,
    Part::TopoShape& lastSeedShape,
    long selfTag
)
{
    // Retagged copies (Transformed id, never Pad.Tag). Product prefers the
    // side-maker Shape so fromMaker images are the stored TShape.
    std::vector<Part::TopoShape> copies;
    copies.reserve(shapes.size());
    for (const auto& s : shapes) {
        if (s.isNull()) {
            continue;
        }
        copies.push_back(copyTopoForBoolean(s, selfTag));
    }
    if (copies.empty()) {
        return nullptr;
    }
    // Patterned instance (last copy), not the support original. uniqueOneImage
    // on the original drops 2-image Pad leftovers; Fillet picks the outer copy.
    lastSeedShape = copies.size() >= 2 ? copies.back() : copies.front();
    if (copies.size() < 2) {
        supportShape = copies.front();
        return nullptr;
    }

    // Product AND fromMaker must share one TShape. makeElementFuse then a
    // second BOP made indexOnPublished miss → seedless LinearPattern Fillet.
    auto mk = sideMakerOnCopies(fuse, copies);
    if (mk) {
        lastMk = std::move(mk);
        Part::TopoShape named(selfTag, copies.front().Hasher);
        named.setShape(lastMk->Shape(), false);
        supportShape = named;
        return nullptr;
    }

    try {
        if (fuse) {
            supportShape.makeElementFuse(copies);
        }
        else {
            supportShape.makeElementCut(copies);
        }
    }
    catch (Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }
    catch (const Standard_Failure& e) {
        return new App::DocumentObjectExecReturn(e.GetMessageString());
    }
    if (selfTag != 0 && supportShape.Tag != selfTag) {
        supportShape.reTagElementMap(selfTag, supportShape.Hasher);
    }
    return nullptr;
}

void publishTransformedSemanticHistory(
    App::DocumentObject* self,
    BRepAlgoAPI_BooleanOperation* mkBool,
    const Part::TopoShape& result,
    App::DocumentObject* supportObj,
    const Part::TopoShape& supportShape,
    const std::vector<App::DocumentObject*>& originals
)
{
    if (!self || !mkBool || !mkBool->IsDone() || result.isNull()) {
        return;
    }
    App::SemanticGraph* graph = App::SemanticDocumentState::graphFor(self);
    if (!graph && self->getDocument()) {
        graph = &self->getDocument()->semanticGraph();
    }
    if (!graph) {
        return;
    }

    std::deque<TopoDS_Shape> held;
    std::vector<std::pair<App::SemanticId, const void*>> inputs;
    std::unordered_set<App::SemanticHandle> seenSourceSeeds;
    std::unordered_set<App::DocumentObject*> seen;
    auto collect = [&](App::DocumentObject* obj) {
        if (!obj || !seen.insert(obj).second) {
            return;
        }
        Part::collectUniqueSourceSeeds(graph, obj, supportShape, held, inputs, seenSourceSeeds);
    };
    collect(supportObj);
    for (App::DocumentObject* orig : originals) {
        collect(orig);
    }
    if (inputs.empty()) {
        return;
    }

    auto indexOf = [&result](const void* occ) -> App::ElementIndex {
        App::ElementIndex idx;
        if (!occ) {
            return idx;
        }
        return Part::indexOnPublished(result, *static_cast<const TopoDS_Shape*>(occ));
    };

    const Part::HistoryTable raw = Part::SemanticHistoryAdapter::fromMaker(mkBool, inputs, indexOf);
    const Part::HistoryTable unique = Part::SemanticHistoryAdapter::uniqueOneImageGenerated(raw);

    const App::ObjectId selfId = static_cast<App::ObjectId>(self->getID());
    App::EvalSerial eval = 0;
    if (App::Document* doc = self->getDocument()) {
        eval = doc->semanticState().currentEval();
    }
    Part::HistoryTable toApply;
    std::vector<App::SemanticId> seeds;
    toApply.reserve(unique.size());
    seeds.reserve(unique.size());
    for (const Part::HistoryRecord& rec : unique) {
        if (!isUsableTransformedHistory(result, rec)) {
            continue;
        }
        // I13 leave-unnamed / C1: already bound, conflicted slot, or unique owner.
        if (App::shouldRefuseGeneratedMint(graph, rec.fromSeed, selfId, rec.toIndex)) {
            continue;
        }
        toApply.push_back(rec);
        seeds.push_back(rec.fromSeed);
    }
    if (toApply.empty()) {
        return;
    }

    const char* opcode = transformedOpcodeName(self);
    Part::SemanticHistoryAdapter::applyHistory(graph, selfId, eval, opcode ? opcode : "", seeds, toApply);
}

}  // namespace

namespace PartDesign
{
extern bool getPDRefineModelParameter();

PROPERTY_SOURCE(PartDesign::Transformed, PartDesign::FeatureRefine)

std::array<char const*, 3> transformModeEnums = {"Features", "Whole shape", nullptr};

Transformed::Transformed()
{
    ADD_PROPERTY(Originals, (nullptr));
    Originals.setSize(0);
    Placement.setStatus(App::Property::ReadOnly, true);

    ADD_PROPERTY(TransformMode, (static_cast<long>(Mode::Features)));
    TransformMode.setEnums(transformModeEnums.data());
}

void Transformed::positionBySupport()
{
    // TODO May be here better to throw exception (silent=false) (2015-07-27, Fat-Zer)
    Part::Feature* support = getBaseObject(/* silent =*/true);
    if (support) {
        this->Placement.setValue(support->Placement.getValue());
    }
}

Part::Feature* Transformed::getBaseObject(bool silent) const
{
    Part::Feature* rv = Feature::getBaseObject(/* silent = */ true);
    if (rv) {
        return rv;
    }

    const char* err = nullptr;
    const std::vector<App::DocumentObject*>& originals = getOriginals();
    // NOTE: may be here supposed to be last origin but in order to keep the old behaviour keep here
    // first
    App::DocumentObject* firstOriginal = originals.empty() ? nullptr : originals.front();
    if (firstOriginal) {
        rv = freecad_cast<Part::Feature*>(firstOriginal);
        if (!rv) {
            err = QT_TRANSLATE_NOOP(
                "Exception",
                "Transformation feature Linked object is not a Part object"
            );
        }
    }
    else {
        if (freecad_cast<const Mirrored*>(this)) {
            err = QT_TRANSLATE_NOOP("Exception", "No features selected to be mirrored.");
        }
        else if (freecad_cast<const LinearPattern*>(this) || freecad_cast<const PolarPattern*>(this)) {
            err = QT_TRANSLATE_NOOP("Exception", "No features selected to be patterned.");
        }
        else {
            err = QT_TRANSLATE_NOOP("Exception", "No features selected to be transformed.");
        }
    }

    if (!silent && err) {
        throw Base::RuntimeError(err);
    }

    return rv;
}

std::vector<App::DocumentObject*> Transformed::getSortedOriginals() const
{
    std::vector<DocumentObject*> originals = Originals.getValues();

    // Sort originals in chronological order of the body's group history
    if (auto body = getFeatureBody()) {
        const auto& group = body->Group.getValues();
        std::unordered_map<const DocumentObject*, size_t> indexMap;
        for (size_t i = 0; i < group.size(); ++i) {
            indexMap[group[i]] = i;
        }
        std::ranges::sort(originals, [&indexMap](const DocumentObject* a, const DocumentObject* b) {
            auto itA = indexMap.find(a);
            auto itB = indexMap.find(b);
            size_t idxA = (itA != indexMap.end()) ? itA->second : std::numeric_limits<size_t>::max();
            size_t idxB = (itB != indexMap.end()) ? itB->second : std::numeric_limits<size_t>::max();
            return idxA < idxB;
        });
    }

    return originals;
}

std::vector<App::DocumentObject*> Transformed::getOriginals() const
{
    auto const mode = static_cast<Mode>(TransformMode.getValue());

    if (mode == Mode::WholeShape) {
        return {};
    }

    std::vector<DocumentObject*> originals = getSortedOriginals();

    const auto isSuppressed = [](const DocumentObject* obj) {
        auto feature = freecad_cast<Feature*>(obj);

        return feature != nullptr && feature->Suppressed.getValue();
    };

    // Remove suppressed features from the list so the transformations behave as if they are not
    // there
    auto [first, last] = std::ranges::remove_if(originals, isSuppressed);
    originals.erase(first, last);

    return originals;
}

App::DocumentObject* Transformed::getSketchObject() const
{
    std::vector<DocumentObject*> originals = getOriginals();
    DocumentObject const* firstOriginal = !originals.empty() ? originals.front() : nullptr;

    if (auto feature = freecad_cast<PartDesign::ProfileBased*>(firstOriginal)) {
        return feature->getVerifiedSketch(true);
    }
    if (freecad_cast<PartDesign::FeatureAddSub*>(firstOriginal)) {
        return nullptr;
    }
    if (auto pattern = freecad_cast<LinearPattern*>(this)) {
        return pattern->Direction.getValue();
    }
    if (auto pattern = freecad_cast<PolarPattern*>(this)) {
        return pattern->Axis.getValue();
    }
    if (auto pattern = freecad_cast<Mirrored*>(this)) {
        return pattern->MirrorPlane.getValue();
    }

    return nullptr;
}

void Transformed::Restore(Base::XMLReader& reader)
{
    PartDesign::Feature::Restore(reader);
}

bool Transformed::isMultiTransformChild() const
{
    // Checking for a MultiTransform in the dependency list is not reliable on initialization
    // because the dependencies are only established after creation.
    /*
    for (auto const* obj : getInList()) {
        auto mt = freecad_cast<PartDesign::MultiTransform*>(obj);
        if (!mt) {
            continue;
        }

        auto const transfmt = mt->Transformations.getValues();
        if (std::find(transfmt.begin(), transfmt.end(), this) != transfmt.end()) {
            return true;
        }
    }
    */

    // instead check for default property values because these are invalid for a standalone
    // transform feature. This will mislabel standalone features during the initialization phase.
    if (TransformMode.getValue() == 0 && Originals.getValue().empty()) {
        return true;
    }

    return false;
}

void Transformed::handleChangedPropertyType(
    Base::XMLReader& reader,
    const char* TypeName,
    App::Property* prop
)
{
    // The property 'Angle' of PolarPattern has changed from PropertyFloat
    // to PropertyAngle and the property 'Length' has changed to PropertyLength.
    Base::Type inputType = Base::Type::fromName(TypeName);
    if (auto property = freecad_cast<App::PropertyFloat*>(prop);
        property != nullptr && inputType.isDerivedFrom(App::PropertyFloat::getClassTypeId())) {
        // Do not directly call the property's Restore method in case the implementation
        // has changed. So, create a temporary PropertyFloat object and assign the value.
        App::PropertyFloat floatProp;
        floatProp.Restore(reader);
        property->setValue(floatProp.getValue());
    }
    else {
        PartDesign::Feature::handleChangedPropertyType(reader, TypeName, prop);
    }
}

short Transformed::mustExecute() const
{
    if (Originals.isTouched() || TransformMode.isTouched()) {
        return 1;
    }

    // Restored documents may retain a valid transformed Shape while STG1 has
    // no durable event/identity rows for this publisher. Schedule one normal
    // execution so Linear/Polar/Mirrored (and the other Transformed variants)
    // can republish their semantic history. MultiTransform children are not
    // publishers and must remain owned by their parent.
    if (!isMultiTransformChild()) {
        if (const App::SemanticGraph* graph = SemanticEmitter::graphFor(this); graph && isValid()
            && !Shape.getShape().isNull()
            && SemanticEmitter::needsSemanticRepublish(graph, static_cast<App::ObjectId>(getID()))) {
            return 1;
        }
    }

    return PartDesign::Feature::mustExecute();
}

App::DocumentObjectExecReturn* Transformed::recomputePreview()
{
    const auto mode = static_cast<Mode>(TransformMode.getValue());

    const Part::Feature* supportFeature = getBaseObject();
    const Part::TopoShape supportShape = supportFeature->Shape.getShape();

    if (supportShape.isNull()) {
        return App::DocumentObject::StdReturn;
    }

    gp_Trsf supportTransform = supportShape.getShape().Location().Transformation();

    const auto makeCompoundOfToolShapes = [this, &supportTransform]() {
        BRep_Builder builder;
        TopoDS_Compound compound;

        builder.MakeCompound(compound);
        for (const auto& original : getOriginals()) {
            if (auto* feature = freecad_cast<FeatureAddSub*>(original)) {
                auto shape = feature->AddSubShape.getShape();

                gp_Trsf trsf = supportTransform.Inverted().Multiplied(
                    feature->getLocation().Transformation()
                );

                if (shape.isNull()) {
                    continue;
                }

                shape = shape.makeElementTransform(trsf);

                builder.Add(compound, shape.getShape());
            }
        }

        return compound;
    };

    switch (mode) {
        case Mode::Features:
            PreviewShape.setValue(makeCompoundOfToolShapes());
            return StdReturn;

        case Mode::WholeShape: {
            auto shape = getBaseTopoShape();
            shape = shape.makeElementTransform(supportTransform.Inverted());

            PreviewShape.setValue(shape.getShape());

            return StdReturn;
        }

        default:
            return FeatureRefine::recomputePreview();
    }
}

void Transformed::onChanged(const App::Property* prop)
{
    if (prop == &TransformMode) {
        auto const mode = static_cast<Mode>(TransformMode.getValue());
        Originals.setStatus(App::Property::Status::Hidden, mode == Mode::WholeShape);
    }

    FeatureRefine::onChanged(prop);
}

App::DocumentObjectExecReturn* Transformed::execute()
{
    if (isMultiTransformChild()) {
        return App::DocumentObject::StdReturn;
    }

    auto const mode = static_cast<Mode>(TransformMode.getValue());

    std::vector<DocumentObject*> originals = getOriginals();

    if (mode == Mode::Features && originals.empty()) {
        return App::DocumentObject::StdReturn;
    }

    if (!this->BaseFeature.getValue()) {
        if (auto body = getFeatureBody()) {
            body->setBaseProperty(this);
        }
    }

    this->positionBySupport();

    // get transformations from subclass by calling virtual method
    std::vector<gp_Trsf> transformations;
    try {
        std::list<gp_Trsf> t_list = getTransformations(originals);
        transformations.insert(transformations.end(), t_list.begin(), t_list.end());
    }
    catch (Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }
    catch (const Standard_Failure& e) {
        return new App::DocumentObjectExecReturn(e.GetMessageString());
    }

    if (transformations.empty()) {
        return App::DocumentObject::StdReturn;  // No transformations defined, exit silently
    }

    // Get the support
    Part::Feature* supportFeature = nullptr;

    try {
        supportFeature = getBaseObject();
    }
    catch (Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }

    const Part::TopoShape& supportTopShape = supportFeature->Shape.getShape();
    if (supportTopShape.getShape().IsNull()) {
        return new App::DocumentObjectExecReturn(
            QT_TRANSLATE_NOOP("Exception", "Cannot transform invalid support shape")
        );
    }

    // Deep-copy support before any OCC boolean. Destination Tag is this
    // Transformed feature id, never Pad's — hasher caches key off Tag.
    const long selfTag = this->getID();
    Part::TopoShape supportShape = copyTopoForBoolean(supportTopShape, selfTag);

    gp_Trsf trsfInv = supportShape.getShape().Location().Transformation().Inverted();

    supportShape.setTransform(Base::Matrix4D());

    auto getTransformedCompShape = [&](const auto& supportShape, const auto& origShape) {
        std::vector<TopoShape> shapes = {copyTopoForBoolean(supportShape, selfTag)};
        TopoShape shape = copyTopoForBoolean(origShape, selfTag);
        int idx = 1;
        auto transformIter = transformations.cbegin();
        transformIter++;
        for (; transformIter != transformations.end(); transformIter++) {
            if (Base::Sequencer().wasCanceled()) {
                return std::vector<TopoShape>();
            }
            auto opName = Data::indexSuffix(idx++);
            shapes.emplace_back(shape.makeElementTransform(*transformIter, opName.c_str()));
        }
        return shapes;
    };

    std::unique_ptr<BRepAlgoAPI_BooleanOperation> lastMk;
    Part::TopoShape lastSeedShape;

    switch (mode) {
        case Mode::Features:
            // NOTE: It would be possible to build a compound from all original addShapes/subShapes
            // and then transform the compounds as a whole. But we choose to apply the
            // transformations to each Original separately. This way it is easier to discover what
            // feature causes a fuse/cut to fail. The downside is that performance suffers when
            // there are many originals. But it seems safe to assume that in most cases there are
            // few originals and many transformations
            for (auto original : originals) {
                // Extract the original shape and determine whether to cut or to fuse
                Part::TopoShape fuseShape;
                Part::TopoShape cutShape;

                auto feature = freecad_cast<PartDesign::FeatureAddSub*>(original);
                if (!feature) {
                    return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP(
                        "Exception",
                        "Only additive and subtractive features can be transformed"
                    ));
                }

                feature->getAddSubShape(fuseShape, cutShape);
                if (fuseShape.isNull() && cutShape.isNull()) {
                    return new App::DocumentObjectExecReturn(
                        QT_TRANSLATE_NOOP("Exception", "Shape of additive/subtractive feature is empty")
                    );
                }
                gp_Trsf trsf = trsfInv.Multiplied(feature->getLocation().Transformation());
                if (!fuseShape.isNull()) {
                    fuseShape = copyTopoForBoolean(fuseShape, selfTag);
                    fuseShape = fuseShape.makeElementTransform(trsf);
                }
                if (!cutShape.isNull()) {
                    cutShape = copyTopoForBoolean(cutShape, selfTag);
                    cutShape = cutShape.makeElementTransform(trsf);
                }
                if (!fuseShape.isNull()) {
                    auto shapes = getTransformedCompShape(supportShape, fuseShape);
                    if (Base::Sequencer().wasCanceled()) {
                        return new App::DocumentObjectExecReturn("User aborted");
                    }
                    if (
                        auto* err
                        = applyTransformedBoolean(supportShape, true, shapes, lastMk, lastSeedShape, selfTag)
                    ) {
                        return err;
                    }
                }
                if (!cutShape.isNull()) {
                    auto shapes = getTransformedCompShape(supportShape, cutShape);
                    if (Base::Sequencer().wasCanceled()) {
                        return new App::DocumentObjectExecReturn("User aborted");
                    }
                    if (
                        auto* err
                        = applyTransformedBoolean(supportShape, false, shapes, lastMk, lastSeedShape, selfTag)
                    ) {
                        return err;
                    }
                }
            }
            break;
        case Mode::WholeShape: {
            auto shapes = getTransformedCompShape(supportShape, supportShape);
            if (Base::Sequencer().wasCanceled()) {
                return new App::DocumentObjectExecReturn("User aborted");
            }
            if (auto* err
                = applyTransformedBoolean(supportShape, true, shapes, lastMk, lastSeedShape, selfTag)) {
                return err;
            }
            break;
        }
    }

    // Refine replaces TShapes; fromMaker images would miss. Skip when we
    // have a side-maker product (isolate-4 emit). Fallback path still refines.
    if (!lastMk) {
        supportShape = refineShapeIfActive((supportShape));
        if (selfTag != 0 && supportShape.Tag != selfTag) {
            supportShape.reTagElementMap(selfTag, supportShape.Hasher);
        }
    }

    this->Shape.setValue(getSolid(supportShape));
    if (singleSolidRuleMode() == SingleSolidRuleMode::Enforced) {
        rejected = getRemainingSolids(supportShape.getShape());
    }
    else {
        rejected.Nullify();
    }

    // Unique 1-image Face/Edge Bindings on this Transformed feature (I13).
    // Product is the side-maker Shape (same TShape as fromMaker). Inputs are
    // Pad seeds on the patterned copy. allocatedBy is this feature, not Pad.
    publishTransformedSemanticHistory(
        this,
        lastMk.get(),
        this->Shape.getShape(),
        supportFeature,
        lastSeedShape.isNull() ? supportShape : lastSeedShape,
        originals
    );

    return App::DocumentObject::StdReturn;
}

TopoDS_Shape Transformed::getRemainingSolids(const TopoDS_Shape& shape)
{
    BRep_Builder builder;
    TopoDS_Compound compShape;
    builder.MakeCompound(compShape);

    if (shape.IsNull()) {
        throw Standard_Failure("Shape is null");
    }
    TopExp_Explorer xp;
    xp.Init(shape, TopAbs_SOLID);
    xp.Next();  // skip the first

    for (; xp.More(); xp.Next()) {
        builder.Add(compShape, xp.Current());
    }

    return {std::move(compShape)};
}

}  // namespace PartDesign
