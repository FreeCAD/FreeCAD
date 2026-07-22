// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <App/GeoFeature.h>
#include <Mod/MbDFEM/MbDFEMGlobal.h>

namespace MbDFEM
{

class MbDFEMExport MbDMarker: public App::GeoFeature
{
    PROPERTY_HEADER_WITH_OVERRIDE(MbDFEM::MbDMarker);

public:
    MbDMarker() = default;
    ~MbDMarker() override = default;

    const char* getViewProviderName() const override
    {
        return "Gui::ViewProviderDocumentObject";
    }
};

}  // namespace MbDFEM
