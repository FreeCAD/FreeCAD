// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "ViewProviderLinearPattern.h"

namespace PartDesignGui
{

class PartDesignGuiExport ViewProviderPathPattern: public ViewProviderLinearPattern
{
    Q_DECLARE_TR_FUNCTIONS(PartDesignGui::ViewProviderPathPattern)
    PROPERTY_HEADER_WITH_OVERRIDE(PartDesignGui::ViewProviderPathPattern);

public:
    ViewProviderPathPattern()
    {
        menuName = tr("Path Pattern Parameters");
        sPixmap = "PartDesign_PathPattern.svg";
    }

    const std::string& featureName() const override;
    void setupContextMenu(QMenu*, QObject*, const char*) override;
};

}  // namespace PartDesignGui
