// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "MbDItemIJ.h"

namespace MbDFEM
{

class MbDFEMExport MbDJoint: public MbDItemIJ
{
    PROPERTY_HEADER_WITH_OVERRIDE(MbDFEM::MbDJoint);

public:
    MbDJoint() = default;
    ~MbDJoint() override = default;

    const char* getViewProviderName() const override
    {
        return "MbDFEMGui::ViewProviderMbDJoint";
    }
};

}  // namespace MbDFEM
