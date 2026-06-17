// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <Mod/Part/App/CircularPatternExtension.h>

#include "FeatureTransformed.h"

namespace PartDesign
{

class PartDesignExport CircularPattern: public PartDesign::Transformed,
                                        public Part::CircularPatternExtension
{
    PROPERTY_HEADER_WITH_EXTENSIONS(PartDesign::CircularPattern);

public:
    CircularPattern();

    const char* getViewProviderName() const override
    {
        return "PartDesignGui::ViewProviderCircularPattern";
    }

    const std::list<gp_Trsf> getTransformations(
        const std::vector<App::DocumentObject*>
    ) override;
    gp_Ax2 getRotation() const override;
};

}  // namespace PartDesign
