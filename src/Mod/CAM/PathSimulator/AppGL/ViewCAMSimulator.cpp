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

#include "CAMSettings.h"
#include "DlgCAMSimulator.h"
#include "Dummy3DViewer.h"
#include "GuiDisplay.h"
#include "View3DSettings.h"
#include <App/Document.h>
#include <Gui/Application.h>
#include <Gui/Camera.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/Navigation/NavigationStyle.h>
#include <Gui/SoFCDB.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
#include <Inventor/nodes/SoOrthographicCamera.h>
#include <Inventor/nodes/SoPerspectiveCamera.h>
#include <QPointer>
#include <QStackedLayout>
#include <QStackedWidget>
#include <string_view>

using namespace std::literals;
using namespace Gui;

namespace CAMSimulator
{

static QPointer<ViewCAMSimulator> viewCAMSimulator;

ViewCAMSimulator::ViewCAMSimulator(Gui::Document* pcDocument, QWidget* parent, Qt::WindowFlags wflags)
    : Gui::MDIViewWithCamera(pcDocument, parent, wflags)
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

bool ViewCAMSimulator::onMsg(const char* pMsg)
{
    // TODO: this is a near 1-to-1 code duplication from View3DInventor.cpp

    if (pMsg == "ViewFit"sv) {
        mDummyViewer->viewAll();
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
    else if (pMsg == "StoreWorkingView"sv) {
        mDummyViewer->saveHomePosition();
        return true;
    }
    else if (pMsg == "RecallWorkingView"sv) {
        if (mDummyViewer->hasHomePosition()) {
            mDummyViewer->resetToHomePosition();
        }
        return true;
    }

    return false;
}

bool ViewCAMSimulator::onHasMsg(const char* pMsg) const
{
    constexpr std::string_view supported[] = {
        "ViewFit",
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
        "StoreWorkingView",
    };

    const auto it = std::find(std::begin(supported), std::end(supported), pMsg);
    if (it != std::end(supported)) {
        return true;
    }

    if (pMsg == "RecallWorkingView"sv) {
        return mDummyViewer->hasHomePosition();
    }

    return false;
}

const std::string& ViewCAMSimulator::getCamera() const
{
    SoCamera* Cam = mDummyViewer->getSoRenderManager()->getCamera();
    if (!Cam) {
        throw Base::RuntimeError("Could not find reference to CAM Simulator camera");
    }
    return SoFCDB::writeNodesToString(Cam);
}

bool ViewCAMSimulator::setCamera(const char* pCamera)
{
    return mDummyViewer->setCamera(pCamera);
}

void ViewCAMSimulator::onSimulationStarted()
{
    // fit camera to scene

    mDummyViewer->viewAll();

    // window title and activate

    App::Document* doc = getAppDocument();
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
    return clone(nullptr);
}

ViewCAMSimulator* ViewCAMSimulator::clone(Gui::Document* doc)
{
    auto viewCam = new ViewCAMSimulator(doc ? doc : getGuiDocument(), nullptr);

    viewCam->cloneFrom(*this);
    viewCam->mDlg->cloneFrom(*mDlg);
    viewCam->mDummyViewer->cloneFrom(*mDummyViewer);

    // camera

    SoCamera& camera = *mDummyViewer->getSoRenderManager()->getCamera();
    viewCam->cloneCamera(camera);

    return viewCam;
}

ViewCAMSimulator& ViewCAMSimulator::instance(Gui::Document* doc)
{
    // The first call comes from CAMSim::resetSimulation giving us the correct document. All
    // subsequent calls don't provide a document and just use the one from resetSimulation.

    if (!viewCAMSimulator) {
        // No simulator exists so we create a new one. We need to make sure that we provide a valid
        // document to the constructor. Otherwise the newly created MDIView will be considered
        // "passive" and will not be able to receive messages from e.g. the view menu.

        if (!doc) {
            throw std::invalid_argument("trying to create a ViewCAMSimulator without a document");
        }

        viewCAMSimulator = new ViewCAMSimulator(doc, nullptr);
        getMainWindow()->addWindow(viewCAMSimulator);
    }
    else if (doc && doc != viewCAMSimulator->getGuiDocument()) {
        // If a document is provided, we make sure that the returned instance belongs to that
        // document. Looking at the constructor of MDIView, it seems the document of a view should
        // not be changed after creating the view (even though there is a setDocument function in
        // BaseView, the constructor of MDIView sets some variables, e.g. ActiveObjects that don't
        // seem to be updated when calling setDocument in BaseView). If the document is different,
        // we clone the simulator with the new document. Cloning the simulator is reasonably fast
        // and should not be noticeable to the user.

        // There was a previous attempt where the simulator was initialized without a document and
        // therefore becoming a "passive" view. Messages where then also delivered to passive views
        // but that caused some issues in the Assembly workbench. See issue #29901.

        auto old = viewCAMSimulator.data();

        viewCAMSimulator = old->clone(doc);
        getMainWindow()->addWindow(viewCAMSimulator);

        old->deleteSelf();
    }

    return *viewCAMSimulator;
}

DlgCAMSimulator& ViewCAMSimulator::dlg()
{
    return *mDlg;
}

}  // namespace CAMSimulator
