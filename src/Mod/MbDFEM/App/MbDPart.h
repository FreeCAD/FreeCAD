// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <App/DocumentObjectGroup.h>
#include <App/OriginGroupExtension.h>
#include <App/PropertyLinks.h>
#include <App/PropertyStandard.h>
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
    App::PropertyFloatList xs;
    App::PropertyFloatList ys;
    App::PropertyFloatList zs;
    App::PropertyFloatList bryxs;
    App::PropertyFloatList bryys;
    App::PropertyFloatList bryzs;
    App::PropertyFloatList vxs;
    App::PropertyFloatList vys;
    App::PropertyFloatList vzs;
    App::PropertyFloatList omexs;
    App::PropertyFloatList omeys;
    App::PropertyFloatList omezs;
    App::PropertyFloatList axs;
    App::PropertyFloatList ays;
    App::PropertyFloatList azs;
    App::PropertyFloatList alpxs;
    App::PropertyFloatList alpys;
    App::PropertyFloatList alpzs;

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
