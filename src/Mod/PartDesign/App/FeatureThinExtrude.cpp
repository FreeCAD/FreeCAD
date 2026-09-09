// SPDX-License-Identifier: LGPL-2.1-or-later
#include "FeatureThinExtrude.h"
#include "ThinExtrusionGeometry.h"
#include "ThinSurfaceExtrusion.h"
#include "ThinExtrusion.h"
#include "ThinProfile.h"
#include <Base/Tools.h>

#include <App/Document.h>
#include <Base/Converter.h>
#include <Base/Exception.h>
#include <Precision.hxx>
#include <Mod/Part/App/Tools.h>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRep_Tool.hxx>
#include <TopoDS.hxx>

namespace PartDesign
{
PROPERTY_SOURCE(PartDesign::ThinExtrude, PartDesign::Pad)

ThinExtrude::ThinExtrude()
{
    static const char* modes[] = {"Web", "Rib", "SpatialWeb", nullptr};
    static const char* extensions[] = {"Off", "Tangent", "Natural", nullptr};

    ADD_PROPERTY_TYPE(
        RibMode,
        (0L),
        "Rib",
        App::Prop_None,
        "Plan-profile web, side-profile rib, or spatial web"
    );
    RibMode.setEnums(modes);

    ADD_PROPERTY_TYPE(
        Extension,
        (0L),
        "Rib",
        App::Prop_None,
        "Extend dangling profile endpoints to the body"
    );
    Extension.setEnums(extensions);

    ADD_PROPERTY_TYPE(
        AutoDirection,
        (false),
        "Rib",
        App::Prop_None,
        "Automatically grow toward the previous body in the profile plane"
    );

    ADD_PROPERTY_TYPE(
        DraftPullDirection,
        (nullptr),
        "Rib",
        App::Prop_None,
        "Geometry defining the draft pull axis/normal or the target of Toward Reference"
    );

    static const char* pullModes[] = {
        "Automatic",
        "TowardParent",
        "TowardReference",
        "ParallelToReference",
        "Vector",
        "ProfileLocalVector",
        nullptr
    };

    ADD_PROPERTY_TYPE(
        DraftPullMode,
        (0L),
        "Rib",
        App::Prop_None,
        "Draft pull direction mode; Automatic preserves root-to-free-end pull"
    );
    DraftPullMode.setEnums(pullModes);

    ADD_PROPERTY_TYPE(
        DraftPullVector,
        (Base::Vector3d(0, 0, 1)),
        "Rib",
        App::Prop_None,
        "Explicit draft pull vector in world or profile-local coordinates"
    );

    ADD_PROPERTY_TYPE(
        FlipPullDirection,
        (false),
        "Rib",
        App::Prop_None,
        "Reverse draft pull direction without reversing rib growth"
    );

    ADD_PROPERTY_TYPE(
        TowardReference,
        (false),
        "Rib",
        App::Prop_None,
        "Grow toward the nearest point on ReferenceAxis from the profile's length-weighted center, "
        "instead of parallel to it"
    );

    ADD_PROPERTY_TYPE(
        FillDirection,
        (Base::Vector3d(0, -1, 0)),
        "Rib",
        App::Prop_None,
        "Rib/spatial growth direction in profile-local coordinates"
    );

    ADD_PROPERTY_TYPE(
        EdgeFilletRadius,
        (0.0),
        "Rib",
        App::Prop_None,
        "Exposed-edge fillet radius; zero disables"
    );

    Thin.setValue(true);
    Thin.setReadOnly(true);
    ThinSide.setValue("Centered");
}

short ThinExtrude::mustExecute() const
{
    if (RibMode.isTouched() || FillDirection.isTouched() || RootFilletRadius.isTouched()
        || EdgeFilletRadius.isTouched() || Extension.isTouched() || TowardReference.isTouched()
        || AutoDirection.isTouched() || DraftPullDirection.isTouched()
        || FlipPullDirection.isTouched() || DraftPullMode.isTouched() || DraftPullVector.isTouched()) {
        return 1;
    }

    return Pad::mustExecute();
}

std::optional<Base::Vector3d> ThinExtrude::getDraftPullVector() const
{
    const long mode = DraftPullMode.getValue();
    auto reference = DraftPullDirection.getValue();
    if (mode == 0) {
        return std::nullopt;
    }

    if (mode == 1) {
        return Base::convertTo<Base::Vector3d>(thinDirectionTowardBody(
            getThinInput(),
            getBaseTopoShape(true),
            Base::convertTo<gp_Dir>(getProfileNormal())
        ));
    }

    if (mode == 4 || mode == 5) {
        const auto value = DraftPullVector.getValue();
        if (value.Length() <= Precision::Confusion()) {
            throw Base::ValueError("Draft pull vector must not be zero");
        }
        auto direction = Base::convertTo<gp_Dir>(value);
        if (mode == 5) {
            direction.Transform(getVerifiedObject()->getLocation().Transformation());
        }
        return Base::convertTo<Base::Vector3d>(direction);
    }

    if (!reference) {
        throw Base::ValueError("Select a reference for Draft Pull Direction");
    }

    return referenceDirection(reference, DraftPullDirection.getSubValues(), mode == 2);
}

Base::Vector3d ThinExtrude::referenceDirection(
    const App::DocumentObject* object,
    const std::vector<std::string>& names,
    bool toward
) const
{
    if (!object || names.size() > 1) {
        throw Base::ValueError("Select one reference object or subelement");
    }

    const std::string name = names.empty() ? std::string() : names.front();

    // Retain sketch H/V/N and construction axes, which are not shape subelements.
    if (!toward && object == Profile.getValue()
        && (name == "H_Axis" || name == "V_Axis" || name == "N_Axis" || name.rfind("Axis", 0) == 0)) {
        Base::Vector3d origin, direction;
        getAxis(object, names, origin, direction, ForbiddenAxis::NoCheck);
        return direction;
    }

    // getTopoShape also resolves datum points/lines/planes, origin geometry,
    // links and nested subobjects. Keep reference and profile in the same frame.
    const auto target = Part::Feature::getTopoShape(
        object,
        Part::ShapeOption::NeedSubElement | Part::ShapeOption::ResolveLink
            | Part::ShapeOption::Transform,
        name.c_str()
    );
    return Base::convertTo<Base::Vector3d>(
        toward ? thinDirectionTowardReference(getThinInput(), target) : thinReferenceAxis(target)
    );
}

Base::Vector3d ThinExtrude::towardReferenceDirection() const
{
    return referenceDirection(ReferenceAxis.getValue(), ReferenceAxis.getSubValues(), true);
}

Base::Vector3d ThinExtrude::computeDirection(const Base::Vector3d& sketchVector, bool inverse)
{
    if (UseCustomVector.getValue() || (!TowardReference.getValue() && !ReferenceAxis.getValue())) {
        return Pad::computeDirection(sketchVector, inverse);
    }

    const auto direction = referenceDirection(
        ReferenceAxis.getValue(),
        ReferenceAxis.getSubValues(),
        TowardReference.getValue()
    );
    Direction.setValue(direction);
    Direction.setReadOnly(true);
    ReferenceAxis.setReadOnly(false);
    return direction;
}

TopoShape ThinExtrude::prepareThinInput(const TopoShape& profile) const
{
    if (Extension.getValue() == 0 || ThinExtension.getValue() != 0) {
        return profile;
    }

    auto body = getBaseTopoShape(true);
    if (!body.isNull()) {
        body = body.makeElementCopy("RibExtensionBody");
    }

    return extendThinProfile(
        profile.makeElementCopy("RibExtensionInput"),
        body,
        Extension.getValue() == 2,
        getID()
    );
}

std::pair<TopoShape, TopoShape> ThinExtrude::getConstructionPreview() const
{
    TopoShape extension;
    auto base = getBaseTopoShape(true);
    if (!base.isNull()) {
        base = base.makeElementCopy("RibPreviewBody");
    }
    if (Extension.getValue() != 0) {
        auto profile = getThinInput().makeElementCopy("RibPreviewProfile");
        extension = extendThinProfile(profile, base, Extension.getValue() == 2, getID())
                        .makeElementCut(profile, "RibPreviewExtension");
    }
    std::vector<TopoShape> targets;
    auto collect = [&](const App::PropertyEnumeration& type,
                       const App::PropertyLinkSub& face,
                       const App::PropertyLinkSubList& shapes) {
        const std::string method = type.getValueAsString();
        TopoShape target;
        if (method == "UpToFace") {
            getUpToFaceFromLinkSub(target, face);
        }
        else if (method == "UpToShape") {
            getUpToShapeFromLinkSubList(target, shapes);
            if (target.isNull()) {
                target = base;
            }
        }
        else if (method == "UpToFirst" || method == "UpToLast") {
            target = base;
        }
        if (!target.isNull()) {
            targets.push_back(target);
        }
    };
    collect(Type, UpToFace, UpToShape);
    if (SideType.getValue() == 1) {
        collect(Type2, UpToFace2, UpToShape2);
    }
    TopoShape target;
    if (!targets.empty()) {
        target.makeElementCompound(targets, "RibPreviewTargets");
    }
    const auto inverse = getLocation().Inverted();
    extension.move(inverse);
    target.move(inverse);
    return {extension, target};
}

}  // namespace PartDesign
