// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <App/DocumentObject.h>
#include <Mod/MbDFEM/MbDFEMGlobal.h>

namespace MbDFEM
{

class MbDFEMExport MbDMotion: public App::DocumentObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(MbDFEM::MbDMotion);

public:
    MbDMotion() = default;
    ~MbDMotion() override = default;

    const char* getViewProviderName() const override
    {
        return "MbDFEMGui::ViewProviderMbDMotion";
    }
};

}  // namespace MbDFEM
