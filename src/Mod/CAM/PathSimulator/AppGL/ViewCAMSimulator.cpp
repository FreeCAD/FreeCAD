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

#include "PreCompiled.h"

#include "ViewCAMSimulator.h"

#include <string_view>

#include <QStackedWidget>
#include <QStackedLayout>
#include <QPointer>

#include <Inventor/nodes/SoPerspectiveCamera.h>
#include <Inventor/nodes/SoOrthographicCamera.h>

#include <App/Document.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
#include <Gui/SoFCDB.h>
#include <Gui/Camera.h>
#include <Gui/Navigation/NavigationStyle.h>
#include "GuiDisplay.h"
#include "DlgCAMSimulator.h"
#include "Dummy3DViewer.h"
#include "View3DSettings.h"
#include "CAMSettings.h"

using namespace std::literals;
using namespace Gui;

namespace CAMSimulator
{

static QPointer<ViewCAMSimulator> viewCAMSimulator;

ViewCAMSimulator::ViewCAMSimulator(Gui::Document* pcDocument, QWidget* parent, Qt::WindowFlags wflags)
    : Gui::MDIView(pcDocument, parent, wflags)
{

    // Under certain conditions, e.g. when docking/undocking the cam simulator, we need to create a
    // new widget (due to some OpenGL bug). The new widget becomes THE cam simulator.

    viewCAMSimulator = this;

    mDlg = new DlgCAMSimulator;
    mDlg->setAttribute(Qt::WA_TransparentForMouseEvents);

    mGui = new GuiDisplay;
    mDummyViewer = new Dummy3DViewer;

    mDlg->connectTo(*mGui, *mDummyViewer);

    connect(mDlg, &DlgCAMSimulator::simulationStarted, this, &ViewCAMSimulator::onSimulationStarted);

    // call apply settings only after mDummyViewer and mDlg have been initialized

    applySettings();
    initCamera();

    auto stack = new QStackedWidget;
    static_cast<QStackedLayout*>(stack->layout())->setStackingMode(QStackedLayout::StackAll);

    stack->addWidget(mGui);
    stack->addWidget(mDlg);

#if 1

    stack->addWidget(mDummyViewer);

    setCentralWidget(stack);

#else

    mDummyViewer->discardPaintEvent_ = false;

    auto container = new QWidget;
    auto container_layout = new QHBoxLayout;
    container->setLayout(container_layout);
    container_layout->addWidget(stack, 1);
    container_layout->addWidget(mDummyViewer, 1);

    setCentralWidget(container);

#endif
}

bool ViewCAMSimulator::onMsg(const char* pMsg, const char** ppReturn)
{
    (void)ppReturn;

    // TODO: this is a near 1-to-1 code duplication from View3DInventor.cpp

    if (pMsg == "ViewFit"sv) {
        mDummyViewer->viewAll();
        return true;
    }
    else if (strcmp("GetCamera", pMsg) == 0) {
        SoCamera* Cam = mDummyViewer->getSoRenderManager()->getCamera();
        if (!Cam) {
            return false;
        }
        *ppReturn = SoFCDB::writeNodesToString(Cam).c_str();
        return true;
    }
    else if (pMsg == "ViewBottom"sv) {
        mDummyViewer->setCameraOrientation(Camera::rotation(Camera::Bottom));
        return true;
    }
    else if (pMsg == "ViewFront"sv) {
        mDummyViewer->setCameraOrientation(Camera::rotation(Camera::Front));
        return true;
    }
    else if (pMsg == "ViewLeft"sv) {
        mDummyViewer->setCameraOrientation(Camera::rotation(Camera::Left));
        return true;
    }
    else if (pMsg == "ViewRear"sv) {
        mDummyViewer->setCameraOrientation(Camera::rotation(Camera::Rear));
        return true;
    }
    else if (pMsg == "ViewRight"sv) {
        mDummyViewer->setCameraOrientation(Camera::rotation(Camera::Right));
        return true;
    }
    else if (pMsg == "ViewTop"sv) {
        mDummyViewer->setCameraOrientation(Camera::rotation(Camera::Top));
        return true;
    }
    else if (pMsg == "ViewAxo"sv) {
        mDummyViewer->setCameraOrientation(Camera::rotation(Camera::Isometric));
        return true;
    }
    else if (pMsg == "ViewDimetric"sv) {
        mDummyViewer->setCameraOrientation(Camera::rotation(Camera::Dimetric));
        return true;
    }
    else if (pMsg == "ViewTrimetric"sv) {
        mDummyViewer->setCameraOrientation(Camera::rotation(Camera::Trimetric));
        return true;
    }
    else if (pMsg == "OrthographicCamera"sv) {
        mDummyViewer->setCameraType(SoOrthographicCamera::getClassTypeId());
        return true;
    }
    else if (pMsg == "PerspectiveCamera"sv) {
        mDummyViewer->setCameraType(SoPerspectiveCamera::getClassTypeId());
        return true;
    }
    else if (pMsg == "ZoomIn"sv) {
        mDummyViewer->navigationStyle()->zoomIn();
        return true;
    }
    else if (pMsg == "ZoomOut"sv) {
        mDummyViewer->navigationStyle()->zoomOut();
        return true;
    }

    return false;
}

bool ViewCAMSimulator::onHasMsg(const char* pMsg) const
{
    constexpr std::string_view supported[] = {
        "ViewFit",
        "GetCamera",
        "ViewBottom",
        "ViewFront",
        "ViewLeft",
        "ViewRear",
        "ViewRight",
        "ViewTop",
        "ViewAxo",
        "ViewDimetric",
        "ViewTrimetric",
        "OrthographicCamera",
        "PerspectiveCamera",
        "ZoomIn",
        "ZoomOut",
    };

    const auto it = std::find(std::begin(supported), std::end(supported), pMsg);
    return it != std::end(supported);
}

void ViewCAMSimulator::onSimulationStarted()
{
    // fit camera to scene

    mDummyViewer->viewAll();

    // window title and activate

    App::Document* doc = App::GetApplication().getActiveDocument();
    setWindowTitle(tr("%1 - New CAM Simulator").arg(QString::fromUtf8(doc->getName())));

    Gui::getMainWindow()->setActiveWindow(this);
    show();
}

void ViewCAMSimulator::initCamera()
{
    Gui::Document* doc = Gui::Application::Instance->activeDocument();
    if (!doc) {
        return;
    }

    auto view = dynamic_cast<View3DInventor*>(doc->getActiveView());
    if (!view) {
        return;
    }

    cloneCamera(*view->getViewer()->getCamera());
}

void ViewCAMSimulator::cloneCamera(SoCamera& camera)
{
    const std::string str = SoFCDB::writeNodesToString(&camera);
    mDummyViewer->setCamera(str.c_str());
}

void ViewCAMSimulator::applySettings()
{
    assert(mDummyViewer && mDlg);

    const ParameterGrp::handle hGrpView = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/View"
    );

