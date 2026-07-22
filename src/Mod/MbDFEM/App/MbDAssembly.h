// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <App/DocumentObjectGroup.h>
#include <App/PropertyGeo.h>
#include <App/PropertyLinks.h>
#include <Mod/MbDFEM/MbDFEMGlobal.h>

namespace MbDFEM
{

class MbDMarker;
class MbDPart;

class MbDFEMExport MbDAssembly: public App::DocumentObjectGroup
{
    PROPERTY_HEADER_WITH_OVERRIDE(MbDFEM::MbDAssembly);

public:
    MbDAssembly();
    ~MbDAssembly() override = default;

    App::PropertyPlacement Placement;
    App::PropertyLinkList parts;
    App::PropertyLinkList markers;

    void addPart(MbDPart* part);
    void addMarker(MbDMarker* marker);

    PyObject* getPyObject() override;

private:
    App::PropertyLink _partsGroup;
    App::PropertyLink _markersGroup;

    App::DocumentObjectGroup* ensurePartsGroup();
    App::DocumentObjectGroup* ensureMarkersGroup();
};

}  // namespace MbDFEM
