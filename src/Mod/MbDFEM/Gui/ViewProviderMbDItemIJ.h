// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <Gui/ViewProviderDocumentObject.h>
#include <Mod/MbDFEM/MbDFEMGlobal.h>

#include <vector>

namespace App
{
class DocumentObject;
}

namespace MbDFEMGui
{

class MbDFEMGuiExport ViewProviderMbDItemIJ: public Gui::ViewProviderDocumentObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(MbDFEMGui::ViewProviderMbDItemIJ);

public:
    ViewProviderMbDItemIJ();
    ~ViewProviderMbDItemIJ() override = default;

    std::vector<App::DocumentObject*> claimChildren() const override;
};

}  // namespace MbDFEMGui
