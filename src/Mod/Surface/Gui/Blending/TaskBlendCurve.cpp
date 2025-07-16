/***************************************************************************
 *   Copyright (c) 2025 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/

#include "PreCompiled.h"
#ifndef _PreComp_
#include <QAction>
#include <QMenu>
#include <QMessageBox>
#include <QTimer>
#endif

#include <App/Document.h>
#include <Base/Tools.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/CommandT.h>
#include <Gui/Control.h>
#include <Gui/Selection/SelectionObject.h>
#include <Gui/Tools.h>
#include <Gui/Widgets.h>
#include <Mod/Part/Gui/ViewProvider.h>
#include <Mod/Surface/App/Blending/FeatureBlendCurve.h>

#include "TaskBlendCurve.h"
#include "ui_TaskBlendCurve.h"
#include "ViewProviderBlendCurve.h"


using namespace SurfaceGui;

class BlendCurvePanel::EdgeSelection: public Gui::SelectionFilterGate
{
public:
    explicit EdgeSelection(Surface::FeatureBlendCurve* editedObject)
        : Gui::SelectionFilterGate(nullPointer())
        , editedObject(editedObject)
    {}
    ~EdgeSelection() override = default;

    EdgeSelection(const EdgeSelection&) = delete;
    EdgeSelection& operator=(const EdgeSelection&) = delete;
    EdgeSelection(EdgeSelection&&) = delete;
    EdgeSelection& operator=(EdgeSelection&&) = delete;
    /**
     * Allow the user to pick only edges.
     */
    bool allow(App::Document* doc, App::DocumentObject* pObj, const char* sSubName) override
    {
        (void)doc;

        // don't allow references to itself
        if (pObj == editedObject) {
            return false;
        }
        if (!pObj->isDerivedFrom<Part::Feature>()) {
            return false;
        }

        if (Base::Tools::isNullOrEmpty(sSubName)) {
            return false;
        }

        std::string element(sSubName);
        return (element.substr(0, 4) == "Edge");
    }

private:
    Surface::FeatureBlendCurve* editedObject;
};

// ----------------------------------------------------------------------------

BlendCurvePanel::BlendCurvePanel(ViewProviderBlendCurve* vp)
    : ui(new Ui_BlendCurve())
    , vp(vp)
{
    ui->setupUi(this);

    initControls();
    setupConnections();
    bindProperties();
}

BlendCurvePanel::~BlendCurvePanel() = default;

void BlendCurvePanel::setupConnections()
{
    // clang-format off
    connect(ui->buttonFirstEdge,
            &QToolButton::toggled,
            this,
            &BlendCurvePanel::onFirstEdgeButton);
    connect(ui->buttonSecondEdge,
            &QToolButton::toggled,
            this,
            &BlendCurvePanel::onSecondEdgeButton);
    connect(ui->contFirstEdge,
            qOverload<int>(&QComboBox::currentIndexChanged),
            this,
            &BlendCurvePanel::onFirstEdgeContChanged);
    connect(ui->contSecondEdge,
            qOverload<int>(&QComboBox::currentIndexChanged),
            this,
            &BlendCurvePanel::onSecondEdgeContChanged);
    connect(ui->paramFirstEdge,
            qOverload<double>(&QDoubleSpinBox::valueChanged),
            this,
            &BlendCurvePanel::onFirstEdgeParameterChanged);
    connect(ui->paramSecondEdge,
            qOverload<double>(&QDoubleSpinBox::valueChanged),
            this,
            &BlendCurvePanel::onSecondEdgeParameterChanged);
    connect(ui->sizeFirstEdge,
            qOverload<double>(&QDoubleSpinBox::valueChanged),
            this,
            &BlendCurvePanel::onFirstEdgeSizeChanged);
    connect(ui->sizeSecondEdge,
            qOverload<double>(&QDoubleSpinBox::valueChanged),
            this,
            &BlendCurvePanel::onSecondEdgeSizeChanged);
    // clang-format on
}

void BlendCurvePanel::initControls()
{
    initSubLinks();
    initContinuity();
    initParameter();
    initSize();
}

void BlendCurvePanel::initSubLinks()
{
    auto fea = vp->getObject<Surface::FeatureBlendCurve>();

    ui->firstEdgeEdit->setText(linkToString(fea->StartEdge));
    ui->secondEdgeEdit->setText(linkToString(fea->EndEdge));
}

