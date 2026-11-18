// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "ViewProviderLinearPattern.h"

namespace PartDesignGui
{

class PartDesignGuiExport ViewProviderPointPattern: public ViewProviderLinearPattern
{
    Q_DECLARE_TR_FUNCTIONS(PartDesignGui::ViewProviderPointPattern)
    PROPERTY_HEADER_WITH_OVERRIDE(PartDesignGui::ViewProviderPointPattern);

public:
    ViewProviderPointPattern()
    {
        menuName = tr("Point Pattern Parameters");
        sPixmap = "PartDesign_PointPattern.svg";
    }

    const std::string& featureName() const override;
    void setupContextMenu(QMenu*, QObject*, const char*) override;
};

}  // namespace PartDesignGui
