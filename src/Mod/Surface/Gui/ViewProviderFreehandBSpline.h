// SPDX-License-Identifier: LGPL-2.1-or-later
#pragma once

#include <QPointer>
#include <Mod/Part/Gui/ViewProviderSpline.h>

class SoSwitch;

namespace SurfaceGui
{
class FreehandBSplineEditor;
class ViewProviderFreehandBSpline: public PartGui::ViewProviderSpline
{
    PROPERTY_HEADER_WITH_OVERRIDE(SurfaceGui::ViewProviderFreehandBSpline);

public:
    QIcon getIcon() const override;
    bool doubleClicked() override;
    bool isSelectable() const override;
    bool selectAll() override;
    bool onDelete(const std::vector<std::string>&) override;
    void setEditViewer(Gui::View3DInventorViewer*, int) override;
    void unsetEditViewer(Gui::View3DInventorViewer*) override;

protected:
    void onChanged(const App::Property*) override;
    bool setEdit(int) override;
    void unsetEdit(int) override;

private:
    QPointer<FreehandBSplineEditor> editor;
    SoSwitch* editDisplaySwitch {nullptr};
};
}  // namespace SurfaceGui
