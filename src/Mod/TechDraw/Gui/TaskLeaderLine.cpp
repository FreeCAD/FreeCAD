/***************************************************************************
 *   Copyright (c) 2019 WandererFan <wandererfan@gmail.com>                *
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

#include <App/Document.h>
#include <Base/Console.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/Selection/Selection.h>
#include <Gui/ViewProvider.h>
#include <Mod/TechDraw/App/ArrowPropEnum.h>
#include <Mod/TechDraw/App/DrawLeaderLine.h>
#include <Mod/TechDraw/App/DrawPage.h>
#include <Mod/TechDraw/App/DrawView.h>
#include <Mod/TechDraw/App/LineGroup.h>

#include "TaskLeaderLine.h"
#include "ui_TaskLeaderLine.h"
#include "DrawGuiUtil.h"
#include "MDIViewPage.h"
#include "PreferencesGui.h"
#include "QGVPage.h"
#include "ViewProviderLeader.h"
#include "ViewProviderPage.h"
#include "TechDrawLeaderLineHandler.h"


using namespace Gui;
using namespace TechDraw;
using namespace TechDrawGui;

TaskLeaderLine::TaskLeaderLine(TechDrawGui::ViewProviderLeader* leadVP) :
    ui(new Ui_TaskLeaderLine),
    m_lineVP(leadVP),
    m_baseFeat(nullptr),
    m_basePage(nullptr),
    m_lineFeat(m_lineVP->getFeature()),
    m_createMode(false),
    m_saveContextPolicy(Qt::DefaultContextMenu),
    m_btnOK(nullptr),
    m_btnCancel(nullptr),
    m_saveX(0.0),
    m_saveY(0.0)
{
    //existence of leadVP is guaranteed by caller being ViewProviderLeaderLine.setEdit


    m_basePage = m_lineFeat->findParentPage();
    if (!m_basePage) {
        Base::Console().error("TaskRichAnno - bad parameters (2).  Cannot proceed.\n");
        return;
    }
    App::DocumentObject* obj = m_lineFeat->LeaderParent.getValue();
    if (obj) {
        if (obj->isDerivedFrom<TechDraw::DrawView>() )  {
            m_baseFeat = static_cast<TechDraw::DrawView*>(m_lineFeat->LeaderParent.getValue());
        }
    }

    Gui::Document* activeGui = Gui::Application::Instance->getDocument(m_basePage->getDocument());
    Gui::ViewProvider* vp = activeGui->getViewProvider(m_basePage);
    m_vpp = static_cast<ViewProviderPage*>(vp);

    //TODO: when/if leaders are allowed to be parented to Page, check for m_baseFeat will be removed
    if (!m_baseFeat || !m_basePage) {
        Base::Console().error("TaskLeaderLine - bad parameters (2).  Cannot proceed.\n");
        return;
    }

    ui->setupUi(this);

    setUiEdit();

    saveState();

    if (m_vpp->getMDIViewPage()) {
        m_saveContextPolicy = m_vpp->getMDIViewPage()->contextMenuPolicy();
    }
}

//ctor for creation
TaskLeaderLine::TaskLeaderLine(TechDraw::DrawPage* page) :
    ui(new Ui_TaskLeaderLine),
    m_lineVP(nullptr),
    m_baseFeat(nullptr),
    m_basePage(page),
    m_lineFeat(nullptr),
    m_createMode(true),
    m_saveContextPolicy(Qt::DefaultContextMenu),
    m_btnOK(nullptr),
    m_btnCancel(nullptr),
    m_saveX(0.0),
    m_saveY(0.0),
    m_vpp(nullptr),
    m_viewPage(nullptr)
{
    //existence of page is guaranteed by caller (CmdTechDrawLeaderLine::activated)
    if (!m_basePage) {
        Base::Console().error("TaskLeaderLine - bad parameters (1).  Cannot proceed.\n");
        return;
    }

    Gui::Document* activeGui = Gui::Application::Instance->getDocument(m_basePage->getDocument());
    Gui::ViewProvider* vp = activeGui->getViewProvider(m_basePage);
    m_vpp = static_cast<ViewProviderPage*>(vp);

    std::vector<Gui::SelectionObject> selection = Gui::Selection().getSelectionEx();
    if (!selection.empty()) {
        m_baseFeat = dynamic_cast<TechDraw::DrawView*>(selection.front().getObject());
    }
    ui->setupUi(this);

    setUiPrimary();

    connect(ui->cboxLeaderType, qOverload<int>(&QComboBox::currentIndexChanged),
                this, &TaskLeaderLine::onLeaderTypeChanged);

    if (m_vpp->getMDIViewPage()) {
        m_saveContextPolicy = m_vpp->getMDIViewPage()->contextMenuPolicy();
    }

    m_viewPage = m_vpp->getQGVPage();

    handler = new TechDrawLeaderLineHandler();
    handler->color.setValue<QColor>(ui->cpLineColor->color());
    handler->lineWidth = ui->dsbWeight->rawValue();
    handler->lineStyle = ui->cboxStyle->currentIndex();
    handler->startSymbol = ui->cboxStartSym->currentIndex();
    handler->endSymbol = ui->cboxEndSym->currentIndex();
    handler->kinkLength = ui->dsbKinkLength->rawValue();

    m_viewPage->activateHandler(handler);
}

void TaskLeaderLine::saveState()
{
    if (m_lineFeat) {
        m_savePoints = m_lineFeat->WayPoints.getValues();
        m_saveX = m_lineFeat->X.getValue();
        m_saveY = m_lineFeat->Y.getValue();
    }
}

void TaskLeaderLine::restoreState()
{
    if (m_lineFeat) {
        m_lineFeat->WayPoints.setValues(m_savePoints);
        m_lineFeat->X.setValue(m_saveX);
        m_lineFeat->Y.setValue(m_saveY);
    }
}

void TaskLeaderLine::updateTask()
{
//    blockUpdate = true;

//    blockUpdate = false;
}

void TaskLeaderLine::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
}

void TaskLeaderLine::setUiPrimary()
{
    enableVPUi(true);
    setWindowTitle(QObject::tr("New Leader Line"));

    DrawGuiUtil::loadArrowBox(ui->cboxStartSym);
    ArrowType aStyle = PreferencesGui::dimArrowStyle();
    ui->cboxStartSym->setCurrentIndex(static_cast<int>(aStyle));

    DrawGuiUtil::loadArrowBox(ui->cboxEndSym);
    ui->cboxEndSym->setCurrentIndex(static_cast<int>(TechDraw::ArrowType::NONE));

    ui->dsbWeight->setUnit(Base::Unit::Length);
    ui->dsbWeight->setMinimum(0);
    ui->dsbWeight->setValue(TechDraw::LineGroup::getDefaultWidth("Graphic"));

    ui->cpLineColor->setColor(PreferencesGui::leaderColor().asValue<QColor>());

    ui->dsbKinkLength->setUnit(Base::Unit::Length);
    ui->dsbKinkLength->setValue(5.0);

    connect(ui->cpLineColor, &ColorButton::changed, this, &TaskLeaderLine::onColorChanged);
    connect(ui->dsbWeight, qOverload<double>(&QuantitySpinBox::valueChanged), this, &TaskLeaderLine::onLineWidthChanged);
    connect(ui->cboxStyle, qOverload<int>(&QComboBox::currentIndexChanged), this, &TaskLeaderLine::onLineStyleChanged);
    connect(ui->cboxStartSym, qOverload<int>(&QComboBox::currentIndexChanged), this, &TaskLeaderLine::onStartSymbolChanged);
    connect(ui->cboxEndSym, qOverload<int>(&QComboBox::currentIndexChanged), this, &TaskLeaderLine::onEndSymbolChanged);
    connect(ui->dsbKinkLength, qOverload<double>(&QuantitySpinBox::valueChanged), this, &TaskLeaderLine::onKinkLengthChanged);

    onLeaderTypeChanged(ui->cboxLeaderType->currentIndex());
}

//switch widgets related to ViewProvider on/off
//there is no ViewProvider until some time after feature is created.
void TaskLeaderLine::enableVPUi(bool enable)
{
    ui->cpLineColor->setEnabled(enable);
    ui->dsbWeight->setEnabled(enable);
    ui->cboxStyle->setEnabled(enable);
}

void TaskLeaderLine::setUiEdit()
{
    enableVPUi(true);
    setWindowTitle(QObject::tr("Edit Leader Line"));

    ui->labelLeaderType->setVisible(false);
    ui->cboxLeaderType->setVisible(false);

    if (m_lineFeat) {
        DrawGuiUtil::loadArrowBox(ui->cboxStartSym);
        ui->cboxStartSym->setCurrentIndex(m_lineFeat->StartSymbol.getValue());
        connect(ui->cboxStartSym, qOverload<int>(&QComboBox::currentIndexChanged), this, &TaskLeaderLine::onStartSymbolChanged);
        DrawGuiUtil::loadArrowBox(ui->cboxEndSym);
        ui->cboxEndSym->setCurrentIndex(m_lineFeat->EndSymbol.getValue());
        connect(ui->cboxEndSym, qOverload<int>(&QComboBox::currentIndexChanged), this, &TaskLeaderLine::onEndSymbolChanged);

        if (m_lineFeat->Type.getValue() == 0 || m_lineFeat->Type.getValue() == 3) {
            ui->labelKinkLength->setVisible(true);
            ui->dsbKinkLength->setVisible(true);
            std::vector<Base::Vector3d> pts = m_lineFeat->WayPoints.getValues();
            Base::Vector3d startPt = pts[pts.size() - 2];
            Base::Vector3d endPt = pts[pts.size() - 1];
            double kinkLength = std::hypot(startPt.x - endPt.x, startPt.y - endPt.y);
            ui->dsbKinkLength->setValue(kinkLength);
        } else {
            ui->labelKinkLength->setVisible(false);
            ui->dsbKinkLength->setVisible(false);
        }
    }

    if (m_lineVP) {
        ui->cpLineColor->setColor(m_lineVP->Color.getValue().asValue<QColor>());
        ui->dsbWeight->setValue(m_lineVP->LineWidth.getValue());
        ui->cboxStyle->setCurrentIndex(m_lineVP->LineStyle.getValue());
    }

    connect(ui->cpLineColor, &ColorButton::changed, this, &TaskLeaderLine::onColorChanged);
    ui->dsbWeight->setMinimum(0);
    connect(ui->dsbWeight, qOverload<double>(&QuantitySpinBox::valueChanged), this, &TaskLeaderLine::onLineWidthChanged);
    connect(ui->cboxStyle, qOverload<int>(&QComboBox::currentIndexChanged), this, &TaskLeaderLine::onLineStyleChanged);

    ui->dsbKinkLength->setUnit(Base::Unit::Length);
    connect(ui->dsbKinkLength, qOverload<double>(&QuantitySpinBox::valueChanged), this, &TaskLeaderLine::onKinkLengthChanged);
}

void TaskLeaderLine::recomputeFeature()
{
    App::DocumentObject* objVP = m_lineVP->getObject();
    assert(objVP);
    objVP->recomputeFeature();
}

void TaskLeaderLine::onStartSymbolChanged()
{
    if (handler) {
        handler->startSymbol = ui->cboxStartSym->currentIndex();
        handler->updateLeader();
    }

    if (m_lineFeat) {
        m_lineFeat->StartSymbol.setValue(ui->cboxStartSym->currentIndex());
        recomputeFeature();
    }
}

void TaskLeaderLine::onEndSymbolChanged()
{
    if (handler) {
        handler->endSymbol = ui->cboxEndSym->currentIndex();
        handler->updateLeader();
    }

    if (m_lineFeat) {
        m_lineFeat->EndSymbol.setValue(ui->cboxEndSym->currentIndex());
        recomputeFeature();
    }
}

void TaskLeaderLine::onColorChanged()
{
    Base::Color ac;
    ac.setValue<QColor>(ui->cpLineColor->color());
    if (handler) {
        handler->color = ac;
        handler->updateLeader();
    }

    if (m_lineVP) {
        m_lineVP->Color.setValue(ac);
        recomputeFeature();
    }
}

void TaskLeaderLine::onLineWidthChanged()
{
    if (handler) {
        handler->lineWidth = ui->dsbWeight->rawValue();
        handler->updateLeader();
    }

    if (m_lineVP) {
        m_lineVP->LineWidth.setValue(ui->dsbWeight->rawValue());
        recomputeFeature();
    }
}

void TaskLeaderLine::onLineStyleChanged()
{
    if (handler) {
        handler->lineStyle = ui->cboxStyle->currentIndex();
        handler->updateLeader();
    }

    if (m_lineVP) {
        m_lineVP->LineStyle.setValue(ui->cboxStyle->currentIndex());
        recomputeFeature();
    }
}

void TaskLeaderLine::onKinkLengthChanged()
{
    if (handler) {
        handler->kinkLength = ui->dsbKinkLength->rawValue();
        handler->updateLeader();
    }

    if (m_lineFeat) {
        std::vector<Base::Vector3d> pts = m_lineFeat->WayPoints.getValues();
        if (pts.size() >= 3) {
            Base::Vector3d& kinkStart = pts[pts.size() - 2];
            Base::Vector3d& kinkEnd = pts[pts.size() - 1];

            double kinkLength = ui->dsbKinkLength->rawValue();
            Base::Vector3d mid = (kinkStart + kinkEnd) / 2.0;
            double halfLength = (kinkEnd.x >= kinkStart.x) ? kinkLength / 2.0 : -kinkLength / 2.0;

            kinkStart.x = mid.x - halfLength;
            kinkEnd.x = mid.x + halfLength;
            
            m_lineFeat->WayPoints.setValues(pts);
            recomputeFeature();
        }
    }
}

void TaskLeaderLine::onLeaderTypeChanged(int index)
{
    if (index == 0 || index == 3) {
        ui->labelKinkLength->setVisible(true);
        ui->dsbKinkLength->setVisible(true);
    } else {
        ui->labelKinkLength->setVisible(false);
        ui->dsbKinkLength->setVisible(false);
    }

    if (!handler) {
        return;
    }

    handler->currentLeaderType = static_cast<TechDrawLeaderLineHandler::LeaderType>(index);
    handler->updateLeader();
}


void TaskLeaderLine::updateLeaderFeature()
{
//    Base::Console().message("TTL::updateLeaderFeature()\n");
    int tid = Gui::Command::openActiveDocumentCommand(QT_TRANSLATE_NOOP("Command", "Edit Leader"));
    //waypoints & x, y are updated by QGILeaderLine (for edits only!)
    commonFeatureUpdate();
    Base::Color ac;
    ac.setValue<QColor>(ui->cpLineColor->color());

    Gui::Command::updateActive();
    Gui::Command::commitCommand(tid);

    if (m_baseFeat) {
        m_baseFeat->requestPaint();
    }
    m_lineFeat->requestPaint();
}

void TaskLeaderLine::commonFeatureUpdate()
{
    int start = ui->cboxStartSym->currentIndex();
    int end   = ui->cboxEndSym->currentIndex();
    m_lineFeat->StartSymbol.setValue(start);
    m_lineFeat->EndSymbol.setValue(end);
}

void TaskLeaderLine::saveButtons(QPushButton* btnOK,
                             QPushButton* btnCancel)
{
    m_btnOK = btnOK;
    m_btnCancel = btnCancel;
}

void TaskLeaderLine::enableTaskButtons(bool enable)
{
    m_btnOK->setEnabled(enable);
    m_btnCancel->setEnabled(enable);
}

//******************************************************************************

bool TaskLeaderLine::accept()
{
//    Base::Console().message("TTL::accept()\n");
    Gui::Document* doc = Gui::Application::Instance->getDocument(m_basePage->getDocument());
    if (!doc)
        return false;

    if (!getCreateMode())  {
        updateLeaderFeature();
    }

    Gui::Command::doCommand(Gui::Command::Gui, "Gui.ActiveDocument.resetEdit()");

    if (m_vpp->getMDIViewPage())
        m_vpp->getMDIViewPage()->setContextMenuPolicy(m_saveContextPolicy);

    if (handler) {
        m_viewPage->deactivateHandler();
    }

    return true;
}

bool TaskLeaderLine::reject()
{
    Gui::Document* doc = Gui::Application::Instance->getDocument(m_basePage->getDocument());
    if (!doc)
        return false;

    restoreState();

    //make sure any dangling objects are cleaned up
    Gui::Command::doCommand(Gui::Command::Gui, "App.activeDocument().recompute()");
    Gui::Command::doCommand(Gui::Command::Gui, "Gui.ActiveDocument.resetEdit()");

    if (m_vpp->getMDIViewPage()) {
        m_vpp->getMDIViewPage()->setContextMenuPolicy(m_saveContextPolicy);
    }

    if (handler) {
        handler->deleteLeaderLine();
        m_viewPage->deactivateHandler();
    }
    

    return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TaskDlgLeaderLine::TaskDlgLeaderLine(TechDraw::DrawPage* page)
    : TaskDialog()
{
    widget  = new TaskLeaderLine(page);
    taskbox = new Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("actions/TechDraw_LeaderLine"),
                                             widget->windowTitle(), true, nullptr);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

TaskDlgLeaderLine::TaskDlgLeaderLine(TechDrawGui::ViewProviderLeader* leadVP)
    : TaskDialog()
{
    widget  = new TaskLeaderLine(leadVP);
    taskbox = new Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("actions/TechDraw_LeaderLine"),
                                             widget->windowTitle(), true, nullptr);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

TaskDlgLeaderLine::~TaskDlgLeaderLine()
{
}

void TaskDlgLeaderLine::update()
{
//    widget->updateTask();
}

void TaskDlgLeaderLine::modifyStandardButtons(QDialogButtonBox* box)
{
    QPushButton* btnOK = box->button(QDialogButtonBox::Ok);
    QPushButton* btnCancel = box->button(QDialogButtonBox::Cancel);
    widget->saveButtons(btnOK, btnCancel);
}

//==== calls from the TaskView ===============================================================
void TaskDlgLeaderLine::open()
{
}

void TaskDlgLeaderLine::clicked(int)
{
}

bool TaskDlgLeaderLine::accept()
{
    widget->accept();
    return true;
}

bool TaskDlgLeaderLine::reject()
{
    widget->reject();
    return true;
}

#include <Mod/TechDraw/Gui/moc_TaskLeaderLine.cpp>
