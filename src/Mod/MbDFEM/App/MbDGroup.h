// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <App/DocumentObjectGroup.h>
#include <Mod/MbDFEM/MbDFEMGlobal.h>

namespace MbDFEM
{

class MbDFEMExport MbDGroup: public App::DocumentObjectGroup
{
    PROPERTY_HEADER_WITH_OVERRIDE(MbDFEM::MbDGroup);

public:
    MbDGroup() = default;
    ~MbDGroup() override = default;

    bool allowDuplicateLabel() const override
    {
        return true;
    }
};

}  // namespace MbDFEM
