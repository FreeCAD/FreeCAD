/*
 * ViewCAMSimulator.cpp
 *
 *  Created on: 30.06.2025
 *      Author: jffmichi
 */

#include "PreCompiled.h"

#include "ViewCAMSimulator.h"

#include <string_view>

#include <QStackedWidget>
#include <QStackedLayout>
#include <QPointer>

#include <App/Document.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
#include <Gui/SoFCDB.h>
#include <Inventor/nodes/SoCamera.h>

#include "DlgCAMSimulator.h"
#include "GuiDisplay.h"
#include "Dummy3DViewer.h"
#include "CAMSimulatorSettings.h"

using namespace std::literals;
using namespace Gui;

namespace CAMSimulator
{

static QPointer<ViewCAMSimulator> viewCAMSimulator;

ViewCAMSimulator::ViewCAMSimulator(Gui::Document* pcDocument,
                                   QWidget* parent,
                                   Qt::WindowFlags wflags)
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

    // clang-format off
    connect(mDlg, &DlgCAMSimulator::simulationStarted, this, &ViewCAMSimulator::onSimulationStarted);
    // clang-format on

    // call apply settings only after mDummyViewer and mDlg have been initialized

    initCamera();
    applySettings();

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

    if (pMsg == "ViewFit"sv) {
        mDummyViewer->viewAll();
        return true;
    }

    return false;
}

bool ViewCAMSimulator::onHasMsg(const char* pMsg) const
{
    if (pMsg == "ViewFit"sv) {
        return true;
    }

    return false;
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

    const ParameterGrp::handle hGrp =
        App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");

    mViewSettings = std::make_unique<CAMSimulatorSettings>(hGrp, *mDummyViewer, *mDlg);
    mViewSettings->applySettings();
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
