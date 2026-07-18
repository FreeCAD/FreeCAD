// SPDX-License-Identifier: LGPL-2.1-or-later

#include "FeaturePointPattern.h"

using namespace PartDesign;

PROPERTY_SOURCE_WITH_EXTENSIONS(PartDesign::PointPattern, PartDesign::Transformed)

PointPattern::PointPattern()
{
    Part::PointPatternExtension::initExtension(this);
}

const std::list<gp_Trsf> PointPattern::getTransformations(const std::vector<App::DocumentObject*>)
{
    return calculateTransformations(true);
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
    placement.setPosition(Base::Vector3d(position.X(), position.Y(), position.Z()));
    Placement.setValue(placement);
}