    const ParameterGrp::handle hGrpCAM = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/CAM"
    );

    mViewSettings = std::make_unique<CAMSimulator::View3DSettings>(hGrpView, *mDummyViewer, *mDlg);
    mCAMSettings = std::make_unique<CAMSettings>(hGrpCAM, *mDlg);

    mViewSettings->applySettings();
    mCAMSettings->applySettings();
}

ViewCAMSimulator* ViewCAMSimulator::clone()
{
    auto viewCam = new ViewCAMSimulator(_pcDocument, nullptr);

    viewCam->cloneFrom(*this);
    viewCam->mDlg->cloneFrom(*mDlg);
    viewCam->mDummyViewer->cloneFrom(*mDummyViewer);

    // camera

    SoCamera& camera = *mDummyViewer->getSoRenderManager()->getCamera();
    viewCam->cloneCamera(camera);

    return viewCam;
}

ViewCAMSimulator& ViewCAMSimulator::instance()
{
    if (!viewCAMSimulator) {
        viewCAMSimulator = new ViewCAMSimulator(nullptr, nullptr);
        getMainWindow()->addWindow(viewCAMSimulator);
    }

    return *viewCAMSimulator;
}

DlgCAMSimulator& ViewCAMSimulator::dlg()
{
    return *mDlg;
}

}  // namespace CAMSimulator
