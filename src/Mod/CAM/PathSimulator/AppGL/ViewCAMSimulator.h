/*
 * ViewCAMSimulator.h
 *
 *  Created on: 30.06.2025
 *      Author: jffmichi
 */

#ifndef PATHSIMULATOR_VIEWCAMSIMULATOR_H
#define PATHSIMULATOR_VIEWCAMSIMULATOR_H

#include <Gui/MDIView.h>

namespace Gui
{
class View3DSettings;
}  // namespace Gui

namespace CAMSimulator
{
class DlgCAMSimulator;
class Dummy3DViewer;

class ViewCAMSimulator: public Gui::MDIView
{
public:
    ViewCAMSimulator(Gui::Document* pcDocument,
                     QWidget* parent,
                     Qt::WindowFlags wflags = Qt::WindowFlags());

    ViewCAMSimulator* clone() override;

    DlgCAMSimulator& dlg();

private:
    void applySettings();

protected:
    DlgCAMSimulator* mDlg;

    Dummy3DViewer* mDummyViewer;
    std::unique_ptr<Gui::View3DSettings> viewSettings;
};

}  // namespace CAMSimulator

#endif /* SRC_MOD_CAM_PATHSIMULATOR_APPGL_VIEWCAMSIMULATOR_H_ */
