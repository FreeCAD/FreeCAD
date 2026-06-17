// SPDX-License-Identifier: LGPL-2.1-or-later

#include "ViewProviderCircularPattern.h"

using namespace PartDesignGui;

PROPERTY_SOURCE(PartDesignGui::ViewProviderCircularPattern,
                PartDesignGui::ViewProviderPolarPattern)

void ViewProviderCircularPattern::setupContextMenu(QMenu* menu,
                                                   QObject* receiver,
                                                   const char* member)
{
    addDefaultAction(menu, QObject::tr("Edit Circular Pattern"));
    PartDesignGui::ViewProvider::setupContextMenu(menu, receiver, member);
}

const std::string& ViewProviderCircularPattern::featureName() const
{
    static const std::string name = "CircularPattern";
    return name;
}
