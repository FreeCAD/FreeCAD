// SPDX-License-Identifier: LGPL-2.1-or-later
#pragma once
#include "FeaturePad.h"
#include <optional>

namespace PartDesign
{
/** Reusable additive thin-extrusion feature engine. Owns profile interpretation,
 * direction, termination and finishing independently of dedicated task panels.
 * No Rib feature or GUI dependency is permitted in this layer. */
class PartDesignExport ThinExtrude: public Pad
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartDesign::ThinExtrude);

public:
    ThinExtrude();

    App::PropertyEnumeration RibMode;
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
        return "PartDesignGui::ViewProviderPad";
    }

protected:
    Base::Vector3d computeDirection(const Base::Vector3d& sketchVector, bool inverse) override;
    Base::Vector3d towardReferenceDirection() const;
    TopoShape prepareThinInput(const TopoShape& profile) const override;
};
}  // namespace PartDesign
