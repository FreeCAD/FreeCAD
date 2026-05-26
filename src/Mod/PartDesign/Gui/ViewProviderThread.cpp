// SPDX-License-Identifier: LGPL-2.1-or-later

#include "TaskThreadParameters.h"
#include "ViewProviderThread.h"

using namespace PartDesignGui;

PROPERTY_SOURCE(PartDesignGui::ViewProviderThread, PartDesignGui::ViewProviderDressUp)

const std::string& ViewProviderThread::featureName() const
{
    static const std::string name = "Thread";
    return name;
}

void ViewProviderThread::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    addDefaultAction(menu, QObject::tr("Edit Thread"));
    PartDesignGui::ViewProvider::setupContextMenu(menu, receiver, member);
}

TaskDlgFeatureParameters* ViewProviderThread::getEditDialog()
{
    return new TaskDlgThreadParameters(this);
}
