// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <App/DocumentObject.h>
#include <App/PropertyLinks.h>
#include <Mod/MbDFEM/MbDFEMGlobal.h>

namespace MbDFEM
{

class MbDMarker;

class MbDFEMExport MbDItemIJ: public App::DocumentObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(MbDFEM::MbDItemIJ);

public:
    MbDItemIJ();
    ~MbDItemIJ() override = default;

    App::PropertyLink markerI;
    App::PropertyLink markerJ;

    void setMarkerI(MbDMarker* marker);
    void setMarkerJ(MbDMarker* marker);
    void setMarkers(MbDMarker* markerI, MbDMarker* markerJ);

    PyObject* getPyObject() override;
};

}  // namespace MbDFEM
