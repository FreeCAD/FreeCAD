/*
 * CAMSettings.cpp
 *
 *  Created on: 28.07.2025
 *      Author: jffmichi
 */

#include "PreCompiled.h"

#include "CAMSettings.h"

#include <string_view>

#include "DlgCAMSimulator.h"

using namespace std::literals;

namespace CAMSimulator
{

CAMSettings::CAMSettings(ParameterGrp::handle hGrp, DlgCAMSimulator& dlg)
    : hGrp(hGrp)
    , mDlg(dlg)
{
    hGrp->Attach(this);
}

CAMSettings::~CAMSettings()
{
    hGrp->Detach(this);
}

void CAMSettings::applySettings()
{
    OnChange(*hGrp, "DefaultNormalPathColor");
    OnChange(*hGrp, "DefaultRapidPathColor");
}

void CAMSettings::OnChange(ParameterGrp::SubjectType& rCaller, ParameterGrp::MessageType Reason)
{
    if (Reason == "DefaultNormalPathColor"sv || Reason == "DefaultRapidPathColor"sv) {

        const unsigned long lcol
            = hGrp->GetUnsigned("DefaultNormalPathColor", 11141375UL);  // dark green (0,170,0)
        float lr, lg, lb;
        lr = ((lcol >> 24) & 0xff) / 255.0;
        lg = ((lcol >> 16) & 0xff) / 255.0;
        lb = ((lcol >> 8) & 0xff) / 255.0;

        const unsigned long rcol
            = hGrp->GetUnsigned("DefaultRapidPathColor", 2852126975UL);  // dark red (170,0,0)
        float rr, rg, rb;
        rr = ((rcol >> 24) & 0xff) / 255.0;
        rg = ((rcol >> 16) & 0xff) / 255.0;
        rb = ((rcol >> 8) & 0xff) / 255.0;

        mDlg.setPathColor(QColor::fromRgbF(lr, lg, lb), QColor::fromRgbF(rr, rg, rb));
    }
}

}  // namespace CAMSimulator
