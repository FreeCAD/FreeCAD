// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "FeatureThinExtrude.h"

namespace PartDesign
{

/** Dedicated Rib/Web command adapter. Geometry and feature execution belong
 * to ThinExtrude so Pad/Pocket's first PR owns the reusable foundation. */
class PartDesignExport Rib: public ThinExtrude
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartDesign::Rib);

public:
    Rib();
    const char* getViewProviderName() const override
    {
        return "PartDesignGui::ViewProviderRib";
    }
};

}  // namespace PartDesign
