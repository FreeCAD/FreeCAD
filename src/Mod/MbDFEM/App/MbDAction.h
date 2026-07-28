// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "MbDItemIJ.h"

namespace MbDFEM
{

class MbDFEMExport MbDAction: public MbDItemIJ
{
    PROPERTY_HEADER_WITH_OVERRIDE(MbDFEM::MbDAction);

public:
    MbDAction() = default;
    ~MbDAction() override = default;

    const char* getViewProviderName() const override
    {
        return "MbDFEMGui::ViewProviderMbDAction";
    }
};

}  // namespace MbDFEM
