/*
 * ViewCAMSimulator.cpp
 *
 *  Created on: 30.06.2025
 *      Author: jffmichi
 */

#include "ViewCAMSimulator.h"
#include "DlgCAMSimulator.h"

namespace CAMSimulator
{

ViewCAMSimulator::ViewCAMSimulator(Gui::Document* pcDocument,
                                   QWidget* parent,
                                   Qt::WindowFlags wflags)
    : Gui::MDIView(pcDocument, parent, wflags)
{
    mDlg = new DlgCAMSimulator(*this);
    setCentralWidget(mDlg);
}

ViewCAMSimulator* ViewCAMSimulator::clone()
{
    auto viewCam = new ViewCAMSimulator(_pcDocument, nullptr);

    viewCam->cloneFrom(*this);
    viewCam->mDlg->cloneFrom(*mDlg);

    return viewCam;
}

DlgCAMSimulator& ViewCAMSimulator::dlg()
{
    return *mDlg;
}

}  // namespace CAMSimulator
