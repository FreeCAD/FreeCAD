// SPDX-License-Identifier: LGPL-2.1-or-later
#pragma once
#include "ViewProvider.h"

namespace PartDesignGui
{
class PartDesignGuiExport ViewProviderRib: public ViewProvider
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartDesignGui::ViewProviderRib);

public:
    ViewProviderRib();
    ~ViewProviderRib() override;
    std::vector<App::DocumentObject*> claimChildren() const override;
    void setupContextMenu(QMenu*, QObject*, const char*) override;

protected:
    Part::TopoShape getPreviewShape() override;
    void attachPreview() override;
    void updatePreview() override;
    TaskDlgFeatureParameters* getEditDialog() override;

private:
    Gui::CoinPtr<PartGui::SoPreviewShape> profilePreview;
    Gui::CoinPtr<PartGui::SoPreviewShape> extensionPreview;
    Gui::CoinPtr<PartGui::SoPreviewShape> targetPreview;
};
}  // namespace PartDesignGui
