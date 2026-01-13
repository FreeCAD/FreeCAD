// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2024 Shai Seger <shaise at gmail>                       *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

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

#endif /* PATHSIMULATOR_VIEWCAMSIMULATOR_H */
