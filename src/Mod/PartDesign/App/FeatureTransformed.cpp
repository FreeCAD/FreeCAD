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

#include "PreCompiled.h"
#ifndef _PreComp_
#include <Bnd_Box.hxx>
#include <BRep_Builder.hxx>
#include <Mod/Part/App/FCBRepAlgoAPI_Cut.h>
#include <Mod/Part/App/FCBRepAlgoAPI_Fuse.h>
#include <BRepBndLib.hxx>
#include <BRepBuilderAPI_Copy.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <Precision.hxx>
#include <TopExp_Explorer.hxx>
#endif

#include <array>

#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/ProgressIndicator.h>
#include <Base/Reader.h>
#include <Mod/Part/App/modelRefine.h>

#include "FeatureTransformed.h"
#include "Body.h"
#include "FeatureAddSub.h"
#include "FeatureMultiTransform.h"
#include "FeatureMirrored.h"
#include "FeatureLinearPattern.h"
#include "FeaturePolarPattern.h"
#include "FeatureSketchBased.h"
#include "Mod/Part/App/TopoShapeOpCode.h"
#include "Mod/Part/App/OCCTProgressIndicator.h"


using namespace PartDesign;

namespace PartDesign
{
using Part::OCCTProgressIndicator;
extern bool getPDRefineModelParameter();

PROPERTY_SOURCE(PartDesign::Transformed, PartDesign::FeatureRefine)

std::array<char const*, 3> transformModeEnums = {"Transform tool shapes",
                                                 "Transform body",
                                                 nullptr};

Transformed::Transformed()
{
    ADD_PROPERTY(Originals, (nullptr));
    Originals.setSize(0);
    Placement.setStatus(App::Property::ReadOnly, true);

    ADD_PROPERTY(TransformMode, (static_cast<long>(Mode::TransformToolShapes)));
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
    const std::vector<App::DocumentObject*>& originals = Originals.getValues();
    // NOTE: may be here supposed to be last origin but in order to keep the old behaviour keep here
    // first
    App::DocumentObject* firstOriginal = originals.empty() ? nullptr : originals.front();
    if (firstOriginal) {
        rv = freecad_cast<Part::Feature*>(firstOriginal);
        if (!rv) {
            err = QT_TRANSLATE_NOOP("Exception",
                                    "Transformation feature Linked object is not a Part object");
        }
    }
    else {
        err = QT_TRANSLATE_NOOP("Exception", "No originals linked to the transformed feature.");
    }

    if (!silent && err) {
        throw Base::RuntimeError(err);
    }

    return rv;
}

App::DocumentObject* Transformed::getSketchObject() const
{
    std::vector<DocumentObject*> originals = Originals.getValues();
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

    // instead check for default property values because these are invalid for a standalone transform feature.
    // This will mislabel standalone features during the initialization phase.
    if (TransformMode.getValue() == 0 && Originals.getValue().empty()) {
        return true;
    }

    return false;
}

void Transformed::handleChangedPropertyType(Base::XMLReader& reader,
                                            const char* TypeName,
                                            App::Property* prop)
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
    return PartDesign::Feature::mustExecute();
}

App::DocumentObjectExecReturn* Transformed::execute()
{
    if (isMultiTransformChild()) {
        return App::DocumentObject::StdReturn;
    }

    std::vector<App::DocumentObject*> originals;
    auto const mode = static_cast<Mode>(TransformMode.getValue());
    if (mode == Mode::TransformBody) {
        Originals.setStatus(App::Property::Status::Hidden, true);
    } else {
        Originals.setStatus(App::Property::Status::Hidden, false);
        originals = Originals.getValues();
    }
    // Remove suppressed features from the list so the transformations behave as if they are not
    // there
    auto eraseIter =
        std::remove_if(originals.begin(), originals.end(), [](App::DocumentObject const* obj) {
            auto feature = freecad_cast<PartDesign::Feature*>(obj);
            return feature != nullptr && feature->Suppressed.getValue();
        });
    originals.erase(eraseIter, originals.end());

    if (mode == Mode::TransformToolShapes && originals.empty()) {
        return App::DocumentObject::StdReturn;
    }

    if (!this->BaseFeature.getValue()) {
        auto body = getFeatureBody();
        if (body) {
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
            QT_TRANSLATE_NOOP("Exception", "Cannot transform invalid support shape"));
    }

    // create an untransformed copy of the support shape
    Part::TopoShape supportShape(supportTopShape);

    gp_Trsf trsfInv = supportShape.getShape().Location().Transformation().Inverted();

    supportShape.setTransform(Base::Matrix4D());

    auto getTransformedCompShape = [&](const auto& supportShape, const auto& origShape) {
        std::vector<TopoShape> shapes = {supportShape};
        TopoShape shape (origShape);
        int idx=1;
        auto transformIter = transformations.cbegin();
        transformIter++;
        for ( ; transformIter != transformations.end(); transformIter++) {
            if (OCCTProgressIndicator::getAppIndicator().UserBreak()) {
                return std::vector<TopoShape>();
            }
            auto opName = Data::indexSuffix(idx++);
            shapes.emplace_back(shape.makeElementTransform(*transformIter, opName.c_str()));
        }
        return shapes;
    };

    switch (mode) {
        case Mode::TransformToolShapes:
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
                        "Only additive and subtractive features can be transformed"));
                }

                feature->getAddSubShape(fuseShape, cutShape);
                if (fuseShape.isNull() && cutShape.isNull()) {
                    return new App::DocumentObjectExecReturn(
                        QT_TRANSLATE_NOOP("Exception",
                                          "Shape of additive/subtractive feature is empty"));
                }
                gp_Trsf trsf = feature->getLocation().Transformation().Multiplied(trsfInv);
                if (!fuseShape.isNull()) {
                    fuseShape = fuseShape.makeElementTransform(trsf);
                }
                if (!cutShape.isNull()) {
                    cutShape = cutShape.makeElementTransform(trsf);
                }
                if (!fuseShape.isNull()) {
                    auto shapes = getTransformedCompShape(supportShape, fuseShape);
                    if (OCCTProgressIndicator::getAppIndicator().UserBreak()) {
                        return new App::DocumentObjectExecReturn("User aborted");
                    }
                    supportShape.makeElementFuse(shapes);
                }
                if (!cutShape.isNull()) {
                    auto shapes = getTransformedCompShape(supportShape, cutShape);
                    if (OCCTProgressIndicator::getAppIndicator().UserBreak()) {
                        return new App::DocumentObjectExecReturn("User aborted");
                    }
                    supportShape.makeElementCut(shapes);
                }
            }
            break;
        case Mode::TransformBody: {
            auto shapes = getTransformedCompShape(supportShape, supportShape);
            if (OCCTProgressIndicator::getAppIndicator().UserBreak()) {
                return new App::DocumentObjectExecReturn("User aborted");
            }
            supportShape.makeElementFuse(shapes);
            break;
        }
    }

    supportShape = refineShapeIfActive((supportShape));
    if (!isSingleSolidRuleSatisfied(supportShape.getShape())) {
        Base::Console().warning("Transformed: Result has multiple solids. Only keeping the first.\n");
    }

    this->Shape.setValue(getSolid(supportShape));  // picking the first solid
    rejected = getRemainingSolids(supportShape.getShape());

    return App::DocumentObject::StdReturn;
}

TopoDS_Shape Transformed::getRemainingSolids(const TopoDS_Shape& shape)
{
    BRep_Builder builder;
    TopoDS_Compound compShape;
    builder.MakeCompound(compShape);

    if (shape.IsNull()) {
        Standard_Failure::Raise("Shape is null");
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
