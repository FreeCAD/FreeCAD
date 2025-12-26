/*
 * ViewCAMSimulator.h
 *
 *  Created on: 30.06.2025
 *      Author: jffmichi
 */

#ifndef PATHSIMULATOR_VIEWCAMSIMULATOR_H
#define PATHSIMULATOR_VIEWCAMSIMULATOR_H

#include <Gui/MDIView.h>

class SoCamera;

namespace Gui
{
class View3DSettings;
}  // namespace Gui

namespace CAMSimulator
{
class GuiDisplay;
class DlgCAMSimulator;
class Dummy3DViewer;
class View3DSettings;
class CAMSettings;

class ViewCAMSimulator: public Gui::MDIView
{
public:
    ViewCAMSimulator(
        Gui::Document* pcDocument,
        QWidget* parent,
        Qt::WindowFlags wflags = Qt::WindowFlags()
    );

    ViewCAMSimulator* clone() override;

    static ViewCAMSimulator& instance();
    DlgCAMSimulator& dlg();

    bool onMsg(const char* pMsg, const char** ppReturn) override;
    bool onHasMsg(const char* pMsg) const override;

private Q_SLOTS:
    void onSimulationStarted();

private:
    void initCamera();
    void cloneCamera(SoCamera& camera);
    void applySettings();

protected:
    GuiDisplay* mGui = nullptr;
    DlgCAMSimulator* mDlg = nullptr;
    Dummy3DViewer* mDummyViewer = nullptr;

    std::unique_ptr<View3DSettings> mViewSettings;
    std::unique_ptr<CAMSettings> mCAMSettings;
};

}  // namespace CAMSimulator

#endif /* SRC_MOD_CAM_PATHSIMULATOR_APPGL_VIEWCAMSIMULATOR_H_ */
