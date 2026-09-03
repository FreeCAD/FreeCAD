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
#include "FeatureMirrored.h"
#include "FeaturePolarPattern.h"
#include "FeatureSketchBased.h"
#include "FeatureTransformed.h"

using namespace PartDesign;

namespace PartDesign
{
extern bool getPDRefineModelParameter();

PROPERTY_SOURCE(PartDesign::Transformed, PartDesign::FeatureRefine)

std::array<char const*, 4> transformModeEnums = {"Features", "Whole Body", "Feature Result", nullptr};

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
    if (Originals.isTouched() || TransformMode.isTouched()) {
        return 1;
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

                gp_Trsf trsf = supportTransform.Inverted().Multiplied(
                    feature->getLocation().Transformation()
                );

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
            std::vector<FeatureShape> shapes(originals.size());
            App::DocumentObjectExecReturn* ret = computeFeatureShapes(supportShape, originals, shapes);
            if (ret) {
                return ret;
            }
            std::vector<TopoShape> compoundShapes(shapes.size());
            for (auto s : shapes) {
                compoundShapes.push_back(s.shape);
            }
            PreviewShape.setValue(TopoShape().makeCompound(compoundShapes));
            return StdReturn;
        }

        case Mode::Features: {
            PreviewShape.setValue(makeCompoundOfToolShapes());
            return StdReturn;
        }

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

