// SPDX-License-Identifier: LGPL-2.1-or-later

#include "ViewProviderRib.h"
#include "TaskRibParameters.h"

#include <Mod/PartDesign/App/FeatureRib.h>

namespace PartDesignGui
{

PROPERTY_SOURCE(PartDesignGui::ViewProviderRib, PartDesignGui::ViewProvider)

ViewProviderRib::ViewProviderRib()
{
    sPixmap = "PartDesign_Rib.svg";
}

Part::TopoShape ViewProviderRib::getPreviewShape()
{
    if (!getObject<PartDesign::Rib>()->isValid()) {
        return {};  // Do not show a previous result after construction fails.
    }
    return ViewProvider::getPreviewShape();
}

std::vector<App::DocumentObject*> ViewProviderRib::claimChildren() const
{
    auto profile = getObject<PartDesign::Rib>()->Profile.getValue();
    if (profile && !profile->isDerivedFrom<PartDesign::Feature>()) {
        return {profile};
    }
    return {};
}

void ViewProviderRib::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    addDefaultAction(menu, QObject::tr("Edit Rib"));
    ViewProvider::setupContextMenu(menu, receiver, member);
}

TaskDlgFeatureParameters* ViewProviderRib::getEditDialog()
{
    return new TaskDlgRibParameters(this);
}

}  // namespace PartDesignGui
