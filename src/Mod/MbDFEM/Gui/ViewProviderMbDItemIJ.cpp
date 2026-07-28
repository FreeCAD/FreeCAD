// SPDX-License-Identifier: LGPL-2.1-or-later

#include "ViewProviderMbDItemIJ.h"

#include <Mod/MbDFEM/App/MbDItemIJ.h>

using namespace MbDFEMGui;

PROPERTY_SOURCE(MbDFEMGui::ViewProviderMbDItemIJ, Gui::ViewProviderDocumentObject)

ViewProviderMbDItemIJ::ViewProviderMbDItemIJ()
{
    sPixmap = "Document";
}

std::vector<App::DocumentObject*> ViewProviderMbDItemIJ::claimChildren() const
{
    auto* item = getObject<MbDFEM::MbDItemIJ>();
    if (!item) {
        return {};
    }

    std::vector<App::DocumentObject*> children;
    auto* markerI = item->markerI.getValue();
    auto* markerJ = item->markerJ.getValue();

    if (markerI) {
        children.push_back(markerI);
    }
    if (markerJ && markerJ != markerI) {
        children.push_back(markerJ);
    }

    return children;
}
