// SPDX-License-Identifier: LGPL-2.1-or-later
#pragma once
#include "FeatureSketchBased.h"
#include <App/PropertyUnits.h>
#include <optional>

namespace PartDesign
{
/** Isolated side-profile reinforcement feature. Reuses ProfileBased helpers
 * without changing the shared Pad/Pocket feature or its execution path. */
class PartDesignExport Rib: public ProfileBased
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartDesign::Rib);

public:
    Rib();

    App::PropertyEnumeration SideType;
    App::PropertyEnumeration Type;
    App::PropertyEnumeration Type2;
    App::PropertyLength Length;
    App::PropertyLength Length2;
    App::PropertyAngle TaperAngle;
    App::PropertyAngle TaperAngle2;
    App::PropertyBool UseCustomVector;
    App::PropertyVector Direction;
    App::PropertyLinkSub ReferenceAxis;
    App::PropertyEnumeration StartType;
    App::PropertyLength StartOffset;
    App::PropertyLinkSub StartReference;
    App::PropertyLength Offset;
    App::PropertyLength Offset2;

    App::PropertyLength ThinThickness;
    App::PropertyLength ThinThickness2;
    App::PropertyEnumeration ThinSide;
    App::PropertyEnumeration ThinJoin;
    App::PropertyEnumeration ThinCap;
    App::PropertyEnumeration ThinDraftReference;
    App::PropertyLength RootFilletRadius;

    Base::Vector3d getProfileNormal() const override;
    double getStartOffset() const;
    std::pair<double, double> getThinWidths() const;

    App::PropertyVector FillDirection;
    App::PropertyLength EdgeFilletRadius;
    App::PropertyEnumeration Extension;
    App::PropertyBool TowardReference;
    App::PropertyBool AutoDirection;
    App::PropertyLinkSub DraftPullDirection;
    App::PropertyEnumeration DraftPullMode;
    App::PropertyVector DraftPullVector;
    App::PropertyBool FlipPullDirection;

    short mustExecute() const override;
    App::DocumentObjectExecReturn* execute() override;
    std::pair<TopoShape, TopoShape> getConstructionPreview() const;

    TopoShape getInputProfile() const
    {
        return getThinInput();
    }

    /** Explicit draft pull in world coordinates, or no override for Automatic.
     * FlipPullDirection is applied separately by the draft construction. */
    std::optional<Base::Vector3d> getDraftPullVector() const;

    /** Resolve the same reference semantics for execution and selection filtering. */
    Base::Vector3d referenceDirection(
        const App::DocumentObject* object,
        const std::vector<std::string>& names,
        bool toward
    ) const;

    const char* getViewProviderName() const override
    {
        return "PartDesignGui::ViewProviderRib";
    }

protected:
    Base::Vector3d towardReferenceDirection() const;
    TopoShape getThinInput() const;
};
}  // namespace PartDesign
