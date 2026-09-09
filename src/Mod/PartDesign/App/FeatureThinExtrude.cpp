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

}  // namespace PartDesign