    if (mode != Mode::WholeShape && originals.empty()) {
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
        return App::DocumentObject::StdReturn;  // No transformations defined, exit
                                                // silently
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

    // create an untransformed copy of the support shape
    Part::TopoShape supportShape(supportTopShape);
    supportShape.setTransform(Base::Matrix4D());

    App::DocumentObjectExecReturn* result = nullptr;
    switch (mode) {
        case Mode::Features: {
            result = executeFeatures(transformations, supportShape, originals);
            break;
        }

        case Mode::FeatureResult: {
            result = executeFeatureResult(transformations, supportShape, originals);
            break;
        }

        case Mode::WholeShape: {
            result = executeWholeBody(transformations, supportShape, originals);
            break;
        }

        default:
            result = new App::DocumentObjectExecReturn("Invalid mode.");
    }

    if (result) {
        return result;
    }

    supportShape = refineShapeIfActive(supportShape);

    this->Shape.setValue(getSolid(supportShape));
    if (singleSolidRuleMode() == SingleSolidRuleMode::Enforced) {
        rejected = getRemainingSolids(supportShape.getShape());
    }
    else {
        rejected.Nullify();
    }

    return App::DocumentObject::StdReturn;
}

App::DocumentObjectExecReturn* Transformed::executeFeatures(
    const std::vector<gp_Trsf>& transformations,
    Part::TopoShape& supportShape,
    const std::vector<DocumentObject*>& originals
)
{
    gp_Trsf trsfInv = supportShape.getShape().Location().Transformation().Inverted();

    for (auto original : originals) {
        Part::TopoShape addShape;
        Part::TopoShape subShape;

        auto feature = freecad_cast<PartDesign::FeatureAddSub*>(original);
        if (!feature) {
            return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP(
                "Exception",
                "Only additive and subtractive features can be transformed"
            ));
        }

        feature->getAddSubShape(addShape, subShape);
        if (addShape.isNull() && subShape.isNull()) {
            return new App::DocumentObjectExecReturn(
                QT_TRANSLATE_NOOP("Exception", "Shape of additive/subtractive feature is empty")
            );
        }
        gp_Trsf trsf = trsfInv.Multiplied(feature->getLocation().Transformation());
        if (!addShape.isNull()) {
            addShape.makeElementTransform(
                addShape,
                trsf,
                std::format("Transform_add_{}", feature->getNameInDocument()).c_str()
            );
        }
        if (!subShape.isNull()) {
            subShape.makeElementTransform(
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

App::DocumentObjectExecReturn* Transformed::computeFeatureShapes(
    const Part::TopoShape& supportShape,
    const std::vector<DocumentObject*>& originals,
    std::vector<FeatureShape>& shapes
)
{

    // compute the difference solid between each Feature and the shape of its previous Feature,
    // * for additive operations, we take (toolShape-previousShape) and use it to Fuse later.
    // * for subtractive operations, we take the (toolShape ∩ previousShape) and use it to Cut
    // later.
    // Then apply them in order to the shape from the Feature before this FeatureTransformed.
    gp_Trsf trsfInv = supportShape.getShape().Location().Transformation().Inverted();

    auto getPreviousOriginal = [&](DocumentObject* original) -> PartDesign::Feature* {
        auto body = getFeatureBody();
        if (!body) {
            return nullptr;
        }

        const auto& group = body->Group.getValues();

        auto it = std::ranges::find(group, original);
        if (it == group.end()) {
            return nullptr;
        }

        while (it != group.begin()) {
            --it;

            if (auto feature = freecad_cast<PartDesign::Feature*>(*it)) {
                return feature;
            }
        }

        return nullptr;
    };

    for (auto original : originals) {
        Part::TopoShape addShape;
        Part::TopoShape subShape;

        auto feature = freecad_cast<PartDesign::FeatureAddSub*>(original);
        if (!feature) {
            return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP(
                "Exception",
                "Only additive and subtractive features can be transformed"
            ));
        }

        feature->getAddSubShape(addShape, subShape);

        if (addShape.isNull() && subShape.isNull()) {
            return new App::DocumentObjectExecReturn(
                QT_TRANSLATE_NOOP("Exception", "Shape of additive/subtractive feature is empty")
            );
        }

        auto prevFeature = getPreviousOriginal(original);
        auto prevShape = prevFeature != nullptr ? prevFeature->Shape.getShape() : NULL;

        gp_Trsf trsf = trsfInv.Multiplied(feature->getLocation().Transformation());
        if (!addShape.isNull()) {
            addShape.makeElementTransform(
                addShape,
                trsf,
                std::format("Transform_add_{}", feature->getNameInDocument()).c_str()
            );
            if (prevShape != NULL) {
                addShape.makeElementCut(
                    {addShape, prevShape},
                    std::format(
                        "Cut_add_{}-{}",
                        feature->getNameInDocument(),
                        prevFeature->getNameInDocument()
                    )
                        .c_str()
                );
            }

            if (!addShape.isNull()) {
                shapes.push_back({feature->getNameInDocument(), addShape, Operation::Add});
            }
        }

        if (!subShape.isNull()) {
            if (!prevFeature) {
                continue;
                // skip this feature if it is subtractive with nothing to subtract it from
            }

            subShape.makeElementTransform(
                subShape,
                trsf,
                std::format("Transform_sub_{}", feature->getNameInDocument()).c_str()
            );

            // we need to unwrap the pocket toolShapes because
            // (COMMON(subShape = compounds(A, B), prevShape = C) = A ∩ B ∩ C)
            // returns an empty solid if A and B don't intersect.
            std::vector<Part::TopoShape> subShapes;
            if (subShape.shapeType() == TopAbs_COMPOUND) {
                TopoShape::expandCompound(subShape, subShapes);
            }
            else {
                subShapes.push_back(subShape);
            }

            size_t i = 0;
            for (auto s : subShapes) {
                s.makeElementCommon(
                    {s, prevShape},
                    std::format(
                        "Common_sub_{}[{}]+{}",
                        feature->getNameInDocument(),
                        i,
                        prevFeature->getNameInDocument()
                    )
                        .c_str()
                );

                if (!s.isNull()) {
                    // TODO: We could merge them back together ?
                    shapes.push_back(
                        {std::format("{}[{}]", feature->getNameInDocument(), i), s, Operation::Sub}
                    );
                }

                i++;
            }
        }

        if (Base::Sequencer().wasCanceled()) {
            return new App::DocumentObjectExecReturn("User aborted");
        }
    }

    return nullptr;
}

App::DocumentObjectExecReturn* Transformed::executeFeatureResult(
    const std::vector<gp_Trsf>& transformations,
    Part::TopoShape& supportShape,
    const std::vector<DocumentObject*>& originals
)
{
    std::vector<FeatureShape> shapes;
    auto* ret = computeFeatureShapes(supportShape, originals, shapes);

    if (ret) {
        return ret;
    }


    for (auto& element : shapes) {
        auto transformedShapes = getTransformedCompShape(transformations, supportShape, element.shape);

        if (Base::Sequencer().wasCanceled()) {
            return new App::DocumentObjectExecReturn("User aborted");
        }

        switch (element.operation) {
            case Operation::Add:
                supportShape.makeElementFuse(
                    transformedShapes,
                    std::format("Fuse_add_+{}", element.source).c_str()
                );
                break;

            case Operation::Sub:
                supportShape.makeElementCut(
                    transformedShapes,
                    std::format("Cut_sub_-{}", element.source).c_str()
                );
                break;

            default:
                return new App::DocumentObjectExecReturn("Invalid operation.");
        }
    }

    if (Base::Sequencer().wasCanceled()) {
        return new App::DocumentObjectExecReturn("User aborted");
    }

    return nullptr;
}

App::DocumentObjectExecReturn* Transformed::executeWholeBody(
    const std::vector<gp_Trsf>& transformations,
    Part::TopoShape& supportShape,
    const std::vector<DocumentObject*>& originals
)
{
    auto shapes = getTransformedCompShape(transformations, supportShape, supportShape);
    if (Base::Sequencer().wasCanceled()) {
        return new App::DocumentObjectExecReturn("User aborted");
    }
    supportShape.makeElementFuse(
        shapes,
        std::format(
            "Fuse_add_-{}",
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
    std::vector<TopoShape> shapes = {supportShape};

    TopoShape shape(origShape);

    int idx = 1;
    auto transformIter = transformations.cbegin();

    // ignore first instance
    if (transformIter != transformations.end()) {
        transformIter++;
    }

    for (; transformIter != transformations.end(); transformIter++) {
        if (Base::Sequencer().wasCanceled()) {
            return std::vector<TopoShape>();
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
