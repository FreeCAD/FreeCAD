// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "ViewProviderMbDItemIJ.h"

namespace MbDFEMGui
{

class MbDFEMGuiExport ViewProviderMbDJoint: public ViewProviderMbDItemIJ
{
    PROPERTY_HEADER_WITH_OVERRIDE(MbDFEMGui::ViewProviderMbDJoint);

public:
    ViewProviderMbDJoint();
    ~ViewProviderMbDJoint() override = default;
};

}  // namespace MbDFEMGui
