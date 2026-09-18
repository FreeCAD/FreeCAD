// SPDX-License-Identifier: LGPL-2.1-or-later

#include "LinkArrayPoint.h"

using namespace Part;

PROPERTY_SOURCE_WITH_EXTENSIONS(Part::LinkArrayPoint, Part::LinkArray)

LinkArrayPoint::LinkArrayPoint()
{
    Part::PointPatternExtension::initExtension(this);
    setPointObjectLinkScope();
}

void LinkArrayPoint::onDocumentRestored()
{
    inherited::onDocumentRestored();
    setPointObjectLinkScope();
}

std::vector<Base::Placement> LinkArrayPoint::getElementPlacements()
{
    return placementsFromTransforms(calculateTransformations(false));
}

void LinkArrayPoint::setPointObjectLinkScope()
{
    PointObject.setScope(App::LinkScope::Global);
}
