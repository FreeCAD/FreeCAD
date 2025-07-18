/*
 * CAMSimulatorSettings.cpp
 *
 *  Created on: 15.07.2025
 *      Author: jffmichi
 */

#include "PreCompiled.h"

#include "CAMSimulatorSettings.h"

#include <Inventor/nodes/SoPerspectiveCamera.h>

#include "ViewCAMSimulator.h"
#include "DlgCAMSimulator.h"
#include "Dummy3DViewer.h"

using namespace Gui;

namespace CAMSimulator
{
CAMSimulatorSettings::CAMSimulatorSettings(ParameterGrp::handle hGrp,
                                           Dummy3DViewer& view,
                                           DlgCAMSimulator& dlg)
    : View3DSettings(hGrp, &view)
    , mView(view)
    , mDlg(dlg)
{}

void CAMSimulatorSettings::OnChange(ParameterGrp::SubjectType& rCaller,
                                    ParameterGrp::MessageType Reason)
{
    if (strcmp(Reason, "Orthographic") == 0) {
        // the new cam simulator currently only supports perspective camera
        mView.setCameraType(SoPerspectiveCamera::getClassTypeId());
    }
    else if (strcmp(Reason, "ShowNaviCube") == 0) {
        // always hide the navi cube
        mView.setEnabledNaviCube(false);
    }
    else {
        View3DSettings::OnChange(rCaller, Reason);
    }
}

}  // namespace CAMSimulator
