// SPDX-License-Identifier: LGPL-2.1-or-later

#include "FeaturePathPattern.h"

using namespace PartDesign;

PROPERTY_SOURCE_WITH_EXTENSIONS(PartDesign::PathPattern, PartDesign::Transformed)

PathPattern::PathPattern()
{
    Part::PathPatternExtension::initExtension(this);
}

short PathPattern::mustExecute() const
{
    return Transformed::mustExecute() || Path.isTouched() || Count.isTouched()
        || SpacingMode.isTouched() || Spacing.isTouched() || StartOffset.isTouched()
        || EndOffset.isTouched() || ReversePath.isTouched() || Align.isTouched()
        || VerticalVector.isTouched();
}

const std::list<gp_Trsf> PathPattern::getTransformations(const std::vector<App::DocumentObject*>)
{
    return calculateTransformations();
}

bool PathPattern::deleteOriginalTransformation() const
{
    return identityTransformSuppressed();
}

bool PathPattern::hasOriginalTransformation() const
{
    return deleteOriginalTransformation() ? false : Transformed::hasOriginalTransformation();
}
