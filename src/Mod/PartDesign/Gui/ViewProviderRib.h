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
    std::vector<App::DocumentObject*> claimChildren() const override;
    void setupContextMenu(QMenu*, QObject*, const char*) override;

protected:
    Part::TopoShape getPreviewShape() override;
    TaskDlgFeatureParameters* getEditDialog() override;
};

}  // namespace PartDesignGui