void BlendCurvePanel::initContinuity()
{
    auto fea = vp->getObject<Surface::FeatureBlendCurve>();

    constexpr long maxCont = 4;
    // clang-format off
    ui->contFirstEdge->setCurrentIndex(static_cast<int>(std::min(maxCont, fea->StartContinuity.getValue())));
    ui->contSecondEdge->setCurrentIndex(static_cast<int>(std::min(maxCont, fea->EndContinuity.getValue())));
    // clang-format on
}

void BlendCurvePanel::initParameter()
{
    auto fea = vp->getObject<Surface::FeatureBlendCurve>();

    const double minPara = fea->StartParameter.getMinimum();
    const double maxPara = fea->StartParameter.getMaximum();
    const double stepsPara = fea->StartParameter.getStepSize();
    ui->paramFirstEdge->setRange(minPara, maxPara);
    ui->paramFirstEdge->setSingleStep(stepsPara);
    ui->paramSecondEdge->setRange(minPara, maxPara);
    ui->paramSecondEdge->setSingleStep(stepsPara);
    ui->paramFirstEdge->setValue(fea->StartParameter.getValue());
    ui->paramSecondEdge->setValue(fea->EndParameter.getValue());
}

void BlendCurvePanel::initSize()
{
    auto fea = vp->getObject<Surface::FeatureBlendCurve>();

    const double minSize = fea->StartSize.getMinimum();
    const double maxSize = fea->StartSize.getMaximum();
    const double stepsSize = fea->StartSize.getStepSize();
    ui->sizeFirstEdge->setRange(minSize, maxSize);
    ui->sizeFirstEdge->setSingleStep(stepsSize);
    ui->sizeSecondEdge->setRange(minSize, maxSize);
    ui->sizeSecondEdge->setSingleStep(stepsSize);
    ui->sizeFirstEdge->setValue(fea->StartSize.getValue());
    ui->sizeSecondEdge->setValue(fea->EndSize.getValue());
}

void BlendCurvePanel::bindProperties()
{
    auto fea = vp->getObject<Surface::FeatureBlendCurve>();

    ui->paramFirstEdge->bind(fea->StartParameter);
    ui->sizeFirstEdge->bind(fea->StartSize);
    ui->paramSecondEdge->bind(fea->EndParameter);
    ui->sizeSecondEdge->bind(fea->EndSize);
}

void BlendCurvePanel::onFirstEdgeButton(bool checked)
{
    if (checked) {
        onStartSelection();
        selectionMode = StartEdge;
        onUncheckSecondEdgeButton();
    }
    else {
        exitSelectionMode();
    }
}

void BlendCurvePanel::onSecondEdgeButton(bool checked)
{
    if (checked) {
        onStartSelection();
        selectionMode = EndEdge;
        onUncheckFirstEdgeButton();
    }
    else {
        exitSelectionMode();
    }
}

void BlendCurvePanel::onUncheckFirstEdgeButton()
{
    QSignalBlocker block(ui->buttonFirstEdge);
    ui->buttonFirstEdge->setChecked(false);
}

void BlendCurvePanel::onUncheckSecondEdgeButton()
{
    QSignalBlocker block(ui->buttonSecondEdge);
    ui->buttonSecondEdge->setChecked(false);
}

void BlendCurvePanel::onFirstEdgeContChanged(int index)
{
    if (vp.expired()) {
        return;
    }

    auto fea = vp->getObject<Surface::FeatureBlendCurve>();
    fea->StartContinuity.setValue(index);
    fea->recomputeFeature();
}

void BlendCurvePanel::onSecondEdgeContChanged(int index)
{
    if (vp.expired()) {
        return;
    }

    auto fea = vp->getObject<Surface::FeatureBlendCurve>();
    fea->EndContinuity.setValue(index);
    fea->recomputeFeature();
}

void BlendCurvePanel::onFirstEdgeParameterChanged(double value)
{
    if (vp.expired()) {
        return;
    }

    auto fea = vp->getObject<Surface::FeatureBlendCurve>();
    fea->StartParameter.setValue(value);
    fea->recomputeFeature();
}

void BlendCurvePanel::onSecondEdgeParameterChanged(double value)
{
    if (vp.expired()) {
        return;
    }

    auto fea = vp->getObject<Surface::FeatureBlendCurve>();
    fea->EndParameter.setValue(value);
    fea->recomputeFeature();
}

