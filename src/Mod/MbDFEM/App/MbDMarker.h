// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <App/PropertyLinks.h>
#include <Base/Reader.h>
#include <Mod/MbDFEM/MbDFEMGlobal.h>
#include <Mod/Part/App/PartFeature.h>

namespace MbDFEM
{

class MbDFEMExport MbDMarker: public Part::Feature
{
    PROPERTY_HEADER_WITH_OVERRIDE(MbDFEM::MbDMarker);

public:
    MbDMarker();
    ~MbDMarker() override = default;

    App::PropertyLinkSubHidden Geometry;

    const char* getViewProviderName() const override
    {
        return "MbDFEMGui::ViewProviderMbDMarker";
    }

    App::DocumentObjectExecReturn* execute() override;

protected:
    void handleChangedPropertyName(Base::XMLReader& reader,
                                   const char* typeName,
                                   const char* propName) override;
};

}  // namespace MbDFEM
