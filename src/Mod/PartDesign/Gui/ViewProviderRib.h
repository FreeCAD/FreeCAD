// SPDX-License-Identifier: LGPL-2.1-or-later
#pragma once
#include "ViewProviderPad.h"

namespace PartDesignGui
{
class PartDesignGuiExport ViewProviderRib: public ViewProviderPad
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartDesignGui::ViewProviderRib);

public:
    ViewProviderRib();
    ~ViewProviderRib() override;
    void setupContextMenu(QMenu*, QObject*, const char*) override;

protected:
    Part::TopoShape getPreviewShape() override;
    void attachPreview() override;
    void updatePreview() override;
    TaskDlgFeatureParameters* getEditDialog() override;

private:
    Gui::CoinPtr<PartGui::SoPreviewShape> extensionPreview;
    Gui::CoinPtr<PartGui::SoPreviewShape> targetPreview;
};
}  // namespace PartDesignGui
