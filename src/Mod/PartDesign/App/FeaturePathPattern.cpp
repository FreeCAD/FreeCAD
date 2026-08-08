// SPDX-License-Identifier: LGPL-2.1-or-later

#include "FeaturePathPattern.h"

using namespace PartDesign;

PROPERTY_SOURCE_WITH_EXTENSIONS(PartDesign::PathPattern, PartDesign::Transformed)

PathPattern::PathPattern()
{
    Part::PathPatternExtension::initExtension(this);
}

const std::list<gp_Trsf> PathPattern::getTransformations(const std::vector<App::DocumentObject*>)
{
    return calculateTransformations(true);
}
