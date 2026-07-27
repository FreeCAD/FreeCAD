// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <Gui/ViewProviderDocumentObject.h>
#include <Mod/MbDFEM/MbDFEMGlobal.h>

namespace MbDFEMGui
{

class MbDFEMGuiExport ViewProviderMbDPart: public Gui::ViewProviderDocumentObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(MbDFEMGui::ViewProviderMbDPart);

public:
    ViewProviderMbDPart();
    ~ViewProviderMbDPart() override = default;

    std::vector<App::DocumentObject*> claimChildren() const override;
};

}  // namespace MbDFEMGui
