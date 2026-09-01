// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "ViewProviderPolarPattern.h"

namespace PartDesignGui
{

class PartDesignGuiExport ViewProviderCircularPattern: public ViewProviderPolarPattern
{
    Q_DECLARE_TR_FUNCTIONS(PartDesignGui::ViewProviderCircularPattern)
    PROPERTY_HEADER_WITH_OVERRIDE(PartDesignGui::ViewProviderCircularPattern);

public:
    ViewProviderCircularPattern()
    {
        menuName = tr("Circular Pattern Parameters");
        sPixmap = "PartDesign_CircularPattern.svg";
    }

    const std::string& featureName() const override;
    void setupContextMenu(QMenu*, QObject*, const char*) override;
};

}  // namespace PartDesignGui
