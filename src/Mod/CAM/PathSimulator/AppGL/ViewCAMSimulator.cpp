/*
 * ViewCAMSimulator.cpp
 *
 *  Created on: 30.06.2025
 *      Author: jffmichi
 */

#include "PreCompiled.h"

#include "ViewCAMSimulator.h"
#include "DlgCAMSimulator.h"
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
#include <Gui/View3DSettings.h>
#include <Gui/ViewProvider.h>
#include <Gui/SoFCDB.h>
#include <Inventor/nodes/SoPerspectiveCamera.h>
#include <Inventor/nodes/SoSeparator.h>

#include <QStackedWidget>
#include <QStackedLayout>
#include <QToolButton>

#include "GuiDisplay.h"

namespace CAMSimulator
{
using namespace Gui;

class TopoShapeViewProvider: public ViewProvider
{
public:
    explicit TopoShapeViewProvider(const Part::TopoShape& shape)
    {
        std::stringstream s;
        shape.exportFaceSet(0.1f, 0.0f, {}, s);

        const auto str = s.str();
        SoInput in;
        in.setBuffer(str.data(), str.length());

        SoNode* stock;
        SoDB::read(&in, stock);

        pcRoot->addChild(stock);
    }
};

class Dummy3DViewer: public View3DInventorViewer
{
public:
    Dummy3DViewer(QWidget* parent = nullptr)
        : View3DInventorViewer(parent)
    {}

    void setStockShape(const Part::TopoShape& shape)
    {
        // addViewProvider(new TopoShapeViewProvider(shape));
    }

    void setBaseShape(const Part::TopoShape& shape)
    {
        addViewProvider(new TopoShapeViewProvider(shape));
        viewAll();
    }

protected:
    void paintEvent(QPaintEvent* event) override
    {
        if (discardPaintEvent_) {
            return;
        }

        View3DInventorViewer::paintEvent(event);
    }

public:
    bool discardPaintEvent_ = true;
};

class CAMSimulatorSettings: public View3DSettings
{
public:
    explicit CAMSimulatorSettings(ParameterGrp::handle hGrp,
                                  View3DInventorViewer& view,
                                  DlgCAMSimulator& dlg)
        : View3DSettings(hGrp, &view)
        , mView(view)
        , mDlg(dlg)
    {}

    void OnChange(ParameterGrp::SubjectType& rCaller, ParameterGrp::MessageType Reason) override
    {
        if (strcmp(Reason, "Orthographic") == 0) {
            // the new cam simulator currently only supports perspective camera
            mView.setCameraType(SoPerspectiveCamera::getClassTypeId());
            mDlg.setCamera(*mView.getCamera());
        }
        else if (strcmp(Reason, "ShowNaviCube") == 0) {
            // always hide the navi cube
            mView.setEnabledNaviCube(false);
        }
        else {
            View3DSettings::OnChange(rCaller, Reason);
        }
    }

private:
    View3DInventorViewer& mView;
    DlgCAMSimulator& mDlg;
};

ViewCAMSimulator::ViewCAMSimulator(Gui::Document* pcDocument,
                                   QWidget* parent,
                                   Qt::WindowFlags wflags)
    : Gui::MDIView(pcDocument, parent, wflags)
{
    mDummyViewer = new Dummy3DViewer;

    mDlg = new DlgCAMSimulator(*this);
    mDlg->setAttribute(Qt::WA_TransparentForMouseEvents);

    // call apply settings only after mDummyViewer and mDlg have been initialized

    initCamera();
    applySettings();

    connect(mDlg, &DlgCAMSimulator::stockChanged, mDummyViewer, &Dummy3DViewer::setStockShape);
    connect(mDlg, &DlgCAMSimulator::baseChanged, mDummyViewer, &Dummy3DViewer::setBaseShape);

    mGui = new GuiDisplay;

#if 1

    auto stack = new QStackedWidget;
    static_cast<QStackedLayout*>(stack->layout())->setStackingMode(QStackedLayout::StackAll);

    stack->addWidget(mGui);
    stack->addWidget(mDlg);
    stack->addWidget(mDummyViewer);

    setCentralWidget(stack);

#else

    mDummyViewer->discardPaintEvent_ = false;

    auto container = new QWidget;
    auto container_layout = new QHBoxLayout;
    container->setLayout(container_layout);
    container_layout->addWidget(mDlg, 1);
    container_layout->addWidget(mDummyViewer, 1);

    setCentralWidget(container);

#endif
}

void ViewCAMSimulator::initCamera()
{
    Gui::Document* doc = Gui::Application::Instance->activeDocument();
    if (!doc) {
        return;
    }

    auto temp = dynamic_cast<View3DInventor*>(doc->getActiveView());
    if (!temp) {
        return;
    }

    const char* ppReturn = nullptr;
    temp->onMsg("GetCamera", &ppReturn);
    if (!ppReturn) {
        return;
    }

    mDummyViewer->setCamera(ppReturn);
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

    // camera

    // TODO: use "GetCamera" message?

    const auto& src =
        dynamic_cast<const SoPerspectiveCamera&>(*mDummyViewer->getSoRenderManager()->getCamera());

    auto& dest = dynamic_cast<SoPerspectiveCamera&>(
        *viewCam->mDummyViewer->getSoRenderManager()->getCamera());

    dest.position = src.position;
    dest.orientation = src.orientation;
    dest.nearDistance = src.nearDistance;
    dest.farDistance = src.farDistance;
    dest.focalDistance = src.focalDistance;

    return viewCam;
}

DlgCAMSimulator& ViewCAMSimulator::dlg()
{
    return *mDlg;
}

}  // namespace CAMSimulator
