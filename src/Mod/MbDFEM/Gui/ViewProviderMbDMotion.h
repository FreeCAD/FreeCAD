// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "ViewProviderMbDItemIJ.h"

namespace MbDFEMGui
{

class MbDFEMGuiExport ViewProviderMbDMotion: public ViewProviderMbDItemIJ
{
    PROPERTY_HEADER_WITH_OVERRIDE(MbDFEMGui::ViewProviderMbDMotion);

public:
    ViewProviderMbDMotion();
    ~ViewProviderMbDMotion() override = default;
};

}  // namespace MbDFEMGui
