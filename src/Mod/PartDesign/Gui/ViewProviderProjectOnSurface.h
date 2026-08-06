// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <Mod/Part/Gui/ViewProviderProjectOnSurface.h>
#include <Mod/PartDesign/PartDesignGlobal.h>

namespace PartDesignGui
{

class PartDesignGuiExport ViewProviderProjectOnSurface:
    public PartGui::ViewProviderProjectOnSurface
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartDesignGui::ViewProviderProjectOnSurface);

public:
    ViewProviderProjectOnSurface();

    bool setEdit(int mode) override;
    void unsetEdit(int mode) override;
};

}  // namespace PartDesignGui
