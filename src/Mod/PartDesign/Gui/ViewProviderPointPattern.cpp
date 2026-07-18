// SPDX-License-Identifier: LGPL-2.1-or-later

#include "ViewProviderPointPattern.h"

using namespace PartDesignGui;

PROPERTY_SOURCE(PartDesignGui::ViewProviderPointPattern, PartDesignGui::ViewProviderLinearPattern)

void ViewProviderPointPattern::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    addDefaultAction(menu, QObject::tr("Edit Point Pattern"));
    PartDesignGui::ViewProvider::setupContextMenu(menu, receiver, member);
}

const std::string& ViewProviderPointPattern::featureName() const
{
    static const std::string name = "PointPattern";
    return name;
}
