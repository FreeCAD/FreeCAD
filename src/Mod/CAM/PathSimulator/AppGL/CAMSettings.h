/*
 * CAMSettings.h
 *
 *  Created on: 28.07.2025
 *      Author: jffmichi
 */

#ifndef PATHSIMULATOR_CAMSETTINGS_H_
#define PATHSIMULATOR_CAMSETTINGS_H_

#include <Base/Parameter.h>

namespace CAMSimulator
{

class DlgCAMSimulator;

class CAMSettings: public ParameterGrp::ObserverType
{
public:
    explicit CAMSettings(ParameterGrp::handle hGrp, DlgCAMSimulator& dlg);
    ~CAMSettings();

    void applySettings();
    void OnChange(ParameterGrp::SubjectType& rCaller, ParameterGrp::MessageType Reason) override;

private:
    ParameterGrp::handle hGrp;

    DlgCAMSimulator& mDlg;
};

}  // namespace CAMSimulator

#endif
