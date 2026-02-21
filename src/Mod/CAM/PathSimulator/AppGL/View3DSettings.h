/*
 * CAMSimulatorSettings.h
 *
 *  Created on: 15.07.2025
 *      Author: jffmichi
 */

#ifndef PATHSIMULATOR_VIEW3DSETTINGS_H_
#define PATHSIMULATOR_VIEW3DSETTINGS_H_

#include <Gui/View3DSettings.h>

namespace CAMSimulator
{

class Dummy3DViewer;
class DlgCAMSimulator;

class View3DSettings: public Gui::View3DSettings
{
public:
    explicit View3DSettings(ParameterGrp::handle hGrp, Dummy3DViewer& view, DlgCAMSimulator& dlg);

    void OnChange(ParameterGrp::SubjectType& rCaller, ParameterGrp::MessageType Reason) override;

private:
    Dummy3DViewer& mView;
    DlgCAMSimulator& mDlg;
};

}  // namespace CAMSimulator

#endif
