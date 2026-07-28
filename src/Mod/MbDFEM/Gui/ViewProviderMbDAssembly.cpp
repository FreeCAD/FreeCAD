// SPDX-License-Identifier: LGPL-2.1-or-later

#include "ViewProviderMbDAssembly.h"

#include <Mod/MbDFEM/App/MbDAssembly.h>

using namespace MbDFEMGui;

PROPERTY_SOURCE(MbDFEMGui::ViewProviderMbDAssembly, Gui::ViewProviderDocumentObject)

ViewProviderMbDAssembly::ViewProviderMbDAssembly()
{
    sPixmap = "Document";
}

std::vector<App::DocumentObject*> ViewProviderMbDAssembly::claimChildren() const
{
    auto* assembly = getObject<MbDFEM::MbDAssembly>();
    if (!assembly) {
        return {};
    }

    std::vector<App::DocumentObject*> children;
    if (auto* markersFolder = assembly->getMarkersFolder()) {
        children.push_back(markersFolder);
    }
    if (auto* assembliesFolder = assembly->getAssembliesFolder()) {
        children.push_back(assembliesFolder);
    }
    if (auto* partsFolder = assembly->getPartsFolder()) {
        children.push_back(partsFolder);
    }
    if (auto* jointsFolder = assembly->getJointsFolder()) {
        children.push_back(jointsFolder);
    }
    if (auto* motionsFolder = assembly->getMotionsFolder()) {
        children.push_back(motionsFolder);
    }
    if (auto* actionsFolder = assembly->getActionsFolder()) {
        children.push_back(actionsFolder);
    }
    return children;
}
