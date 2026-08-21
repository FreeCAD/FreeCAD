// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <Gui/ViewProviderDocumentObject.h>
#include <Mod/MbDFEM/MbDFEMGlobal.h>

class QMenu;

namespace MbDFEMGui
{

class MbDFEMGuiExport ViewProviderMbDSimulationParameters: public Gui::ViewProviderDocumentObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(MbDFEMGui::ViewProviderMbDSimulationParameters);

public:
    ViewProviderMbDSimulationParameters();
    ~ViewProviderMbDSimulationParameters() override = default;

    bool doubleClicked() override;
    void setupContextMenu(QMenu* menu, QObject* receiver, const char* member) override;
};

}  // namespace MbDFEMGui
