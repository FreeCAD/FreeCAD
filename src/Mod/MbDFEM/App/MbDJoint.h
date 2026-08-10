// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "MbDItemIJ.h"

namespace MbDFEM
{

class MbDFEMExport MbDJoint: public MbDItemIJ
{
    PROPERTY_HEADER_WITH_OVERRIDE(MbDFEM::MbDJoint);

public:
    static const char* JointTypeEnums[];

    MbDJoint();
    ~MbDJoint() override = default;

    App::PropertyEnumeration jointType;
    App::PropertyFloat gearRatio;
    App::PropertyFloat pitchRadius;
    App::PropertyFloatList fxs;
    App::PropertyFloatList fys;
    App::PropertyFloatList fzs;
    App::PropertyFloatList txs;
    App::PropertyFloatList tys;
    App::PropertyFloatList tzs;

    void onChanged(const App::Property* prop) override;

    const char* getViewProviderName() const override
    {
        return "MbDFEMGui::ViewProviderMbDJoint";
    }
};

}  // namespace MbDFEM
