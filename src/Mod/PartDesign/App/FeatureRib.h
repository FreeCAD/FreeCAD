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
    App::PropertyVector Direction;  // Rib-local fill direction; Reversed is inherited.
    App::PropertyAngle DraftAngle;
    App::PropertyEnumeration DraftReference;
    App::PropertyBool UseCustomPullDirection;
    App::PropertyVector PullDirection;  // Rib-local axis; otherwise opposite the fill.
    App::PropertyFloat FilletRadius;

    short mustExecute() const override;

    /// World-space queries shared with the task panel, including on failed previews.
    Part::TopoShape getRibProfileWire() const;
    gp_Pln getRibProfilePlane() const;

protected:
    App::DocumentObjectExecReturn* execute() override;

private:
    /// Validate and scale the user-selected Rib-local fill direction.
    gp_Vec getRibTravel(const gp_Pln& plane, double reach) const;

    /// Extend both free ends to full reach, preserving interior curves and element names.
    /// Each extension must contact the body; contact validation never trims the curve.
    Part::TopoShape extendRibProfile(
        const Part::TopoShape& body,
        const Part::TopoShape& profile,
        double reach,
        long continuity
    ) const;

    /// Planar fill region between the profile and its translated copy.
    Part::TopoShape makeRibSurface(
        const Part::TopoShape& profile,
        const gp_Vec& travel,
        const gp_Pln& plane
    ) const;

    /// Form a prism, with total width and Side A/B/Centered placement.
    Part::TopoShape makeRibTool(
        const Part::TopoShape& surface,
        const gp_Dir& normal,
        double thickness,
        long placement
    ) const;

    /// Roof ribbon used to distinguish wanted material from remote cut pieces.
    Part::TopoShape makeProfileReference(
        const Part::TopoShape& profile,
        const gp_Dir& normal,
        double width
    ) const;

    Part::TopoShape cutRibTool(const Part::TopoShape& tool, const Part::TopoShape& base) const;
    Part::TopoShape selectRibMaterial(
        const Part::TopoShape& cut,
        const Part::TopoShape& roof,
        const gp_Vec& travel,
        bool requireBodyTermination
    ) const;

    /// Transform into the common draft frame: X=reach, Y=thickness, Z=pull.
    gp_Trsf getDraftFrame(const gp_Pln& plane, const gp_Vec& travel) const;
    double getExtensionWidth(
        const Part::TopoShape& retained,
        const gp_Pln& plane,
        const gp_Vec& travel
    ) const;

    /// Draft ONLY a simple box's two broad sides, then intersect the profile prism.
    Part::TopoShape makeDraftedRibTool(
        const Part::TopoShape& surface,
        const Part::TopoShape& retained,
        const Part::TopoShape& body,
        const gp_Pln& plane,
        const gp_Vec& travel
    ) const;
    void checkDraftTermination(
        const Part::TopoShape& tool,
        const Part::TopoShape& retained,
        const gp_Pln& plane,
        const gp_Vec& travel
    ) const;

    Part::TopoShape fuseRibWithBase(const Part::TopoShape& base, const Part::TopoShape& retained) const;
    Part::TopoShape filletIntersectingEdges(
        const Part::TopoShape& fused,
        const Part::TopoShape& body
    ) const;
    void publishRib(const Part::TopoShape& result, const Part::TopoShape& body);
};

}  // namespace PartDesign
