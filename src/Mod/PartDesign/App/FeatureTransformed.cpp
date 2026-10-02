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

#include <BRepBndLib.hxx>
#include <BRepBuilderAPI_Copy.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRep_Builder.hxx>
#include <Bnd_Box.hxx>
#include <Mod/Part/App/FCBRepAlgoAPI_Cut.h>
#include <Mod/Part/App/FCBRepAlgoAPI_Fuse.h>
#include <Precision.hxx>
#include <TopExp_Explorer.hxx>

#include <algorithm>
#include <array>
#include <unordered_map>
#include <string>

#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Reader.h>
#include <Base/Sequencer.h>
#include <Mod/Part/App/modelRefine.h>

#include "Body.h"
#include "FeatureAddSub.h"
#include "FeatureLinearPattern.h"

#include "FeatureCircularPattern.h"
#include "FeaturePathPattern.h"
#include "FeaturePointPattern.h"
#include "FeatureMirrored.h"
#include "FeaturePolarPattern.h"
#include "FeatureSketchBased.h"
#include "FeatureTransformed.h"

#include <Mod/Part/App/TopoShapeOpCode.h>

#include "FeatureAddSub.h"
#include "FeatureBoolean.h"

using namespace PartDesign;

namespace PartDesign
{
extern bool getPDRefineModelParameter();

PROPERTY_SOURCE(PartDesign::Transformed, PartDesign::FeatureRefine)

std::array<char const*, 4> transformModeEnums = {"Tool Shapes", "Body", "Feature Result", nullptr};

Transformed::Transformed()
{
    ADD_PROPERTY(Originals, (nullptr));
    Originals.setSize(0);
    Placement.setStatus(App::Property::ReadOnly, true);

    ADD_PROPERTY(TransformMode, (static_cast<long>(Mode::Features)));
    TransformMode.setEnums(transformModeEnums.data());

    ADD_PROPERTY_TYPE(
        SuppressedIndices,
        (std::vector<long>()),
        "Transformation",
        App::Prop_None,
        "Indices of pattern instances that are suppressed."
    );
}

void Transformed::positionBySupport()
{
    // TODO May be here better to throw exception (silent=false) (2015-07-27,
    // Fat-Zer)
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
    // NOTE: may be here supposed to be last origin but in order to keep the old
    // behaviour keep here first
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
        if (isDerivedFrom<Mirrored>()) {
            err = QT_TRANSLATE_NOOP("Exception", "No features selected to be mirrored.");
        }
        else if (
            isDerivedFrom<LinearPattern>() || isDerivedFrom<CircularPattern>()
            || isDerivedFrom<PathPattern>() || isDerivedFrom<PointPattern>()
            || isDerivedFrom<PolarPattern>()
        ) {
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

    // Remove suppressed features from the list so the transformations behave as
    // if they are not there
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
    if (auto pattern = freecad_cast<CircularPattern*>(this)) {
        return pattern->Axis.getValue();
    }
    if (auto pattern = freecad_cast<PathPattern*>(this)) {
        return pattern->Path.getValue();
    }
    if (auto pattern = freecad_cast<PointPattern*>(this)) {
        return pattern->PointObject.getValue();
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
    // Checking for a MultiTransform in the dependency list is not reliable on
    // initialization because the dependencies are only established after
    // creation.
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

    // instead check for default property values because these are invalid for a
    // standalone transform feature. This will mislabel standalone features during
    // the initialization phase.
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
        // Do not directly call the property's Restore method in case the
        // implementation has changed. So, create a temporary PropertyFloat object
        // and assign the value.
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
    if (Originals.isTouched() || TransformMode.isTouched() || SuppressedIndices.isTouched()) {
        return 1;
    }
    return PartDesign::Feature::mustExecute();
}

bool Transformed::isTransformationSuppressed(int index) const
{
    if (index < 0) {
        return false;
    }

    const auto& suppressed = SuppressedIndices.getValues();
    return std::ranges::find(suppressed, static_cast<long>(index)) != suppressed.end();
}

void Transformed::setTransformationSuppressed(int index, bool suppress)
{
    if (index < 0 || isTransformationSuppressed(index) == suppress) {
        return;
    }
    auto suppressed = SuppressedIndices.getValues();
    if (suppress) {
        suppressed.push_back(index);
    }
    else {
        std::erase(suppressed, static_cast<long>(index));
    }
    std::ranges::sort(suppressed);
    const auto duplicates = std::ranges::unique(suppressed);
    suppressed.erase(duplicates.begin(), duplicates.end());
    SuppressedIndices.setValues(suppressed);
}

const std::list<gp_Trsf> Transformed::getFilteredTransformations(
    const std::vector<App::DocumentObject*> originals
)
{
    std::list<gp_Trsf> filtered;
    int index = 0;
    for (const auto& transformation : getTransformations(originals)) {
        if (!isTransformationSuppressed(index)) {
            filtered.push_back(transformation);
        }
        ++index;
    }

    return filtered;
}

App::DocumentObjectExecReturn* Transformed::recomputePreview()
{
    const auto mode = static_cast<Mode>(TransformMode.getValue());

    const Part::Feature* supportFeature = getBaseObject();
    const Part::TopoShape supportShape = supportFeature->Shape.getShape();

    if (supportShape.isNull()) {
        return App::DocumentObject::StdReturn;
    }

    gp_Trsf trsfInv = supportShape.getShape().Location().Transformation().Inverted();

    auto originals = getOriginals();
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
        return App::DocumentObject::StdReturn;
    }

    const auto makeCompoundOfToolShapes = [&]() {
        BRep_Builder builder;
        TopoDS_Compound compound;

        builder.MakeCompound(compound);
        for (const auto& original : originals) {
            if (auto* feature = freecad_cast<FeatureAddSub*>(original)) {
                auto shape = feature->AddSubShape.getShape();

                gp_Trsf trsf = trsfInv.Multiplied(feature->getLocation().Transformation());

                if (shape.isNull()) {
                    continue;
                }

                shape.makeElementTransform(shape, trsf);

                builder.Add(compound, shape.getShape());
            }
        }

        return compound;
    };

    switch (mode) {
        case Mode::FeatureResult: {
            std::vector<FeatureShape> shapes;
            App::DocumentObjectExecReturn* ret = computeFeatureShapes(trsfInv, originals, shapes);
            if (ret) {
                return ret;
            }
            BRep_Builder builder;
            TopoDS_Compound compound;

            builder.MakeCompound(compound);
            for (const auto& s : shapes) {
                builder.Add(compound, s.shape.getShape());
            }

            PreviewShape.setValue(compound);
            return StdReturn;
        }

        case Mode::Features: {
            PreviewShape.setValue(makeCompoundOfToolShapes());
            return StdReturn;
        }

        case Mode::WholeShape: {
            auto shape = getBaseTopoShape();
            shape = shape.makeElementTransform(trsfInv);

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

    if ((mode == Mode::Features || mode == Mode::FeatureResult) && originals.empty()) {
        return App::DocumentObject::StdReturn;
    }

    if (!this->BaseFeature.getValue()) {
        if (auto body = getFeatureBody()) {
            body->setBaseProperty(this);
        }
    }

    this->positionBySupport();

    // Get transformations from subclass by calling virtual method.
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
        return App::DocumentObject::StdReturn;
    }

    // Get the support.
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

    const gp_Trsf trsfInv = supportTopShape.getShape().Location().Transformation().Inverted();

    // Create an untransformed copy of the support shape.
    Part::TopoShape supportShape(supportTopShape);
    Part::TopoShape wholeShapeSource(supportTopShape);

    supportShape.setTransform(Base::Matrix4D());
    wholeShapeSource.setTransform(Base::Matrix4D());

    if (!supportShape.isValid()) {
        return new App::DocumentObjectExecReturn(
            QT_TRANSLATE_NOOP("Exception", "Cannot transform invalid support shape")
        );
    }

    // This is the support before the original transformation.
    //
    // When the original occurrence is suppressed, remove the material
    // contributed by the transformed feature. This is needed because the
    // support already contains occurrence zero.
    const auto transformToSupport = [&trsfInv](Part::TopoShape shape) {
        if (shape.isNull()) {
            return shape;
        }

        const gp_Trsf location = shape.getShape().Location().Transformation();

        shape.setTransform(Base::Matrix4D());

        return shape.makeElementTransform(trsfInv.Multiplied(location));
    };

    if (!hasOriginalTransformation() || isTransformationSuppressed(0)) {
        if (mode == Mode::WholeShape) {
            supportShape.setShape(TopoDS_Shape());
        }
        else {
            const auto sortedOriginals = getSortedOriginals();

            for (auto it = sortedOriginals.rbegin(); it != sortedOriginals.rend(); ++it) {

                auto* feature = freecad_cast<FeatureAddSub*>(*it);
                if (!feature) {
                    continue;
                }

                Part::TopoShape before = transformToSupport(feature->getBaseTopoShape(true));

                Part::TopoShape after = transformToSupport(feature->Shape.getShape());

                Part::TopoShape delta;

                if (feature->getAddSubType() == FeatureAddSub::Type::Additive) {
                    if (before.isNull()) {
                        delta = after;
                    }
                    else {
                        delta.makeElementCut({after, before});
                    }

                    if (!delta.isNull() && !supportShape.isNull()) {
                        supportShape.makeElementCut({supportShape, delta});
                    }
                }
                else if (!before.isNull()) {
                    delta.makeElementCut({before, after});

                    if (!delta.isNull()) {
                        if (supportShape.isNull()) {
                            supportShape = delta;
                        }
                        else {
                            supportShape.makeElementFuse({supportShape, delta});
                        }
                    }
                }
            }
        }
    }

    App::DocumentObjectExecReturn* result = nullptr;

    switch (mode) {
        case Mode::Features:
            result = executeFeatures(trsfInv, transformations, supportShape, originals);
            break;

        case Mode::FeatureResult:
            result = executeFeatureResult(trsfInv, transformations, supportShape, originals);
            break;

        case Mode::WholeShape:
            result = executeWholeBody(transformations, supportShape);
            break;

        default:
            result = new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Invalid mode."));
            break;
    }

    if (result) {
        return result;
    }

    if (supportShape.isNull()) {
        this->Shape.setValue(TopoDS_Shape());
        rejected.Nullify();
        return App::DocumentObject::StdReturn;
    }

    if (!supportShape.isValid()) {
        return new App::DocumentObjectExecReturn(
            QT_TRANSLATE_NOOP("Exception", "Resulting shape is invalid.")
        );
    }

    supportShape = refineShapeIfActive(supportShape);

    this->Shape.setValue(getSolid(supportShape));

    if (singleSolidRuleMode() == SingleSolidRuleMode::Enforced
        && supportShape.countSubShapes(TopAbs_SOLID) > 0) {
        rejected = getRemainingSolids(supportShape.getShape());
    }
    else {
        rejected.Nullify();
    }

    return App::DocumentObject::StdReturn;
}

App::DocumentObjectExecReturn* Transformed::executeFeatures(
    const gp_Trsf& trsfInv,
    const std::vector<gp_Trsf>& transformations,
    Part::TopoShape& supportShape,
    const std::vector<DocumentObject*>& originals
)
{
    for (auto original : originals) {
        Part::TopoShape addShape;
        Part::TopoShape subShape;
        FeatureAddSub::BooleanOperation booleanOperation;

        auto* feature = freecad_cast<Feature*>(original);

        if (auto* result = extractFeature(feature, addShape, subShape, booleanOperation)) {
            return result;
        }

        if (addShape.isNull() && subShape.isNull()) {
            return new App::DocumentObjectExecReturn(
                QT_TRANSLATE_NOOP("Exception", "Shape of additive/subtractive feature is empty")
            );
        }

        gp_Trsf trsf = trsfInv.Multiplied(feature->getLocation().Transformation());

        if (!addShape.isNull()) {
            addShape = addShape.makeElementTransform(
                addShape,
                trsf,
                std::format("Transform_add_{}", feature->getNameInDocument()).c_str()
            );
        }

        if (!subShape.isNull()) {
            subShape = subShape.makeElementTransform(
                subShape,
                trsf,
                std::format("Transform_sub_{}", feature->getNameInDocument()).c_str()
            );
        }

        if (!addShape.isNull()) {
            auto shapes = getTransformedCompShape(transformations, supportShape, addShape);

            if (Base::Sequencer().wasCanceled()) {
                return new App::DocumentObjectExecReturn("User aborted");
            }

            supportShape.makeElementFuse(
                shapes,
                std::format("Fuse_add_{}-{}", feature->getNameInDocument(), shapes.size()).c_str()
            );
        }

        if (!subShape.isNull()) {
            auto shapes = getTransformedCompShape(transformations, supportShape, subShape);

            if (Base::Sequencer().wasCanceled()) {
                return new App::DocumentObjectExecReturn("User aborted");
            }

            supportShape.makeElementCut(
                shapes,
                std::format("Cut_sub_{}-{}", feature->getNameInDocument(), shapes.size()).c_str()
            );
        }
    }

    return nullptr;
}

App::DocumentObjectExecReturn* Transformed::extractFeature(
    Feature* feature,
    Part::TopoShape& addShape,
    Part::TopoShape& subShape,
    FeatureAddSub::BooleanOperation& op
)
{
    if (!feature) {
        return new App::DocumentObjectExecReturn(
            QT_TRANSLATE_NOOP("Exception", "Feature is not supported")
        );
    }

    if (feature->isDerivedFrom<FeatureAddSub>()) {
        auto* addSub = freecad_cast<FeatureAddSub*>(feature);
        addSub->getAddSubShape(addShape, subShape);
        op = addSub->getBooleanOperation();
    }
    else if (feature->isDerivedFrom<Boolean>()) {
        auto* boolean = freecad_cast<Boolean*>(feature);
        boolean->getAddSubShape(addShape, subShape);
        op = boolean->getBooleanOperation();
    }
    else {
        return new App::DocumentObjectExecReturn(
            QT_TRANSLATE_NOOP("Exception", "Feature is not supported")
        );
    }

    return nullptr;
}

App::DocumentObjectExecReturn* Transformed::computeFeatureShapes(
    const gp_Trsf& trsfInv,
    const std::vector<DocumentObject*>& originals,
    std::vector<FeatureShape>& shapes
)
{
    auto checkValidShape = [](const TopoShape& shape, std::string_view text, auto&&... args) {
        if (!shape.isValid()) {
            std::ostringstream details;
            shape.analyze(false, details);

            std::string message = "Invalid shape after ";
            message += std::vformat(text, std::make_format_args(args...));

            if (!details.str().empty()) {
                message += ":\n";
                message += details.str();
            }

            FC_THROWM(Base::CADKernelError, message.c_str());
        }
    };

    for (auto original : originals) {
        Part::TopoShape addShape;
        Part::TopoShape subShape;

        auto* feature = freecad_cast<Feature*>(original);
        FeatureAddSub::BooleanOperation booleanOperation;

        if (auto* result = extractFeature(feature, addShape, subShape, booleanOperation)) {
            return result;
        }

        if (addShape.isNull() && subShape.isNull()) {
            return new App::DocumentObjectExecReturn(
                QT_TRANSLATE_NOOP("Exception", "Shape of additive/subtractive feature is empty")
            );
        }

        const auto* prevFeature = feature->getBaseObject(true);

        std::optional<Part::TopoShape> prevShape;

        gp_Trsf trsf = trsfInv.Multiplied(feature->getLocation().Transformation());

        if (prevFeature) {
            prevShape.emplace(feature->getBaseShape());
        }

        if (!addShape.isNull()) {
            addShape = addShape.makeElementTransform(
                addShape,
                trsf,
                std::format("Transform_add_{}", feature->getNameInDocument()).c_str()
            );

            if (prevFeature) {
                addShape = addShape.makeElementCut(
                    {addShape, *prevShape},
                    std::format(
                        "Cut_add_{}-{}",
                        feature->getNameInDocument(),
                        prevFeature->getNameInDocument()
                    )
                        .c_str()
                );
            }

            if (!addShape.isNull()) {
                if (prevFeature) {
                    checkValidShape(
                        addShape,
                        "CUT {}-{}",
                        feature->getNameInDocument(),
                        prevFeature->getNameInDocument()
                    );
                }

                shapes.push_back({feature->getNameInDocument(), addShape, Operation::Add});
            }
        }

        if (!subShape.isNull()) {
            if (!prevFeature) {
                continue;
            }

            subShape = subShape.makeElementTransform(
                subShape,
                trsf,
                std::format("Transform_sub_{}", feature->getNameInDocument()).c_str()
            );

            std::vector<Part::TopoShape> subShapes;
            TopoShape::expandCompound(subShape, subShapes);

            size_t i = 0;

            for (auto& s : subShapes) {
                if (booleanOperation == FeatureAddSub::BooleanOperation::Common) {

                    s = s.makeElementCut(
                        {*prevShape, s},
                        std::format(
                            "Cut_cmn_{}*{}[{}]",
                            prevFeature->getNameInDocument(),
                            feature->getNameInDocument(),
                            i
                        )
                            .c_str()
                    );
                }
                else {
                    s = s.makeElementCommon(
                        {*prevShape, s},
                        std::format(
                            "Common_sub_{}[{}]*{}",
                            feature->getNameInDocument(),
                            i,
                            prevFeature->getNameInDocument()
                        )
                            .c_str()
                    );
                }

                if (!s.isNull()) {
                    checkValidShape(
                        s,
                        "COMMON {}[{}]*{}",
                        feature->getNameInDocument(),
                        i,
                        prevFeature->getNameInDocument()
                    );

                    shapes.push_back(
                        {std::format("{}[{}]", feature->getNameInDocument(), i), s, Operation::Sub}
                    );
                }

                ++i;
            }
        }

        if (Base::Sequencer().wasCanceled()) {
            return new App::DocumentObjectExecReturn("User aborted");
        }
    }

    return nullptr;
}

App::DocumentObjectExecReturn* Transformed::executeFeatureResult(
    const gp_Trsf& trsfInv,
    const std::vector<gp_Trsf>& transformations,
    Part::TopoShape& supportShape,
    const std::vector<DocumentObject*>& originals
)
{
    const auto verifyShape = [](const Part::TopoShape& shape, std::string_view text, auto&&... args) {
        if (!shape.isValid()) {
            std::ostringstream details;
            shape.analyze(false, details);

            std::string message = std::vformat(text, std::make_format_args(args...));

            if (!details.str().empty()) {
                message += "\n";
                message += details.str();
            }

            FC_THROWM(Base::CADKernelError, message.c_str());
        }
    };

    verifyShape(supportShape, "Initial support shape invalid.");

    std::vector<FeatureShape> shapes;

    if (auto* result = computeFeatureShapes(trsfInv, originals, shapes)) {
        return result;
    }

    verifyShape(supportShape, "Invalid support shape after computing feature shapes.");

    for (auto& element : shapes) {
        verifyShape(
            element.shape,
            "Invalid minimum feature shape for {} [{}]",
            element.source,
            element.operation == Operation::Add ? "ADD" : "SUB"
        );

        auto transformedShapes = getTransformedCompShape(transformations, supportShape, element.shape);

        if (Base::Sequencer().wasCanceled()) {
            return new App::DocumentObjectExecReturn("User aborted");
        }

        switch (element.operation) {
            case Operation::Add:
                supportShape.makeElementFuse(
                    transformedShapes,
                    std::format("Fuse_add_{}", element.source).c_str()
                );
                break;

            case Operation::Sub:
                supportShape.makeElementCut(
                    transformedShapes,
                    std::format("Cut_sub_{}", element.source).c_str()
                );
                break;

            default:
                return new App::DocumentObjectExecReturn("Invalid operation.");
        }

        verifyShape(
            supportShape,
            "Invalid shape after applying boolean for {} [{}]",
            element.source,
            element.operation == Operation::Add ? "ADD" : "SUB"
        );
    }

    if (Base::Sequencer().wasCanceled()) {
        return new App::DocumentObjectExecReturn("User aborted");
    }

    return nullptr;
}

App::DocumentObjectExecReturn* Transformed::executeWholeBody(
    const std::vector<gp_Trsf>& transformations,
    Part::TopoShape& supportShape
)
{
    auto shapes = getTransformedCompShape(transformations, supportShape, supportShape);

    if (Base::Sequencer().wasCanceled()) {
        return new App::DocumentObjectExecReturn("User aborted");
    }

    supportShape.makeElementFuse(
        shapes,
        std::format(
            "Fuse_add_{}",
            this->getFeatureBody() == nullptr ? "<no body>"
                                              : this->getFeatureBody()->getNameInDocument()
        )
            .c_str()
    );

    return nullptr;
}

std::vector<TopoShape> Transformed::getTransformedCompShape(
    const std::vector<gp_Trsf>& transformations,
    const Part::TopoShape& supportShape,
    const Part::TopoShape& origShape
)
{
    std::vector<TopoShape> shapes;

    if (!supportShape.isNull()) {
        shapes.push_back(supportShape);
    }

    TopoShape shape(origShape);

    // If the original feature already exists in the support, transformation
    // zero represents that existing occurrence and must not be generated again.
    int idx = hasOriginalTransformation() ? 1 : 0;

    auto transformIter = transformations.cbegin();

    std::advance(transformIter, std::min(idx, static_cast<int>(transformations.size())));

    for (; transformIter != transformations.end(); ++transformIter) {
        if (Base::Sequencer().wasCanceled()) {
            return {};
        }

        if (isTransformationSuppressed(idx)) {
            ++idx;
            continue;
        }

        auto opName = Data::indexSuffix(idx++);

        shapes.emplace_back(shape.makeElementTransform(*transformIter, opName.c_str()));
    }

    return shapes;
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
