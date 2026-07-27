// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <App/DocumentObject.h>
#include <App/DocumentObjectGroup.h>
#include <App/PropertyGeo.h>
#include <App/PropertyLinks.h>
#include <Mod/MbDFEM/MbDFEMGlobal.h>

namespace MbDFEM
{

class MbDMarker;

class MbDFEMExport MbDPart: public App::DocumentObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(MbDFEM::MbDPart);

public:
    MbDPart();
    ~MbDPart() override = default;

    App::PropertyPlacement Placement;
    App::PropertyLinkList markers;

    void addMarker(MbDMarker* marker);

    PyObject* getPyObject() override;

    App::DocumentObjectGroup* getMarkersFolder() const;

    const char* getViewProviderName() const override
    {
        return "MbDFEMGui::ViewProviderMbDPart";
    }

private:
    App::PropertyLink _markersFolder;

    App::DocumentObjectGroup* ensureMarkersFolder();
};

}  // namespace MbDFEM
