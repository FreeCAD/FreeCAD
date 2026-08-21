// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <App/PropertyStandard.h>
#include <Gui/ViewProviderDocumentObject.h>
#include <Mod/MbDFEM/MbDFEMGlobal.h>

class QMenu;

namespace MbDFEMGui
{

class MbDFEMGuiExport ViewProviderMbDGravity: public Gui::ViewProviderDocumentObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(MbDFEMGui::ViewProviderMbDGravity);

public:
    ViewProviderMbDGravity();
    ~ViewProviderMbDGravity() override;

    App::PropertyBool ShowArrow;

    void attach(App::DocumentObject* object) override;
    void finishRestoring() override;
    void setupContextMenu(QMenu* menu, QObject* receiver, const char* member) override;
    void updateData(const App::Property* prop) override;

protected:
    void onChanged(const App::Property* prop) override;

private:
    void updateCornerGravityIndicator(bool forceHidden = false);
    bool arrowVisible() const;
};

}  // namespace MbDFEMGui
