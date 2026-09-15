// SPDX-License-Identifier: LGPL-2.1-or-later
#pragma once

#include <App/PropertyUnits.h>
#include <gp_Pln.hxx>
#include "FeatureSketchBased.h"

namespace PartDesign
{

/// A planar open profile filled towards a body, with thickness, draft and root fillets.
class PartDesignExport Rib: public ProfileBased
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartDesign::Rib);

public:
    Rib();
    const char* getViewProviderName() const override
    {
        return "PartDesignGui::ViewProviderRib";
    }

    App::PropertyEnumeration ExtendType;
    App::PropertyLength Thickness;
    App::PropertyEnumeration PlacementType;
    App::PropertyEnumeration ExtentType;
    App::PropertyFloat Distance;
    App::PropertyVector Direction;  // sweep direction to form the rib
    App::PropertyAngle DraftAngle;
    App::PropertyEnumeration DraftReference;
    App::PropertyBool UseCustomPullDirection;
    App::PropertyVector PullDirection;  // draft
    App::PropertyFloat FilletRadius;

    short mustExecute() const override;

    Part::TopoShape getRibProfileWire() const;


protected:
    App::DocumentObjectExecReturn* execute() override;

private:
    /// Combine the in-plane direction, extent length and Reversed flag into a displacement.
    gp_Vec makeSweepVector(const gp_Pln& plane, double reach) const;

    /// Extend the free ends of the profile - the extensions must intersect the body
    Part::TopoShape extendRibProfile(
        const Part::TopoShape& body,
        const Part::TopoShape& profile,
        double reach,
        long continuity
    ) const;

    /// Extrude the profile to form a surface from which the rib-tool will be created
    Part::TopoShape makeRibSurface(
        const Part::TopoShape& profile,
        const gp_Vec& travel,
        const gp_Pln& plane
    ) const;

    /// Form a prism from the rib surface with user-supplied thickness
    /// Side A, B or Centered placement around the profile
    Part::TopoShape makeRibTool(
        const Part::TopoShape& surface,
        const gp_Dir& normal,
        double thickness,
        long placement
    ) const;


    /// Cut the rib tool with the body
    Part::TopoShape cutRibTool(
        const Part::TopoShape& tool,
        const Part::TopoShape& base
    ) const;

    /// Keep solids touching the original profile, but not the translated farLimit wire.
    /// A null farLimit permits free ends for Distance extent.
    Part::TopoShape selectRibMaterial(
        const Part::TopoShape& cut,
        const Part::TopoShape& profile,
        const Part::TopoShape& farLimit
    ) const;

    /// Transform into the common draft frame: X=reach, Y=thickness, Z=pull.
    gp_Trsf getDraftFrame(
        const gp_Pln& plane,
        const gp_Vec& travel
    ) const;

    /// Get the extension width
    double getExtensionWidth(
        const Part::TopoShape& retained,
        const gp_Pln& plane,
        const gp_Vec& travel
    ) const;

    /// Draft a simple box's two broad sides, then intersect the profile prism
    Part::TopoShape makeDraftedRibTool(
        const Part::TopoShape& surface,
        const Part::TopoShape& retained,
        const Part::TopoShape& body,
        const gp_Pln& plane,
        const gp_Vec& travel
    ) const;

    /// Check the draft termination
    void checkDraftTermination(
        const Part::TopoShape& tool,
        const Part::TopoShape& retained,
        const gp_Pln& plane,
        const gp_Vec& travel
    ) const;

    /// Fuse the rib tool with the base
    Part::TopoShape fuseRibWithBase(
        const Part::TopoShape& base,
        const Part::TopoShape& retained
    ) const;

    /// Fillet the intersecting edges of the fused rib tool
    Part::TopoShape filletIntersectingEdges(
        const Part::TopoShape& fused,
        const Part::TopoShape& body
    ) const;

    /// Publish the rib result
    void publishRib(
        const Part::TopoShape& result,
        const Part::TopoShape& body
    );
};

}  // namespace PartDesign
