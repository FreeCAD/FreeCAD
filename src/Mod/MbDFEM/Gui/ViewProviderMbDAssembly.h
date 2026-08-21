// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <Gui/ViewProviderPart.h>
#include <Mod/MbDFEM/MbDFEMGlobal.h>

class SoDetail;
class SoFullPath;
class SoPickedPoint;

namespace MbDFEMGui
{

class MbDFEMGuiExport ViewProviderMbDAssembly: public Gui::ViewProviderPart
{
    PROPERTY_HEADER_WITH_OVERRIDE(MbDFEMGui::ViewProviderMbDAssembly);

public:
    ViewProviderMbDAssembly();
    ~ViewProviderMbDAssembly() override = default;

    void attach(App::DocumentObject* object) override;
    void finishRestoring() override;
    std::vector<App::DocumentObject*> claimChildren() const override;
    std::vector<App::DocumentObject*> claimChildren3D() const override;
    bool canDropObjects() const override;
    bool canDropObject(App::DocumentObject* obj) const override;
    void dropObject(App::DocumentObject* obj) override;
    bool getDetailPath(const char* subname,
                       SoFullPath* path,
                       bool append,
                       SoDetail*& det) const override;
    bool getElementPicked(const SoPickedPoint* pp, std::string& subname) const override;

    void setupContextMenu(QMenu* menu, QObject* receiver, const char* member) override;

};

}  // namespace MbDFEMGui
