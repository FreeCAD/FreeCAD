// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <App/PropertyStandard.h>
#include <Mod/Material/App/PropertyMaterial.h>

#include "MbDMarker.h"

namespace MbDFEM
{

class MbDFEMExport MbDMassMarker: public MbDMarker
{
    PROPERTY_HEADER_WITH_OVERRIDE(MbDFEM::MbDMassMarker);

public:
    MbDMassMarker();
    ~MbDMassMarker() override = default;

    App::PropertyFloat mass;
    App::PropertyVector principalInertias;
    App::PropertyBool massMarkerFromShape;
    Materials::PropertyMaterial material;

    double densityInKgPerMm3() const;
    double densityInKgPerM3() const;

    void onChanged(const App::Property* prop) override;
};

}  // namespace MbDFEM
