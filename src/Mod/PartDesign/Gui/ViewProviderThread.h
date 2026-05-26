// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "ViewProviderDressUp.h"

namespace PartDesignGui
{

class PartDesignGuiExport ViewProviderThread: public ViewProviderDressUp
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartDesignGui::ViewProviderThread);

public:
    /// constructor
    ViewProviderThread()
    {
        sPixmap = "PartDesign_Thread.svg";
        menuName = tr("Thread Parameters");
    }

    /// return "Thread"
    const std::string& featureName() const override;
    void setupContextMenu(QMenu*, QObject*, const char*) override;

protected:
    /// Returns a newly create dialog for the part to be placed in the task view
    TaskDlgFeatureParameters* getEditDialog() override;
};

}  // namespace PartDesignGui
