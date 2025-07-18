/*
 * CAMSimulatorSettings.h
 *
 *  Created on: 15.07.2025
 *      Author: jffmichi
 */

#ifndef PATHSIMULATOR_CAMSIMULATORSETTINGS_H_
#define PATHSIMULATOR_CAMSIMULATORSETTINGS_H_

#include <Gui/View3DSettings.h>

namespace CAMSimulator
{

class Dummy3DViewer;
class DlgCAMSimulator;

class CAMSimulatorSettings: public Gui::View3DSettings
{
public:
    explicit CAMSimulatorSettings(ParameterGrp::handle hGrp,
                                  Dummy3DViewer& view,
                                  DlgCAMSimulator& dlg);

    void OnChange(ParameterGrp::SubjectType& rCaller, ParameterGrp::MessageType Reason) override;

private:
    Dummy3DViewer& mView;
    DlgCAMSimulator& mDlg;
};

}  // namespace CAMSimulator

#endif
