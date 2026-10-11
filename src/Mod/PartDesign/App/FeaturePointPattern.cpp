// SPDX-License-Identifier: LGPL-2.1-or-later

#include "FeaturePointPattern.h"

#include <Mod/Part/App/Tools.h>

using namespace PartDesign;

PROPERTY_SOURCE_WITH_EXTENSIONS(PartDesign::PointPattern, PartDesign::Transformed)

PointPattern::PointPattern()
{
    Part::PointPatternExtension::initExtension(this);
}

bool PointPattern::deleteOriginalTransformation() const
{
    return identityTransformSuppressed();
}

const std::list<gp_Trsf> PointPattern::getTransformations(const std::vector<App::DocumentObject*>)
{
    return calculateTransformations();
}

bool PointPattern::hasOriginalTransformation() const
{
    return deleteOriginalTransformation() ? false : Transformed::hasOriginalTransformation();
}
