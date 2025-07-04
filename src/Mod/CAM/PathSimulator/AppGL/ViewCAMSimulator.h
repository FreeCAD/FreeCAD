/*
 * ViewCAMSimulator.h
 *
 *  Created on: 30.06.2025
 *      Author: jffmichi
 */

#ifndef PATHSIMULATOR_VIEWCAMSIMULATOR_H
#define PATHSIMULATOR_VIEWCAMSIMULATOR_H

#include <Gui/MDIView.h>

namespace CAMSimulator
{
class DlgCAMSimulator;

class ViewCAMSimulator: public Gui::MDIView
{
public:
    ViewCAMSimulator(Gui::Document* pcDocument,
                     QWidget* parent,
                     Qt::WindowFlags wflags = Qt::WindowFlags());

    ViewCAMSimulator* clone() override;

    DlgCAMSimulator& dlg();

protected:
    DlgCAMSimulator* mDlg;
};

}  // namespace CAMSimulator

#endif /* SRC_MOD_CAM_PATHSIMULATOR_APPGL_VIEWCAMSIMULATOR_H_ */
