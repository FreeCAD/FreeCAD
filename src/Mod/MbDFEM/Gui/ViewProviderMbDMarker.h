// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <Mod/MbDFEM/MbDFEMGlobal.h>
#include <Mod/Part/Gui/ViewProvider.h>

class SoSwitch;
class QMenu;

namespace Gui
{
class SoFCSelection;
}

namespace MbDFEMGui
{

class MbDFEMGuiExport ViewProviderMbDMarker: public PartGui::ViewProviderPart
{
    PROPERTY_HEADER_WITH_OVERRIDE(MbDFEMGui::ViewProviderMbDMarker);

public:
    ViewProviderMbDMarker();
    ~ViewProviderMbDMarker() override = default;

    void attach(App::DocumentObject* object) override;
    bool canAddToSceneGraph() const override;
    bool doubleClicked() override;
    void setupContextMenu(QMenu* menu, QObject* receiver, const char* member) override;
    void updateTriadVisibility();

protected:
    void onChanged(const App::Property* prop) override;

private:
    bool effectiveVisibility() const;

    SoSwitch* axisTriadSwitch {nullptr};
    Gui::SoFCSelection* axisTriadSelection {nullptr};
};

}  // namespace MbDFEMGui
