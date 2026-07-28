// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "ViewProviderMbDItemIJ.h"

namespace MbDFEMGui
{

class MbDFEMGuiExport ViewProviderMbDAction: public ViewProviderMbDItemIJ
{
    PROPERTY_HEADER_WITH_OVERRIDE(MbDFEMGui::ViewProviderMbDAction);

public:
    ViewProviderMbDAction();
    ~ViewProviderMbDAction() override = default;
};

}  // namespace MbDFEMGui
