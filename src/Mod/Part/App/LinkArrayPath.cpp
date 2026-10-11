// SPDX-License-Identifier: LGPL-2.1-or-later

#include "LinkArrayPath.h"

using namespace Part;

PROPERTY_SOURCE_WITH_EXTENSIONS(Part::LinkArrayPath, Part::LinkArray)

LinkArrayPath::LinkArrayPath()
{
    Part::PathPatternExtension::initExtension(this);
    setPathLinkScope();
}

void LinkArrayPath::onDocumentRestored()
{
    inherited::onDocumentRestored();
    setPathLinkScope();
}

std::vector<Base::Placement> LinkArrayPath::getElementPlacements()
{
    return placementsFromTransforms(calculateTransformations(false));
}

void LinkArrayPath::setPathLinkScope()
{
    Path.setScope(App::LinkScope::Global);
}
