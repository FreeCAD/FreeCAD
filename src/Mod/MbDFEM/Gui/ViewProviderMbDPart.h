// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <Mod/Part/Gui/ViewProvider.h>
#include <Mod/MbDFEM/MbDFEMGlobal.h>

class SoGroup;
class SoDetail;
class SoFullPath;
class SoPickedPoint;

namespace MbDFEMGui
{

class MbDFEMGuiExport ViewProviderMbDPart: public PartGui::ViewProviderPart
{
    PROPERTY_HEADER_WITH_OVERRIDE(MbDFEMGui::ViewProviderMbDPart);

public:
    ViewProviderMbDPart();
    ~ViewProviderMbDPart() override;

    void attach(App::DocumentObject* object) override;
    bool canAddToSceneGraph() const override;
    SoGroup* getChildRoot() const override;
    std::vector<App::DocumentObject*> claimChildren() const override;
    std::vector<App::DocumentObject*> claimChildren3D() const override;
    bool getDetailPath(const char* subname,
                       SoFullPath* path,
                       bool append,
                       SoDetail*& det) const override;
    bool getElementPicked(const SoPickedPoint* pp, std::string& subname) const override;
    void setupContextMenu(QMenu* menu, QObject* receiver, const char* member) override;

protected:
    void onChanged(const App::Property* prop) override;

private:
    void updateMarkerVisibility();

    SoGroup* markerChildRoot {nullptr};
};

}  // namespace MbDFEMGui
