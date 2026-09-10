// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "FeatureSketchBased.h"

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

protected:
    App::DocumentObjectExecReturn* execute() override;
};

}  // namespace PartDesign
