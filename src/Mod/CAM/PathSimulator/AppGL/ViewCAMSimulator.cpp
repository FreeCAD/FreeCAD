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

namespace CAMSimulator
{
using namespace Gui;

class GuiCAMSimulator: public QWidget
{
public:
    explicit GuiCAMSimulator(QWidget* parent = nullptr)
        : QWidget(parent)
    {}

protected:
    void resizeEvent(QResizeEvent* event) override
    {
        QWidget::resizeEvent(event);
        setMask(childrenRegion());
    }
};

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
#if 1
        // do nothing
        (void)event;
#else
        View3DInventorViewer::paintEvent(event);
#endif
    }
};

class CAMSimulatorSettings: public View3DSettings
{
public:
    using View3DSettings::View3DSettings;

    void OnChange(ParameterGrp::SubjectType& rCaller, ParameterGrp::MessageType Reason) override
    {
        if (strcmp(Reason, "Orthographic") == 0) {
            // the new cam simulator currently only supports perspective camera
            for (auto _viewer : _viewers) {
                _viewer->setCameraType(SoPerspectiveCamera::getClassTypeId());
            }
        }
        else {
            View3DSettings::OnChange(rCaller, Reason);
        }
    }
};

ViewCAMSimulator::ViewCAMSimulator(Gui::Document* pcDocument,
                                   QWidget* parent,
                                   Qt::WindowFlags wflags)
    : Gui::MDIView(pcDocument, parent, wflags)
{

    mDummyViewer = new Dummy3DViewer;
    applySettings();

    SoCamera* camera = mDummyViewer->getCamera();
    assert(camera);

    if (Gui::Document* doc = Gui::Application::Instance->activeDocument()) {
        if (auto temp = dynamic_cast<View3DInventor*>(doc->getActiveView())) {
            const auto& c = *temp->getViewer()->getCamera();
            camera->position = c.position;
            camera->orientation = c.orientation;
        }
    }

    mDlg = new DlgCAMSimulator(*this, *camera);
    mDlg->setAttribute(Qt::WA_TransparentForMouseEvents);

    connect(mDlg, &DlgCAMSimulator::stockChanged, mDummyViewer, &Dummy3DViewer::setStockShape);
    connect(mDlg, &DlgCAMSimulator::baseChanged, mDummyViewer, &Dummy3DViewer::setBaseShape);

    auto gui = new GuiCAMSimulator;
    gui->setLayout(new QVBoxLayout);
    gui->layout()->addWidget(new QToolButton);

#if 1

    auto stack = new QStackedWidget;
    static_cast<QStackedLayout*>(stack->layout())->setStackingMode(QStackedLayout::StackAll);

    stack->addWidget(gui);
    stack->addWidget(mDlg);
    stack->addWidget(mDummyViewer);

    setCentralWidget(stack);

#else

    auto container = new QWidget;
    auto container_layout = new QHBoxLayout;
    container->setLayout(container_layout);
    container_layout->addWidget(mDlg, 1);
    container_layout->addWidget(mDummyViewer, 1);

    setCentralWidget(container);

#endif
}

void ViewCAMSimulator::applySettings()
{
    viewSettings = std::make_unique<CAMSimulatorSettings>(
        App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View"),
        mDummyViewer);
    viewSettings->applySettings();
    mDummyViewer->setEnabledNaviCube(false);
}

ViewCAMSimulator* ViewCAMSimulator::clone()
{
    auto viewCam = new ViewCAMSimulator(_pcDocument, nullptr);

    viewCam->cloneFrom(*this);
    viewCam->mDlg->cloneFrom(*mDlg);

    // camera

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
