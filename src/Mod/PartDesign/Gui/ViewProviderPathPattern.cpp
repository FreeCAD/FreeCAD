// SPDX-License-Identifier: LGPL-2.1-or-later

#include "ViewProviderPathPattern.h"

using namespace PartDesignGui;

PROPERTY_SOURCE(PartDesignGui::ViewProviderPathPattern,
                PartDesignGui::ViewProviderLinearPattern)

void ViewProviderPathPattern::setupContextMenu(QMenu* menu,
                                               QObject* receiver,
                                               const char* member)
{
    addDefaultAction(menu, QObject::tr("Edit Path Pattern"));
    PartDesignGui::ViewProvider::setupContextMenu(menu, receiver, member);
}

const std::string& ViewProviderPathPattern::featureName() const
{
    static const std::string name = "PathPattern";
    return name;
}
