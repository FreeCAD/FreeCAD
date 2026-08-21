// SPDX-License-Identifier: LGPL-2.1-or-later

#include "ViewProviderMbDItemIJ.h"

#include <QMenu>

#include <Mod/MbDFEM/App/MbDItemIJ.h>

#include "ViewProviderUtils.h"

using namespace MbDFEMGui;

PROPERTY_SOURCE(MbDFEMGui::ViewProviderMbDItemIJ, Gui::ViewProviderDocumentObject)

ViewProviderMbDItemIJ::ViewProviderMbDItemIJ()
{
    sPixmap = "Document";
}

void ViewProviderMbDItemIJ::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    addMbDFEMContextMenuCommands(menu, {"MbDFEM_CreateMbDMarker", "MbDFEM_CreateMbDJoint"});

    if (auto* otherMenu = addOtherContextMenu(menu)) {
        Gui::ViewProviderDocumentObject::setupContextMenu(otherMenu, receiver, member);
    }
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
