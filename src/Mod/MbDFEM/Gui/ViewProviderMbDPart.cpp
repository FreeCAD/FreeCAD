// SPDX-License-Identifier: LGPL-2.1-or-later

#include "ViewProviderMbDPart.h"

#include <Mod/MbDFEM/App/MbDPart.h>

using namespace MbDFEMGui;

PROPERTY_SOURCE(MbDFEMGui::ViewProviderMbDPart, Gui::ViewProviderDocumentObject)

ViewProviderMbDPart::ViewProviderMbDPart()
{
    sPixmap = "Document";
}

std::vector<App::DocumentObject*> ViewProviderMbDPart::claimChildren() const
{
    auto* part = getObject<MbDFEM::MbDPart>();
    if (!part) {
        return {};
    }

    if (auto* markersFolder = part->getMarkersFolder()) {
        return {markersFolder};
    }
    return {};
}
