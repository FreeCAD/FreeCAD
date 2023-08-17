/***************************************************************************
 *   Copyright (c) 2020 WandererFan <wandererfan@gmail.com>                *
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

#include <App/Document.h>
#include <Base/Console.h>
#include <Base/Tools.h>
#include <Base/UnitsApi.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/Selection.h>
#include <Gui/ViewProvider.h>
#include <Mod/TechDraw/App/DrawPage.h>
#include <Mod/TechDraw/App/DrawProjGroup.h>
#include <Mod/TechDraw/App/DrawProjGroupItem.h>
#include <Mod/TechDraw/App/DrawViewDetail.h>
#include <Mod/TechDraw/App/DrawViewPart.h>

#include "ui_TaskDetail.h"
#include "TaskDetail.h"
#include "QGIGhostHighlight.h"
#include "QGSPage.h"
#include "Rez.h"
#include "ViewProviderPage.h"


using namespace TechDrawGui;
using namespace TechDraw;
using namespace Gui;

static constexpr int CREATEMODE(0);
static constexpr int EDITMODE(1);

//creation constructor
TaskDetail::TaskDetail(TechDraw::DrawViewPart* baseFeat):
    ui(new Ui_TaskDetail),
    blockUpdate(false),
    m_ghost(nullptr),
    m_detailFeat(nullptr),
    m_baseFeat(baseFeat),
    m_basePage(nullptr),
    m_qgParent(nullptr),
    m_inProgressLock(false),
    m_btnOK(nullptr),
    m_btnCancel(nullptr),
    m_saveAnchor(Base::Vector3d(0.0, 0.0, 0.0)),
    m_saveRadius(0.0),
    m_saved(false),
    m_baseName(std::string()),
    m_pageName(std::string()),
    m_detailName(std::string()),
    m_doc(nullptr),
    m_mode(CREATEMODE),
    m_created(false)
{
    //existence of baseFeat checked in CmdTechDrawDetailView (Command.cpp)

    m_basePage = m_baseFeat->findParentPage();
    //it is possible that the basePage could be unparented and have no corresponding Page
    if (!m_basePage) {
        Base::Console().Error("TaskDetail - bad parameters - base page.  Can not proceed.\n");
        return;
    }

    m_baseName = m_baseFeat->getNameInDocument();
    m_doc      = m_baseFeat->getDocument();
    m_pageName = m_basePage->getNameInDocument();

    ui->setupUi(this);

    Gui::Document* activeGui = Gui::Application::Instance->getDocument(m_doc);
    Gui::ViewProvider* vp = activeGui->getViewProvider(m_basePage);
    m_vpp = static_cast<ViewProviderPage*>(vp);

    createDetail();
    setUiFromFeat();
    setWindowTitle(QObject::tr("New Detail View"));

    connect(ui->pbDragger, &QPushButton::clicked,
            this, &TaskDetail::onDraggerClicked);

    // the UI file uses keyboardTracking = false so that a recomputation
    // will only be triggered when the arrow keys of the spinboxes are used
    connect(ui->qsbX, qOverload<double>(&QuantitySpinBox::valueChanged),
            this, &TaskDetail::onXEdit);
    connect(ui->qsbY, qOverload<double>(&QuantitySpinBox::valueChanged),
            this, &TaskDetail::onYEdit);
    connect(ui->qsbRadius, qOverload<double>(&QuantitySpinBox::valueChanged),
            this, &TaskDetail::onRadiusEdit);
    connect(ui->cbScaleType, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &TaskDetail::onScaleTypeEdit);
    connect(ui->qsbScale, qOverload<double>(&QuantitySpinBox::valueChanged),
            this, &TaskDetail::onScaleEdit);
    connect(ui->leReference, &QLineEdit::editingFinished,
            this, &TaskDetail::onReferenceEdit);

    m_ghost = new QGIGhostHighlight();
    m_vpp->getQGSPage()->addItem(m_ghost);
    m_ghost->hide();
    connect(m_ghost, &QGIGhostHighlight::positionChange,
            this, &TaskDetail::onHighlightMoved);
}

//edit constructor
TaskDetail::TaskDetail(TechDraw::DrawViewDetail* detailFeat):
    ui(new Ui_TaskDetail),
    blockUpdate(false),
    m_ghost(nullptr),
    m_detailFeat(detailFeat),
    m_baseFeat(nullptr),
    m_basePage(nullptr),
    m_qgParent(nullptr),
    m_inProgressLock(false),
    m_btnOK(nullptr),
    m_btnCancel(nullptr),
    m_saveAnchor(Base::Vector3d(0.0, 0.0, 0.0)),
    m_saveRadius(0.0),
    m_saved(false),
    m_baseName(std::string()),
    m_pageName(std::string()),
    m_detailName(std::string()),
    m_doc(nullptr),
    m_mode(EDITMODE),
    m_created(false)
{
    if (!m_detailFeat)  {
        //should be caught in CMD caller
        Base::Console().Error("TaskDetail - bad parameters.  Can not proceed.\n");
        return;
    }

    m_doc = m_detailFeat->getDocument();
    m_detailName = m_detailFeat->getNameInDocument();

    m_basePage = m_detailFeat->findParentPage();
    if (m_basePage) {
        m_pageName = m_basePage->getNameInDocument();
    }

    App::DocumentObject* baseObj = m_detailFeat->BaseView.getValue();
    m_baseFeat = dynamic_cast<TechDraw::DrawViewPart*>(baseObj);
    if (m_baseFeat) {
        m_baseName = m_baseFeat->getNameInDocument();
    } else {
        Base::Console().Error("TaskDetail - no BaseView.  Can not proceed.\n");
        return;
    }

    ui->setupUi(this);

    Gui::Document* activeGui = Gui::Application::Instance->getDocument(m_basePage->getDocument());
    Gui::ViewProvider* vp = activeGui->getViewProvider(m_basePage);
    m_vpp = static_cast<ViewProviderPage*>(vp);

    saveDetailState();
    setUiFromFeat();
    setWindowTitle(QObject::tr("Edit Detail View"));

    connect(ui->pbDragger, &QPushButton::clicked,
            this, &TaskDetail::onDraggerClicked);

    // the UI file uses keyboardTracking = false so that a recomputation
    // will only be triggered when the arrow keys of the spinboxes are used
    connect(ui->qsbX, qOverload<double>(&QuantitySpinBox::valueChanged),
            this, &TaskDetail::onXEdit);
    connect(ui->qsbY, qOverload<double>(&QuantitySpinBox::valueChanged),
            this, &TaskDetail::onYEdit);
    connect(ui->qsbRadius, qOverload<double>(&QuantitySpinBox::valueChanged),
            this, &TaskDetail::onRadiusEdit);
    connect(ui->cbScaleType, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &TaskDetail::onScaleTypeEdit);
    connect(ui->qsbScale, qOverload<double>(&QuantitySpinBox::valueChanged),
            this, &TaskDetail::onScaleEdit);
    connect(ui->leReference, &QLineEdit::editingFinished,
            this, &TaskDetail::onReferenceEdit);

    m_ghost = new QGIGhostHighlight();
    m_vpp->getQGSPage()->addItem(m_ghost);
    m_ghost->hide();
    connect(m_ghost, &QGIGhostHighlight::positionChange,
            this, &TaskDetail::onHighlightMoved);
}

void TaskDetail::updateTask()
{
//    blockUpdate = true;

//    blockUpdate = false;
}

void TaskDetail::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
}

//save the start conditions
void TaskDetail::saveDetailState()
{
//    Base::Console().Message("TD::saveDetailState()\n");
    TechDraw::DrawViewDetail* dvd = getDetailFeat();
    m_saveAnchor = dvd->AnchorPoint.getValue();
    m_saveRadius  = dvd->Radius.getValue();
    m_saved = true;
}

void TaskDetail::restoreDetailState()
{
//    Base::Console().Message("TD::restoreDetailState()\n");
    TechDraw::DrawViewDetail* dvd = getDetailFeat();
    dvd->AnchorPoint.setValue(m_saveAnchor);
    dvd->Radius.setValue(m_saveRadius);
}

//***** ui stuff ***************************************************************

void TaskDetail::setUiFromFeat()
{
//    Base::Console().Message("TD::setUIFromFeat()\n");
    if (m_baseFeat) {
        std::string baseName = getBaseFeat()->getNameInDocument();
        ui->leBaseView->setText(Base::Tools::fromStdString(baseName));
    }

    Base::Vector3d anchor;

    TechDraw::DrawViewDetail* detailFeat = getDetailFeat();
    QString detailDisplay = QString::fromUtf8(detailFeat->getNameInDocument()) +
                            QString::fromUtf8(" / ") +
                            QString::fromUtf8(detailFeat->Label.getValue());
    ui->leDetailView->setText(detailDisplay);
    anchor = detailFeat->AnchorPoint.getValue();
    double radius = detailFeat->Radius.getValue();
    long ScaleType = detailFeat->ScaleType.getValue();
    double scale = detailFeat->Scale.getValue();
    QString ref = QString::fromUtf8(detailFeat->Reference.getValue());

    ui->pbDragger->setText(tr("Drag Highlight"));
    ui->pbDragger->setEnabled(true);
    int decimals = Base::UnitsApi::getDecimals();
    ui->qsbX->setUnit(Base::Unit::Length);
    ui->qsbX->setDecimals(decimals);
    ui->qsbX->setValue(anchor.x);
    ui->qsbY->setUnit(Base::Unit::Length);
    ui->qsbY->setDecimals(decimals);
    ui->qsbY->setValue(anchor.y);
    ui->qsbRadius->setDecimals(decimals);
    ui->qsbRadius->setUnit(Base::Unit::Length);
    ui->qsbRadius->setValue(radius);
    ui->qsbScale->setDecimals(decimals);
    ui->cbScaleType->setCurrentIndex(ScaleType);
    if (ui->cbScaleType->currentIndex() == 2) // only if custom scale
        ui->qsbScale->setEnabled(true);
    else
        ui->qsbScale->setEnabled(false);
    ui->qsbScale->setValue(scale);
    ui->leReference->setText(ref);
}

//update ui point fields after tracker finishes
void TaskDetail::updateUi(QPointF pos)
{
    ui->qsbX->setValue(pos.x());
    ui->qsbY->setValue(- pos.y());
}

void TaskDetail::enableInputFields(bool isEnabled)
{
    ui->qsbX->setEnabled(isEnabled);
    ui->qsbY->setEnabled(isEnabled);
    if (ui->cbScaleType->currentIndex() == 2) // only if custom scale
        ui->qsbScale->setEnabled(isEnabled);
    ui->qsbRadius->setEnabled(isEnabled);
    ui->leReference->setEnabled(isEnabled);
}

void TaskDetail::onXEdit()
{
    updateDetail();
}

void TaskDetail::onYEdit()
{
    updateDetail();
}

void TaskDetail::onRadiusEdit()
{
    updateDetail();
}

void TaskDetail::onScaleTypeEdit()
{
    TechDraw::DrawViewDetail* detailFeat = getDetailFeat();

     if (ui->cbScaleType->currentIndex() == 0) {
         // page scale
         ui->qsbScale->setEnabled(false);
         detailFeat->ScaleType.setValue(0.0);
         // set the page scale if there is a valid page
         if (m_basePage) {
             // set the page scale
             detailFeat->Scale.setValue(m_basePage->Scale.getValue());
             ui->qsbScale->setValue(m_basePage->Scale.getValue());
         }
         // finally update the view
         updateDetail();
    }
    else if (ui->cbScaleType->currentIndex() == 1) {
        // automatic scale (if view is too large to fit into page, it will be scaled down)
        ui->qsbScale->setEnabled(false);
        detailFeat->ScaleType.setValue(1.0);
        // updating the feature will trigger the rescaling
        updateDetail();
    }
    else if (ui->cbScaleType->currentIndex() == 2) {
        // custom scale
        ui->qsbScale->setEnabled(true);
        detailFeat->ScaleType.setValue(2.0);
        // no updateDetail() necessary since nothing visibly was changed
    }
}

void TaskDetail::onScaleEdit()
{
    updateDetail();
}

void TaskDetail::onReferenceEdit()
{
    updateDetail();
}

void TaskDetail::onDraggerClicked(bool clicked)
{
    Q_UNUSED(clicked);
    ui->pbDragger->setEnabled(false);
    enableInputFields(false);
    editByHighlight();
    return;
}

void TaskDetail::editByHighlight()
{
//    Base::Console().Message("TD::editByHighlight()\n");
    if (!m_ghost) {
        Base::Console().Error("TaskDetail::editByHighlight - no ghost object\n");
        return;
    }

    double scale = getBaseFeat()->getScale();
    m_vpp->getQGSPage()->clearSelection();
    m_ghost->setSelected(true);
    m_ghost->setRadius(ui->qsbRadius->rawValue() * scale);
    m_ghost->setPos(getAnchorScene());
    m_ghost->draw();
    m_ghost->show();
}

//dragEnd is in scene coords.
void TaskDetail::onHighlightMoved(QPointF dragEnd)
{
//    Base::Console().Message("TD::onHighlightMoved(%s) - highlight: %X\n",
//                            DrawUtil::formatVector(dragEnd).c_str(), m_ghost);
    ui->pbDragger->setEnabled(true);

    double scale = getBaseFeat()->getScale();
    double x = Rez::guiX(getBaseFeat()->X.getValue());
    double y = Rez::guiX(getBaseFeat()->Y.getValue());

    DrawViewPart* dvp = getBaseFeat();
    DrawProjGroupItem* dpgi = dynamic_cast<DrawProjGroupItem*>(dvp);
    if (dpgi) {
        DrawProjGroup* dpg = dpgi->getPGroup();
        if (!dpg) {
            Base::Console().Message("TD::getAnchorScene - projection group is confused\n");
            //TODO::throw something.
            return;
        }
        x += Rez::guiX(dpg->X.getValue());
        y += Rez::guiX(dpg->Y.getValue());
    }

    QPointF basePosScene(x, -y);                 //base position in scene coords
    QPointF anchorDisplace = dragEnd - basePosScene;
    QPointF newAnchorPos = Rez::appX(anchorDisplace / scale);

    updateUi(newAnchorPos);
    updateDetail();
    enableInputFields(true);
    m_ghost->setSelected(false);
    m_ghost->hide();
}

void TaskDetail::saveButtons(QPushButton* btnOK,
                             QPushButton* btnCancel)
{
    m_btnOK = btnOK;
    m_btnCancel = btnCancel;
}

void TaskDetail::enableTaskButtons(bool button)
{
    m_btnOK->setEnabled(button);
    m_btnCancel->setEnabled(button);
}

//***** Feature create & edit stuff *******************************************
void TaskDetail::createDetail()
{
//    Base::Console().Message("TD::createDetail()\n");
    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Create Detail View"));

    const std::string objectName{"Detail"};
    std::string m_detailName = m_doc->getUniqueObjectName(objectName.c_str());
    std::string generatedSuffix {m_detailName.substr(objectName.length())};

    Gui::Command::doCommand(Command::Doc, "App.activeDocument().addObject('TechDraw::DrawViewDetail', '%s')",
                            m_detailName.c_str());

    Gui::Command::doCommand(Command::Doc, "App.activeDocument().%s.translateLabel('DrawViewDetail', 'Detail', '%s')",
              m_detailName.c_str(), m_detailName.c_str());

    App::DocumentObject *docObj = m_doc->getObject(m_detailName.c_str());
    TechDraw::DrawViewDetail* dvd = dynamic_cast<TechDraw::DrawViewDetail *>(docObj);
    if (!dvd) {
        throw Base::TypeError("TaskDetail - new detail view not found\n");
    }
    m_detailFeat = dvd;

    dvd->Source.setValues(getBaseFeat()->Source.getValues());

    Gui::Command::doCommand(Command::Doc, "App.activeDocument().%s.BaseView = App.activeDocument().%s",
                            m_detailName.c_str(), m_baseName.c_str());
    Gui::Command::doCommand(Command::Doc, "App.activeDocument().%s.Direction = App.activeDocument().%s.Direction",
                            m_detailName.c_str(), m_baseName.c_str());
    Gui::Command::doCommand(Command::Doc, "App.activeDocument().%s.XDirection = App.activeDocument().%s.XDirection",
                            m_detailName.c_str(), m_baseName.c_str());
    Gui::Command::doCommand(Command::Doc, "App.activeDocument().%s.Scale = App.activeDocument().%s.Scale",
                            m_detailName.c_str(), m_baseName.c_str());
    Gui::Command::doCommand(Command::Doc, "App.activeDocument().%s.addView(App.activeDocument().%s)",
                            m_pageName.c_str(), m_detailName.c_str());

    Gui::Command::updateActive();
    Gui::Command::commitCommand();

    getBaseFeat()->requestPaint();
    m_created = true;
}

void TaskDetail::updateDetail()
{
//    Base::Console().Message("TD::updateDetail()\n");
    try {
        Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Update Detail"));
        double x = ui->qsbX->rawValue();
        double y = ui->qsbY->rawValue();
        Base::Vector3d temp(x, y, 0.0);
        TechDraw::DrawViewDetail* detailFeat = getDetailFeat();
        detailFeat->AnchorPoint.setValue(temp);
        double scale = ui->qsbScale->rawValue();
        detailFeat->Scale.setValue(scale);
        double radius = ui->qsbRadius->rawValue();
        detailFeat->Radius.setValue(radius);
        QString qRef = ui->leReference->text();
        std::string ref = Base::Tools::toStdString(qRef);
        detailFeat->Reference.setValue(ref);

        detailFeat->recomputeFeature();
        getBaseFeat()->requestPaint();
        Gui::Command::updateActive();
        Gui::Command::commitCommand();
    }
    catch (...) {
        //this is probably due to appl closing while dialog is still open
        Base::Console().Error("Task Detail - detail feature update failed.\n");
    }
}

//***** Getters ****************************************************************

//get the current Anchor highlight position in scene coords
QPointF TaskDetail::getAnchorScene()
{
    DrawViewPart* dvp = getBaseFeat();
    DrawProjGroupItem* dpgi = dynamic_cast<DrawProjGroupItem*>(dvp);
    DrawViewDetail* dvd = getDetailFeat();
    Base::Vector3d anchorPos = dvd->AnchorPoint.getValue();
    anchorPos.y = -anchorPos.y;
    Base::Vector3d basePos;
    double scale = 1;

    if (!dpgi) {          //base is normal view
        double x = dvp->X.getValue();
        double y = dvp->Y.getValue();
        basePos = Base::Vector3d (x, -y, 0.0);
        scale = dvp->getScale();
    } else {                       //part of projection group

        DrawProjGroup* dpg = dpgi->getPGroup();
        if (!dpg) {
            Base::Console().Message("TD::getAnchorScene - projection group is confused\n");
            //TODO::throw something.
            return QPointF(0.0, 0.0);
        }
        double x = dpg->X.getValue();
        x += dpgi->X.getValue();
        double y = dpg->Y.getValue();
        y += dpgi->Y.getValue();
        basePos = Base::Vector3d(x, -y, 0.0);
        scale = dpgi->getScale();
    }

    Base::Vector3d xyScene = Rez::guiX(basePos);
    Base::Vector3d anchorOffsetScene = Rez::guiX(anchorPos) * scale;
    Base::Vector3d netPos = xyScene + anchorOffsetScene;
    return QPointF(netPos.x, netPos.y);
}

// protects against stale pointers
DrawViewPart* TaskDetail::getBaseFeat()
{
//    Base::Console().Message("TD::getBaseFeat()\n");

    if (m_doc) {
        App::DocumentObject* baseObj = m_doc->getObject(m_baseName.c_str());
        if (baseObj) {
            return static_cast<DrawViewPart*>(baseObj);
        }
    }

    std::string msg = "TaskDetail - base feature " +
                        m_baseName +
                        " not found \n";
    throw Base::TypeError(msg);
    return nullptr;
}

// protects against stale pointers
DrawViewDetail* TaskDetail::getDetailFeat()
{
//    Base::Console().Message("TD::getDetailFeat()\n");

    if (m_doc) {
        App::DocumentObject* detailObj = m_doc->getObject(m_detailName.c_str());
        if (detailObj) {
            return static_cast<DrawViewDetail*>(detailObj);
        }
    }

    std::string msg = "TaskDetail - detail feature " +
                        m_detailName +
                        " not found \n";
//        throw Base::TypeError("TaskDetail - detail feature not found\n");
    throw Base::TypeError(msg);
    return nullptr;
}

//******************************************************************************

bool TaskDetail::accept()
{
//    Base::Console().Message("TD::accept()\n");

    Gui::Document* doc = Gui::Application::Instance->getDocument(m_basePage->getDocument());
    if (!doc)
        return false;

    m_ghost->hide();
    getDetailFeat()->requestPaint();
    getBaseFeat()->requestPaint();
    Gui::Command::doCommand(Gui::Command::Gui, "Gui.ActiveDocument.resetEdit()");

    return true;
}

bool TaskDetail::reject()
{
//    Base::Console().Message("TD::reject()\n");
    Gui::Document* doc = Gui::Application::Instance->getDocument(m_basePage->getDocument());
    if (!doc)
        return false;

    m_ghost->hide();
    if (m_mode == CREATEMODE) {
        if (m_created) {
            Gui::Command::doCommand(Gui::Command::Gui, "App.activeDocument().removeObject('%s')",
                                    m_detailName.c_str());
        }
    } else {
        restoreDetailState();
        getDetailFeat()->recomputeFeature();
        getBaseFeat()->requestPaint();
    }

    Gui::Command::doCommand(Gui::Command::Gui, "App.activeDocument().recompute()");
    Gui::Command::doCommand(Gui::Command::Gui, "Gui.ActiveDocument.resetEdit()");

    return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TaskDlgDetail::TaskDlgDetail(TechDraw::DrawViewPart* baseFeat)
    : TaskDialog()
{
    widget  = new TaskDetail(baseFeat);
    taskbox = new Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("actions/TechDraw_DetailView"),
                                             widget->windowTitle(), true, nullptr);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

TaskDlgDetail::TaskDlgDetail(TechDraw::DrawViewDetail* detailFeat)
    : TaskDialog()
{
    widget  = new TaskDetail(detailFeat);
    taskbox = new Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("actions/TechDraw_DetailView"),
                                             widget->windowTitle(), true, nullptr);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

TaskDlgDetail::~TaskDlgDetail()
{
}

void TaskDlgDetail::update()
{
//    widget->updateTask();
}

void TaskDlgDetail::modifyStandardButtons(QDialogButtonBox* box)
{
    QPushButton* btnOK = box->button(QDialogButtonBox::Ok);
    QPushButton* btnCancel = box->button(QDialogButtonBox::Cancel);
    widget->saveButtons(btnOK, btnCancel);
}

//==== calls from the TaskView ===============================================================
void TaskDlgDetail::open()
{
}

void TaskDlgDetail::clicked(int)
{
}

bool TaskDlgDetail::accept()
{
    widget->accept();
    return true;
}

bool TaskDlgDetail::reject()
{
    widget->reject();
    return true;
}

#include <Mod/TechDraw/Gui/moc_TaskDetail.cpp>