void BlendCurvePanel::onFirstEdgeSizeChanged(double value)
{
    if (vp.expired()) {
        return;
    }

    auto fea = vp->getObject<Surface::FeatureBlendCurve>();
    fea->StartSize.setValue(value);
    fea->recomputeFeature();
}

void BlendCurvePanel::onSecondEdgeSizeChanged(double value)
{
    if (vp.expired()) {
        return;
    }

    auto fea = vp->getObject<Surface::FeatureBlendCurve>();
    fea->EndSize.setValue(value);
    fea->recomputeFeature();
}

void BlendCurvePanel::onStartSelection()
{
    if (vp.expired()) {
        return;
    }

    auto gate = new EdgeSelection(vp->getObject<Surface::FeatureBlendCurve>());
    Gui::Selection().addSelectionGate(gate);
}

void BlendCurvePanel::exitSelectionMode()
{
    Gui::Selection().clearSelection();
    Gui::Selection().rmvSelectionGate();
    selectionMode = None;
}

void BlendCurvePanel::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (selectionMode == None) {
        return;
    }

    if (msg.Type != Gui::SelectionChanges::AddSelection) {
        return;
    }

    if (selectionMode == StartEdge) {
        setStartEdge(msg.Object.getObject(), msg.Object.getSubName());
        onUncheckFirstEdgeButton();
    }
    else if (selectionMode == EndEdge) {
        setEndEdge(msg.Object.getObject(), msg.Object.getSubName());
        onUncheckSecondEdgeButton();
    }

    QTimer::singleShot(50, this, &BlendCurvePanel::exitSelectionMode);
}

QString BlendCurvePanel::linkToString(const App::PropertyLinkSub& link)
{
    auto obj = link.getValue();
    const auto& sub = link.getSubValues();
    std::string name = sub.empty() ? "" : sub.front();

    return QString::fromLatin1("%1 [%2]").arg(QString::fromLatin1(obj->Label.getValue()),
                                              QString::fromStdString(name));
}

void BlendCurvePanel::setStartEdge(App::DocumentObject* obj, const std::string& subname)
{
    if (vp.expired()) {
        return;
    }

    auto fea = vp->getObject<Surface::FeatureBlendCurve>();
    fea->StartEdge.setValue(obj, {{subname}});
    fea->recomputeFeature();
    ui->firstEdgeEdit->setText(linkToString(fea->StartEdge));
}

void BlendCurvePanel::setEndEdge(App::DocumentObject* obj, const std::string& subname)
{
    if (vp.expired()) {
        return;
    }

    auto fea = vp->getObject<Surface::FeatureBlendCurve>();
    fea->EndEdge.setValue(obj, {{subname}});
    fea->recomputeFeature();
    ui->secondEdgeEdit->setText(linkToString(fea->EndEdge));
}

void BlendCurvePanel::changeEvent(QEvent* e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    else {
        QWidget::changeEvent(e);
    }
}

void BlendCurvePanel::open()
{
    checkOpenCommand();
    clearSelection();
}

void BlendCurvePanel::clearSelection()
{
    Gui::Selection().clearSelection();
}

void BlendCurvePanel::checkOpenCommand()
{
    if (!Gui::Command::hasPendingCommand()) {
        Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Edit blending curve"));
    }
}

bool BlendCurvePanel::accept()
{
    Gui::cmdGuiDocument(vp->getObject(), "resetEdit()");
    Gui::Command::commitCommand();
    Gui::Command::updateActive();
    return true;
}

bool BlendCurvePanel::reject()
{
    Gui::Command::abortCommand();
    Gui::cmdGuiDocument(vp->getObject(), "resetEdit()");
    Gui::Command::updateActive();
    return true;
}

// ----------------------------------------------------------------------------

TaskBlendCurve::TaskBlendCurve(ViewProviderBlendCurve* vp)
    : widget {new BlendCurvePanel(vp)}
{
    addTaskBox(Gui::BitmapFactory().pixmap("Surface_BlendCurve"), widget);
}

void TaskBlendCurve::open()
{
    widget->open();
}

bool TaskBlendCurve::accept()
{
    return widget->accept();
}

bool TaskBlendCurve::reject()
{
    return widget->reject();
}
