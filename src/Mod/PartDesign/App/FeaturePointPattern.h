// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <Mod/Part/App/PointPatternExtension.h>

#include "FeatureTransformed.h"

namespace PartDesign
{

class PartDesignExport PointPattern: public PartDesign::Transformed, public Part::PointPatternExtension
{
    PROPERTY_HEADER_WITH_EXTENSIONS(PartDesign::PointPattern);

public:
    PointPattern();

    const char* getViewProviderName() const override
    {
        return "PartDesignGui::ViewProviderPointPattern";
    }

    const std::list<gp_Trsf> getTransformations(const std::vector<App::DocumentObject*>) override;

protected:
    void positionBySupport() override;
};

}  // namespace PartDesign
