// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "App/PropertyContainer.h"
#include "App/PropertyStandard.h"
#include <App/PropertyUnits.h>
#include "FeatureSketchBased.h"

class Bnd_Box;

namespace PartDesign
{

/// Starting point for a profile-based additive feature.
class PartDesignExport Rib: public ProfileBased
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartDesign::Rib);

public:
    Rib();

    const char* getViewProviderName() const override
    {
        return "PartDesignGui::ViewProviderRib";
    }

    App::PropertyEnumeration ExtendType; // off, c1, c2
    App::PropertyLength Thickness; //
    App::PropertyEnumeration PlacementType; // centered, side a, side b
    App::PropertyEnumeration ExtentType; // shape or distance
    App::PropertyFloat Distance; // distance to extend towards when distance is specified
    App::PropertyVector Direction; // Rib-local sweep direction, for both extent types
    App::PropertyAngle DraftAngle; // draft angle pos or neg ok
    App::PropertyBool UseCustomPullDirection;
    App::PropertyVector PullDirection; // Optional Rib-local draft axis; otherwise opposite the sweep
    App::PropertyFloat FilletRadius; // fillet radius



    short mustExecute() const override;
protected:
    App::DocumentObjectExecReturn* execute() override;

private:
    /// Get a single valid open profile wire (e.g., a connected wire with two free endpoints)
    Part::TopoShape getRibProfileWire() const;

    /// Extend the open ends a distance (reach) and continuity c1 or c2.
    Part::TopoShape extendRibProfile(
        const Part::TopoShape& source,
        double reach,
        int continuity
    ) const;

    /// Sweep the extended profile and create a surface
    Part::TopoShape makeRibSurface(const Part::TopoShape& profile, const gp_Vec& travel) const;

    /// Extrude the surface to thickness and apply its placement offset (e.g., to recenter).
    Part::TopoShape makeRibTool(
        const Part::TopoShape& surface,
        const double& thickness,
        const long& thicknessPlacementType,
    ) const;

    /// Draft side faces belonging to the uncut tool. Angle is in radians;
    /// faces, pull direction and neutral plane must share the tool's local frame.
    Part::TopoShape applyDraft(
        const Part::TopoShape& tool,
        const std::vector<Part::TopoShape>& faces,
        const gp_Dir& pullDirection,
        double draftAngle,
        const gp_Pln& neutralPlane
    ) const;

    /// Bound the uncut tool using the retained rib, then draft it before the final body cut.
    /// All inputs share the Rib-local frame. Root/top planes are measured before padding.
    Part::TopoShape makeDraftedRibTool(
        const Part::TopoShape& tool,
        const Part::TopoShape& retained,
        const Part::TopoShape& body,
        const gp_Pln& profilePlane,
        const gp_Vec& travel,
        double draftAngle,
        bool holdRoot = false
    ) const;

    /// Subtract the base from the tool while retaining history from both operands.
    Part::TopoShape cutRibTool(const Part::TopoShape& tool, const Part::TopoShape& base) const;

    /// Test geometric contact without changing either shape.
    static bool profileTouchesSolid(const TopoDS_Shape& solid, const TopoDS_Shape& reference);

    /// Keep source-connected solids; require termination at the body only in Shape mode.
    Part::TopoShape selectRibMaterial(
        const Part::TopoShape& cutResult,
        const Part::TopoShape& originalProfile,
        const gp_Vec& travel,
        bool requireBodyTermination
    ) const;

    /// Fuse the post-cut rib tool with the base
    Part::TopoShape fuseRibWithBase(
        const Part::TopoShape& base,
        const Part::TopoShape& retained,
        bool refine
    ) const;

    /// Fillet edges between body and rib faces in an unrefined union, in a common frame.
    Part::TopoShape filletIntersectingEdges(
        const Part::TopoShape& fused,
        const Part::TopoShape& body
    ) const;

    /// Build a solid from finite bounds; rejects empty or degenerate boxes.
    static Part::TopoShape bboxToShape(const Bnd_Box& bbox);

    /// Calculate normalized sweep direction
    Base::Vector3d calculateSweepDirection(const Part::TopoShape& profileShape, const Part::TopoShape& baseShape) const;

    /// Calculate the direction vector from the profile center to the body
    gp_Vec calculateRibDirection(const gp_Vec& profileCenter, const gp_Vec& bodyCenter) const;

};

}  // namespace PartDesign
