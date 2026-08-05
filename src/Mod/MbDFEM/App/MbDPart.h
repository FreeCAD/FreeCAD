// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <App/DocumentObjectGroup.h>
#include <App/OriginGroupExtension.h>
#include <App/PropertyLinks.h>
#include <Mod/MbDFEM/MbDFEMGlobal.h>
#include <Mod/Part/App/PartFeature.h>

namespace MbDFEM
{

class MbDMarker;

class MbDFEMExport MbDPart: public Part::Feature, public App::OriginGroupExtension
{
    PROPERTY_HEADER_WITH_EXTENSIONS(MbDFEM::MbDPart);

public:
    MbDPart();
    ~MbDPart() override = default;

    App::PropertyLinkList markers;

    void addMarker(MbDMarker* marker);
    void removeMarker(MbDMarker* marker);

    int setElementVisible(const char* element, bool visible) override;
    int isElementVisible(const char* element) const override;
    App::DocumentObject* getSubObject(const char* subname,
                                      PyObject** pyObj = nullptr,
                                      Base::Matrix4D* mat = nullptr,
                                      bool transform = true,
                                      int depth = 0) const override;
    void unsetupObject() override;
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
