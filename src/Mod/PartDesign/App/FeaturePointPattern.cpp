// SPDX-License-Identifier: LGPL-2.1-or-later

#include "FeaturePointPattern.h"

#include <Mod/Part/App/Tools.h>

using namespace PartDesign;

PROPERTY_SOURCE_WITH_EXTENSIONS(PartDesign::PointPattern, PartDesign::Transformed)

PointPattern::PointPattern()
{
    Part::PointPatternExtension::initExtension(this);
}

const std::list<gp_Trsf> PointPattern::getTransformations(const std::vector<App::DocumentObject*>)
{
    auto transformations = calculateTransformations(true);
    const gp_Trsf inverse = getLocation().Transformation().Inverted();
    for (auto& transformation : transformations) {
        // Point coordinates are in the body's frame; copies are built in the feature's frame.
        gp_Vec offset(transformation.TranslationPart());
        offset.Transform(inverse);
        transformation.SetTranslation(offset);
    }
    return transformations;
}

void PointPattern::positionBySupport()
{
    Transformed::positionBySupport();

    const auto transformations = calculateTransformations(false);
    if (transformations.empty()) {
        return;
    }

    const gp_XYZ position = transformations.front().TranslationPart();
    Base::Placement placement = Placement.getValue();
    placement.setPosition(Base::convertTo<Base::Vector3d>(position));
    Placement.setValue(placement);
}
