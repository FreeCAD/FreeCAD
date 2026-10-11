// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <Mod/Part/App/PathPatternExtension.h>

#include "FeatureTransformed.h"

namespace PartDesign
{

class PartDesignExport PathPattern: public PartDesign::Transformed, public Part::PathPatternExtension
{
    PROPERTY_HEADER_WITH_EXTENSIONS(PartDesign::PathPattern);

public:
    PathPattern();

    const char* getViewProviderName() const override
    {
        return "PartDesignGui::ViewProviderPathPattern";
    }

    const std::list<gp_Trsf> getTransformations(const std::vector<App::DocumentObject*>) override;
};

}  // namespace PartDesign
