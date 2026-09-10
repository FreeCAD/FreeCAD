// SPDX-License-Identifier: LGPL-2.1-or-later
#include "FeatureRib.h"
#include "ThinExtrusionGeometry.h"
#include "ThinSurfaceExtrusion.h"
#include "ThinExtrusion.h"
#include "ThinProfile.h"
#include <Base/Tools.h>
#include <limits>
#include <cmath>

#include <App/Document.h>
#include <Base/Converter.h>
#include <Base/Exception.h>
#include <Precision.hxx>
#include <Mod/Part/App/Tools.h>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRep_Tool.hxx>
#include <TopoDS.hxx>
#include <gp_Ax3.hxx>
#include <gp_Pln.hxx>

namespace PartDesign
{
PROPERTY_SOURCE(PartDesign::Rib, PartDesign::ProfileBased)

Rib::Rib()
{
    static const char* sideTypes[] = {"One side", "Two sides", "Symmetric", nullptr};
    static const char* extentTypes[]
        = {"Length", "UpToLast", "UpToFirst", "UpToFace", "UpToShape", nullptr};
    static App::PropertyQuantityConstraint::Constraints signedLengthConstraint
        = {-std::numeric_limits<double>::max(), std::numeric_limits<double>::max(), 1.0};
    static App::PropertyAngle::Constraints floatAngle = {-89.999, 89.999, 1.0};
    defineAdditive();

    ADD_PROPERTY_TYPE(SideType, (0L), "Rib", App::Prop_None, "Type of sides definition");
    ADD_PROPERTY_TYPE(Type, (0L), "Side1", App::Prop_None, "Rib type for side 1");
    ADD_PROPERTY_TYPE(Type2, (0L), "Side2", App::Prop_None, "Rib type for side 2");
    SideType.setEnums(sideTypes);
    Type.setEnums(extentTypes);
    Type2.setEnums(extentTypes);
    Type.setValue("UpToShape");
    ADD_PROPERTY_TYPE(Length, (10.0), "Side1", App::Prop_None, "Rib length");
    ADD_PROPERTY_TYPE(Length2, (10.0), "Side2", App::Prop_None, "Rib length in 2nd direction");
    ADD_PROPERTY_TYPE(UseCustomVector, (false), "Rib", App::Prop_None, "Use custom vector for rib direction");
    ADD_PROPERTY_TYPE(
        Direction,
        (Base::Vector3d(1.0, 1.0, 1.0)),
        "Rib",
        App::Prop_None,
        "Rib direction vector"
    );
    ADD_PROPERTY_TYPE(ReferenceAxis, (nullptr), "Rib", App::Prop_None, "Reference axis of direction");
    ADD_PROPERTY_TYPE(StartType, (0L), "Start", App::Prop_None, "How to define the start plane");
    StartType.setEnums(StartTypesEnums);
    ADD_PROPERTY_TYPE(StartOffset, (0.0), "Start", App::Prop_None, "Offset from the start plane");
    ADD_PROPERTY_TYPE(
        StartReference,
        (nullptr),
        "Start",
        App::Prop_None,
        "Face, plane or sketch used as the start reference"
    );
    ADD_PROPERTY_TYPE(UpToFace, (nullptr), "Side1", App::Prop_None, "Face where rib will end");
    ADD_PROPERTY_TYPE(UpToShape, (nullptr), "Side1", App::Prop_None, "Faces or shape(s) where rib will end");
    ADD_PROPERTY_TYPE(UpToFace2, (nullptr), "Side2", App::Prop_None, "Face where rib will end on side2");
    ADD_PROPERTY_TYPE(
        UpToShape2,
        (nullptr),
        "Side2",
        App::Prop_None,
        "Faces or shape(s) where rib will end on side2"
    );
    ADD_PROPERTY_TYPE(Offset, (0.0), "Side1", App::Prop_None, "Offset from face in which rib will end");
    ADD_PROPERTY_TYPE(
        Offset2,
        (0.0),
        "Side2",
        App::Prop_None,
        "Offset from face in which rib will end on side 2"
    );
    Offset.setConstraints(&signedLengthConstraint);
    Offset2.setConstraints(&signedLengthConstraint);
    StartOffset.setConstraints(&signedLengthConstraint);
    ADD_PROPERTY_TYPE(TaperAngle, (0.0), "Side1", App::Prop_None, "Taper angle");
    TaperAngle.setConstraints(&floatAngle);
    ADD_PROPERTY_TYPE(TaperAngle2, (0.0), "Side2", App::Prop_None, "Taper angle for 2nd direction");
    TaperAngle2.setConstraints(&floatAngle);

    static const char* sides[] = {"SideA", "SideB", "Centered", "Two sides", nullptr};
    static const char* joins[] = {"Sharp", "Round", nullptr};
    static const char* caps[] = {"Flat", "Round", nullptr};

    ADD_PROPERTY_TYPE(
        ThinThickness,
        (1.0),
        "Thin",
        App::Prop_None,
        "Total thickness, or side A distance in two-side mode"
    );
    ADD_PROPERTY_TYPE(ThinThickness2, (1.0), "Thin", App::Prop_None, "Side B distance in two-side mode");

    ADD_PROPERTY_TYPE(
        ThinSide,
        (2L),
        "Thin",
        App::Prop_None,
        "Thickness placement, independent of extrusion reversal"
    );
    ADD_PROPERTY_TYPE(ThinJoin, (0L), "Thin", App::Prop_None, "Join at profile corners");
    ThinSide.setEnums(sides);
    ThinSide.setValue(2L);
    ThinJoin.setEnums(joins);
    ADD_PROPERTY_TYPE(ThinCap, (0L), "Thin", App::Prop_None, "Shape of free endpoints");
    ThinCap.setEnums(caps);

    static const char* draftReferences[] = {"Root", "Top", nullptr};
    ADD_PROPERTY_TYPE(
        ThinDraftReference,
        (0L),
        "Thin",
        App::Prop_None,
        "Hold nominal thickness on one neutral plane normal to growth at the root or top extent; "
        "thickness along a curved attachment may vary"
    );
    ThinDraftReference.setEnums(draftReferences);

    ADD_PROPERTY_TYPE(
        RootFilletRadius,
        (0.0),
        "Thin",
        App::Prop_None,
        "Fillet new thin-wall junctions with the existing body; zero disables"
    );


    static const char* extensions[] = {"Off", "Tangent", "Natural", nullptr};


    ADD_PROPERTY_TYPE(
        Extension,
        (0L),
        "Rib",
        App::Prop_None,
        "Extend dangling profile endpoints to the body"
    );
    Extension.setEnums(extensions);
    Extension.setValue(1L);

    ADD_PROPERTY_TYPE(
        AutoDirection,
        (true),
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
        "Rib growth direction in profile-local coordinates"
    );

    ADD_PROPERTY_TYPE(
        EdgeFilletRadius,
        (0.0),
        "Rib",
        App::Prop_None,
        "Exposed-edge fillet radius; zero disables"
    );

    ThinSide.setValue("Centered");
}

std::pair<double, double> Rib::getThinWidths() const
{
    const double thickness = ThinThickness.getValue();
    const long side = ThinSide.getValue();
    const double a = side == 1 ? 0 : side == 2 ? thickness / 2 : thickness;
    const double b = side == 0 ? 0
        : side == 2            ? thickness / 2
        : side == 3            ? ThinThickness2.getValue()
                               : thickness;
    if (!std::isfinite(a) || !std::isfinite(b) || a < 0 || b < 0
        || a + b <= 2 * Precision::Confusion()) {
        throw Base::ValueError("Thin-wall thicknesses must be nonnegative with a positive total");
    }

    return {a, b};
}

TopoShape Rib::getThinInput() const
{
    const auto object = getVerifiedObject();
    auto subs = Profile.getSubValues(false);
    if (subs.empty()) {
        subs.emplace_back("");
    }
    if (subs.size() > 1) {
        const auto full = Part::Feature::getTopoShape(
            object,
            Part::ShapeOption::ResolveLink | Part::ShapeOption::Transform
        );
        // Independent sketch subshape fetches need not share TShape identity.
        // Resolve names before fetching, and anchor A/B to source topology.
        std::stable_sort(subs.begin(), subs.end(), [&full](const auto& a, const auto& b) {
            const auto ai = full.getElementName(a.c_str()).index;
            const auto bi = full.getElementName(b.c_str()).index;
            return ai == bi ? a < b : ai < bi;
        });
    }
    std::vector<TopoShape> shapes;
    for (const auto& sub : subs) {
        if (sub.empty() && subs.size() > 1) {
            continue;
        }
        auto shape = Part::Feature::getTopoShape(
            object,
            Part::ShapeOption::NeedSubElement | Part::ShapeOption::ResolveLink
                | Part::ShapeOption::Transform,
            sub.c_str()
        );
        if (shape.isNull() || !shape.hasSubShape(TopAbs_EDGE)) {
            throw Base::ValueError("Failed to obtain thin profile edges");
        }
        shapes.push_back(shape);
    }
    return TopoShape(getID(), getDocument()->getStringHasher())
        .makeElementCompound(
            shapes,
            "ThinInput",
            TopoShape::SingleShapeCompoundCreationPolicy::returnShape
        );
}

Base::Vector3d Rib::getProfileNormal() const
{
    const auto object = Profile.getValue();
    if (object && !object->isDerivedFrom<Part::Part2DObject>()) {
        gp_Pln plane;
        if (getThinInput().findPlane(plane)) {
            const auto normal = plane.Axis().Direction();
            return Base::Vector3d(normal.X(), normal.Y(), normal.Z());
        }
    }
    return ProfileBased::getProfileNormal();
}

double Rib::getStartOffset() const
{
    const char* startType = StartType.getValueAsString();
    if (std::strcmp(startType, "Profile plane") == 0) {
        return 0.0;
    }

    gp_Dir dir = Base::convertTo<gp_Dir>(Direction.getValue());
    if (Reversed.getValue()) {
        dir.Reverse();
    }
    if (std::strcmp(startType, "Offset") == 0) {
        return StartOffset.getValue();
    }

    TopLoc_Location identity;
    return getStartReferenceOffset(getInputProfile(), StartReference, dir, StartOffset.getValue(), identity);
}

short Rib::mustExecute() const
{
    if (SideType.isTouched() || Type.isTouched() || Type2.isTouched() || Length.isTouched()
        || Length2.isTouched() || TaperAngle.isTouched() || TaperAngle2.isTouched()
        || UseCustomVector.isTouched() || Direction.isTouched() || ReferenceAxis.isTouched()
        || StartType.isTouched() || StartOffset.isTouched() || StartReference.isTouched()
        || Offset.isTouched() || Offset2.isTouched() || ThinThickness.isTouched()
        || ThinThickness2.isTouched() || ThinSide.isTouched() || ThinJoin.isTouched()
        || ThinCap.isTouched() || ThinDraftReference.isTouched() || RootFilletRadius.isTouched()
        || FillDirection.isTouched() || EdgeFilletRadius.isTouched() || Extension.isTouched()
        || TowardReference.isTouched() || AutoDirection.isTouched()
        || DraftPullDirection.isTouched() || DraftPullMode.isTouched()
        || DraftPullVector.isTouched() || FlipPullDirection.isTouched()) {
        return 1;
    }
    return ProfileBased::mustExecute();
}

std::optional<Base::Vector3d> Rib::getDraftPullVector() const
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

Base::Vector3d Rib::referenceDirection(
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

Base::Vector3d Rib::towardReferenceDirection() const
{
    return referenceDirection(ReferenceAxis.getValue(), ReferenceAxis.getSubValues(), true);
}

std::pair<TopoShape, TopoShape> Rib::getConstructionPreview() const
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

App::DocumentObjectExecReturn* Rib::execute()
{
    try {
        if (RootFilletRadius.getValue() < 0 || EdgeFilletRadius.getValue() < 0) {
            throw Base::ValueError("Rib fillet radii cannot be negative");
        }
        auto obj = getVerifiedObject();

        positionByPrevious();
        const auto inverse = getLocation().Inverted();
        auto base = getBaseTopoShape(true);
        if (!base.hasSubShape(TopAbs_SOLID)) {
            base = TopoShape();
        }
        else {
            base = base.makeElementCopy("RibBodyInput");
        }
        base.move(inverse);

        auto profile = getThinInput().makeElementCopy("RibProfileInput");
        profile.move(inverse);

        auto selectionProfile = profile;

        const bool boundEnds = Extension.getValue() != 0;
        if (Extension.getValue() != 0) {
            profile = extendThinProfile(profile, base, Extension.getValue() == 2, getID(), boundEnds);
        }

        gp_Dir dir;
        if (UseCustomVector.getValue()) {
            dir = Base::convertTo<gp_Dir>(Direction.getValue());
        }
        else if (TowardReference.getValue()) {
            dir = Base::convertTo<gp_Dir>(towardReferenceDirection());
        }
        else if (ReferenceAxis.getValue()) {
            dir = Base::convertTo<gp_Dir>(
                referenceDirection(ReferenceAxis.getValue(), ReferenceAxis.getSubValues(), false)
            );
        }
        else if (AutoDirection.getValue()) {
            auto unextended = getThinInput();
            auto target = getBaseTopoShape(true);
            auto sweep = profile;
            sweep.move(getLocation());
            dir = thinDirectionTowardBody(
                unextended,
                target,
                Base::convertTo<gp_Dir>(getProfileNormal()),
                &sweep
            );
        }
        else {
            // Inactive vectors must not invalidate an automatic/reference direction.
            dir = Base::convertTo<gp_Dir>(FillDirection.getValue());
            dir.Transform(obj->getLocation().Transformation());
        }
        Direction.setValue(Base::Vector3d(dir.X(), dir.Y(), dir.Z()));
        Direction.setReadOnly(!UseCustomVector.getValue());
        dir.Transform(inverse.Transformation());
        {  // Validate the side-profile construction plane before sweeping.
            auto normal = Base::convertTo<gp_Dir>(getProfileNormal());
            normal.Transform(inverse.Transformation());
            const gp_Pnt origin = BRep_Tool::Pnt(TopoDS::Vertex(profile.getSubShape(TopAbs_VERTEX, 1)));
            gp_Trsf toPlane;
            toPlane.SetTransformation(gp_Ax3(origin, normal));
            const auto measured
                = TopoShape(getID(), profile.Hasher).makeElementTransform(profile, toPlane);
            const auto bounds = measured.getBoundBoxOptimal();
            if (std::abs(bounds.MinZ) > 10 * Precision::Confusion()
                || std::abs(bounds.MaxZ) > 10 * Precision::Confusion()) {
                throw Base::ValueError("Side-profile rib requires a planar profile");
            }
            if (std::abs(dir.Dot(normal)) > Precision::Angular()) {
                throw Base::ValueError("Side-profile rib fill direction must lie in the profile plane");
            }
        }
        gp_Dir growth = Reversed.getValue() ? dir.Reversed() : dir;
        const double start = StartType.getValue() == 0 ? 0
            : StartType.getValue() == 1
            ? StartOffset.getValue()
            : getStartReferenceOffset(profile, StartReference, growth, StartOffset.getValue(), inverse);
        profile = moveProfileToStart(profile, growth, start, true);
        selectionProfile = moveProfileToStart(selectionProfile, growth, start, true);
        const auto side = ThinSide.getValue();
        const auto [widthA, widthB] = getThinWidths();
        auto construct = [&](TopoShape source,
                             const std::string& method,
                             double length,
                             double taper,
                             const App::PropertyLinkSub& face,
                             const App::PropertyLinkSubList& shapes,
                             double offset,
                             bool reverse) {
            TopoShape target;
            if (method == "UpToFirst") {
                target = base;
            }
            else if (method == "UpToLast") {
                auto actualDir = reverse ? dir.Reversed() : dir;
                getUpToFace(target, base, source, method, actualDir);
            }
            else if (method == "UpToFace") {
                getUpToFaceFromLinkSub(target, face);
                target.move(inverse);
            }
            else if (method == "UpToShape") {
                getUpToShapeFromLinkSubList(target, shapes);
                if (target.isNull()) {
                    target = base;
                }
                else {
                    target.move(inverse);
                }
            }
            else if (method != "Length") {
                throw Base::ValueError("Unsupported Rib termination mode");
            }
            if (method != "Length") {
                if (target.isNull()) {
                    throw Base::ValueError("Select a target body or face");
                }
                target = target.makeElementCopy("RibTargetInput");
                const auto actualDir = reverse ? dir.Reversed() : dir;
                target = moveProfileToStart(target, actualDir, offset, true);
                auto bounds = target.getBoundBox();
                bounds.Add(source.getBoundBox());
                length = 2 * bounds.CalcDiagonalLength();
                if (method == "UpToFace") {
                    BRepBuilderAPI_MakeFace extended(
                        BRep_Tool::Surface(TopoDS::Face(target.getSubShape(TopAbs_FACE, 1))),
                        Precision::Confusion()
                    );
                    target = TopoShape(getID(), profile.Hasher)
                                 .makeElementShape(extended, {target}, "RibExtendedTarget");
                }
            }

            const auto graph = makeThinProfileGraph(source, getID());
            auto wall = [&](double thickness, int placement) {
                std::vector<TopoShape> chains;
                for (const auto& chain : graph.chains) {
                    chains.push_back(makeThinSurfaceExtrusion(
                        chain.getSubTopoShapes(TopAbs_EDGE),
                        dir,
                        length,
                        thickness,
                        placement,
                        reverse,
                        ThinJoin.getValue() == 0 ? Part::JoinType::intersection : Part::JoinType::arc,
                        false,
                        getID()
                    ));
                }
                return chains.size() == 1
                    ? chains.front()
                    : TopoShape(getID(), source.Hasher).makeElementFuse(chains, "RibNetwork");
            };
            TopoShape tool;
            if (side != 3) {
                tool = wall(ThinThickness.getValue(), side);
            }
            else {
                std::vector<TopoShape> halves;
                if (ThinThickness.getValue() > Precision::Confusion()) {
                    halves.push_back(wall(ThinThickness.getValue(), 0));
                }
                if (ThinThickness2.getValue() > Precision::Confusion()) {
                    halves.push_back(wall(ThinThickness2.getValue(), 1));
                }
                if (halves.size() == 1) {
                    tool = halves.front();
                }
                else {
                    tool.makeElementFuse(halves, "RibThicknessSides");
                }
            }

            if (ThinCap.getValue() != 0) {
                tool = roundThinExtrusionEnds(tool, source, dir, length, widthA, widthB, reverse, getID());
            }

            if (!target.isNull()) {
                tool = trimThinExtrusionToBoundary(
                    tool,
                    target,
                    boundEnds ? selectionProfile : source,
                    gp_Vec(dir) * (reverse ? -length : length),
                    getID()
                );
            }

            const bool limitDepthToBody = method == "Length" && !base.isNull();
            auto limitToBody = [&](const TopoShape& wall) {
                return trimThinExtrusionToBoundary(
                    wall,
                    base,
                    boundEnds ? selectionProfile : source,
                    gp_Vec(dir) * (reverse ? -length : length),
                    getID(),
                    false
                );
            };

            if (limitDepthToBody || boundEnds) {
                // Establish the actual root before choosing the draft's
                // neutral plane, rather than the unclipped prism's far end.
                tool = limitToBody(tool);
            }

            std::optional<gp_Dir> draftPull;
            if (const auto pull = getDraftPullVector()) {
                draftPull = Base::convertTo<gp_Dir>(*pull);
                draftPull->Transform(inverse.Transformation());
            }

            tool = draftThinExtrusion(
                tool,
                source,
                base,
                reverse ? dir.Reversed() : dir,
                Base::toRadians(taper),
                ThinDraftReference.getValue() == 1,
                getID(),
                nullptr,
                draftPull ? &*draftPull : nullptr,
                FlipPullDirection.getValue()
            );

            if ((limitDepthToBody || boundEnds) && std::abs(taper) > Precision::Angular()) {
                // A rib's finite depth is a maximum, not permission to
                // emerge through the existing body. Split after drafting
                // so the final wall, including its tapered sides, is capped.
                tool = limitToBody(tool);
            }

            if (boundEnds) {
                auto normal = Base::convertTo<gp_Dir>(getProfileNormal());
                normal.Transform(inverse.Transformation());
                checkThinExtensionBoundary(tool, source, normal, dir);
            }
            return tool;
        };
        std::vector<TopoShape> sides;
        if (SideType.getValue() == 2 && std::string(Type.getValueAsString()) == "Length") {
            profile = moveProfileToStart(profile, growth, -Length.getValue() / 2, true);
            selectionProfile
                = moveProfileToStart(selectionProfile, growth, -Length.getValue() / 2, true);
            sides.push_back(construct(
                profile,
                "Length",
                Length.getValue(),
                TaperAngle.getValue(),
                UpToFace,
                UpToShape,
                Offset.getValue(),
                Reversed.getValue()
            ));
        }
        else {
            sides.push_back(construct(
                profile,
                Type.getValueAsString(),
                Length.getValue(),
                TaperAngle.getValue(),
                UpToFace,
                UpToShape,
                Offset.getValue(),
                Reversed.getValue()
            ));
            if (SideType.getValue() != 0) {
                const bool symmetric = SideType.getValue() == 2;
                sides.push_back(construct(
                    profile,
                    symmetric ? Type.getValueAsString() : Type2.getValueAsString(),
                    symmetric ? Length.getValue() : Length2.getValue(),
                    symmetric ? TaperAngle.getValue() : TaperAngle2.getValue(),
                    symmetric ? UpToFace : UpToFace2,
                    symmetric ? UpToShape : UpToShape2,
                    symmetric ? Offset.getValue() : Offset2.getValue(),
                    !Reversed.getValue()
                ));
            }
        }

        auto tool = sides.size() == 1
            ? sides.front()
            : TopoShape(getID(), profile.Hasher).makeElementFuse(sides, "RibGrowthSides");

        AddSubShape.setValue(tool);

        auto result = base.isNull()
            ? tool
            : TopoShape(getID(), profile.Hasher).makeElementFuse({base, tool}, "RibFuse");

        result = refineShapeIfActive(result);
        if (!isSingleSolidRuleSatisfied(result.getShape()) || !result.isValid()) {
            throw Base::ValueError("Rib does not form a valid connected body");
        }

        rawShape = result;
        Shape.setValue(getSolid(result));

        if (RootFilletRadius.getValue() > 0 || EdgeFilletRadius.getValue() > 0) {
            auto base = getBaseTopoShape(true);
            if (!base.isNull()) {
                base = base.makeElementCopy("RibFinishingBody");
            }
            base.move(getLocation().Inverted());
            auto result = finishThinExtrusion(
                Shape.getShape(),
                base,
                AddSubShape.getShape(),
                RootFilletRadius.getValue(),
                EdgeFilletRadius.getValue(),
                getID()
            );

            rawShape = result;
            Shape.setValue(getSolid(result));
            AddSubShape.setValue(
                base.isNull() ? result : result.makeElementCut(base, "RibFilletMaterial")
            );
        }

        return App::DocumentObject::StdReturn;
    }
    catch (const Base::Exception& error) {
        return new App::DocumentObjectExecReturn(error.what());
    }
    catch (const Standard_Failure& error) {
        return new App::DocumentObjectExecReturn(error.GetMessageString());
    }
}
}  // namespace PartDesign
