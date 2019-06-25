/***************************************************************************
 *   Copyright (c) 2019 Wandererfan <wandererfan@gmail.com                 *
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

#ifndef _PreComp_
#include <cmath>
#endif // #ifndef _PreComp_

#include <QStatusBar>
#include <QGraphicsScene>

#include <Base/Console.h>
#include <Base/Tools.h>

#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Control.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/Selection.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>

#include <Mod/TechDraw/App/DrawPage.h>
#include <Mod/TechDraw/App/DrawUtil.h>
#include <Mod/TechDraw/App/DrawView.h>
#include <Mod/TechDraw/App/DrawLeaderLine.h>

#include <Mod/TechDraw/Gui/ui_TaskLeaderLine.h>

#include "DrawGuiStd.h"
#include "QGVPage.h"
#include "QGIView.h"
#include "QGIPrimPath.h"
#include "MDIViewPage.h"
#include "ViewProviderPage.h"
#include "ViewProviderLeader.h"
#include "QGTracker.h"
#include "QGEPath.h"
#include "QGILeaderLine.h"
#include "Rez.h"

#include "TaskLeaderLine.h"

using namespace Gui;
using namespace TechDraw;
using namespace TechDrawGui;

//ctor for edit
TaskLeaderLine::TaskLeaderLine(TechDrawGui::ViewProviderLeader* leadVP) :
    ui(new Ui_TaskLeaderLine),
    m_tracker(nullptr),
    m_lineVP(leadVP),
    m_baseFeat(nullptr),
    m_basePage(nullptr),
    m_lineFeat(nullptr),
    m_createMode(false),
    m_leadLine(nullptr),
    m_inProgressLock(false),
    m_qgLine(nullptr),
    m_pbTrackerState(TRACKEREDIT)
{
    if (m_lineVP == nullptr)  {
        //should be caught in CMD caller
        Base::Console().Error("TaskLeaderLine - bad parameters.  Can not proceed.\n");
        return;
    }
    ui->setupUi(this);
    
    m_lineFeat = m_lineVP->getFeature();

    App::DocumentObject* obj = m_lineFeat->LeaderParent.getValue();
    if ( obj->isDerivedFrom(TechDraw::DrawView::getClassTypeId()) )  {
        m_baseFeat = static_cast<TechDraw::DrawView*>(m_lineFeat->LeaderParent.getValue());
    }
    m_basePage = m_lineFeat->findParentPage();

    //TODO: when/if leaders are allowed to be parented to Page, check for m_baseFeat will be removed
    if ( (m_lineFeat == nullptr) ||
         (m_baseFeat == nullptr) ||
         (m_basePage == nullptr) ) {
        Base::Console().Error("TaskLeaderLine - bad parameters (2).  Can not proceed.\n");
        return;
    }

    setUiEdit();

    m_mdi = m_lineVP->getMDIViewPage();
    m_scene = m_mdi->m_scene;
    m_view = m_mdi->getQGVPage();

    m_attachPoint = Rez::guiX(Base::Vector3d(m_lineFeat->X.getValue(),
                                            -m_lineFeat->Y.getValue(),
                                             0.0));

    connect(ui->pbTracker, SIGNAL(clicked(bool)),
            this, SLOT(onTrackerClicked(bool)));
    connect(ui->pbCancelEdit, SIGNAL(clicked(bool)),
            this, SLOT(onCancelEditClicked(bool)));
    ui->pbCancelEdit->setEnabled(false);

    saveState();

    m_trackerMode = QGTracker::TrackerMode::Line;
}

//ctor for creation
TaskLeaderLine::TaskLeaderLine(TechDraw::DrawView* baseFeat,
                               TechDraw::DrawPage* page) :
    ui(new Ui_TaskLeaderLine),
    m_tracker(nullptr),
    m_lineVP(nullptr),
    m_baseFeat(baseFeat),
    m_basePage(page),
    m_lineFeat(nullptr),
    m_createMode(true),
    m_leadLine(nullptr),
    m_inProgressLock(false),
    m_qgLine(nullptr),
    m_pbTrackerState(TRACKERPICK)
{
    if ( (m_basePage == nullptr) ||
         (m_baseFeat == nullptr) )  {
        //should be caught in CMD caller
        Base::Console().Error("TaskLeaderLine - bad parameters.  Can not proceed.\n");
        return;
    }

    ui->setupUi(this);

    Gui::Document* activeGui = Gui::Application::Instance->getDocument(m_basePage->getDocument());
    Gui::ViewProvider* vp = activeGui->getViewProvider(m_basePage);
    ViewProviderPage* vpp = static_cast<ViewProviderPage*>(vp);
    m_mdi = vpp->getMDIViewPage();
    m_scene = m_mdi->m_scene;
    m_view = m_mdi->getQGVPage();

    setUiPrimary();
    
    connect(ui->pbTracker, SIGNAL(clicked(bool)),
            this, SLOT(onTrackerClicked(bool)));
    connect(ui->pbCancelEdit, SIGNAL(clicked(bool)),
            this, SLOT(onCancelEditClicked(bool)));
    ui->pbCancelEdit->setEnabled(false);

    m_trackerMode = QGTracker::TrackerMode::Line;
}

TaskLeaderLine::~TaskLeaderLine()
{
    delete ui;
}

void TaskLeaderLine::saveState()
{
    if (m_lineFeat != nullptr) {
        m_savePoints = m_lineFeat->WayPoints.getValues();
        m_saveX = m_lineFeat->X.getValue();
        m_saveY = m_lineFeat->Y.getValue();
    }
}

void TaskLeaderLine::restoreState()
{
    if (m_lineFeat != nullptr) {
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

void TaskLeaderLine::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
}

void TaskLeaderLine::setUiPrimary()
{
//    Base::Console().Message("TTL::setUiPrimary()\n");
    enableVPUi(false);
    setWindowTitle(QObject::tr("New Leader Line"));

    if (m_baseFeat != nullptr) {
        std::string baseName = m_baseFeat->getNameInDocument();
        ui->leBaseView->setText(Base::Tools::fromStdString(baseName));
    }
    ui->pbTracker->setText(QString::fromUtf8("Pick points"));
    ui->pbTracker->setEnabled(true);
    ui->pbCancelEdit->setEnabled(true);
   int aSize = getPrefArrowStyle() + 1;
    ui->cboxStartSym->setCurrentIndex(aSize);
}

void TaskLeaderLine::enableVPUi(bool b)
{
    ui->cpLineColor->setEnabled(b);
    ui->dsbWeight->setEnabled(b);
    ui->cboxStyle->setEnabled(b);
}

void TaskLeaderLine::setUiEdit()
{
//    Base::Console().Message("TTL::setUiEdit()\n");
    enableVPUi(true);
    setWindowTitle(QObject::tr("Edit Leader Line"));

    if (m_lineFeat != nullptr) {
        std::string baseName = m_lineFeat->LeaderParent.getValue()->getNameInDocument();
        ui->leBaseView->setText(Base::Tools::fromStdString(baseName));
        ui->cboxStartSym->setCurrentIndex(m_lineFeat->StartSymbol.getValue() + 1);
        ui->cboxEndSym->setCurrentIndex(m_lineFeat->EndSymbol.getValue() + 1);
        ui->pbTracker->setText(QString::fromUtf8("Edit points"));
        ui->pbTracker->setEnabled(true);
        ui->pbCancelEdit->setEnabled(true);
    }

    if (m_lineVP != nullptr) {
        ui->cpLineColor->setColor(m_lineVP->Color.getValue().asValue<QColor>());
        ui->dsbWeight->setValue(m_lineVP->LineWidth.getValue());
        ui->cboxStyle->setCurrentIndex(m_lineVP->LineStyle.getValue());
    }
}


//******************************************************************************
void TaskLeaderLine::createLeaderFeature(std::vector<Base::Vector3d> converted)
{
//    Base::Console().Message("TTL::createLeaderFeature()\n");
    m_leaderName = m_basePage->getDocument()->getUniqueObjectName("DrawLeaderLine");
    m_leaderType = "TechDraw::DrawLeaderLine";

    std::string PageName = m_basePage->getNameInDocument();

    Gui::Command::openCommand("Create Leader");
    Command::doCommand(Command::Doc,"App.activeDocument().addObject('%s','%s')",
                       m_leaderType.c_str(),m_leaderName.c_str());
    Command::doCommand(Command::Doc,"App.activeDocument().%s.addView(App.activeDocument().%s)",
                       PageName.c_str(),m_leaderName.c_str());
    Command::doCommand(Command::Doc,"App.activeDocument().%s.LeaderParent = App.activeDocument().%s",
                           m_leaderName.c_str(),m_baseFeat->getNameInDocument());

    App::DocumentObject* obj = m_basePage->getDocument()->getObject(m_leaderName.c_str());
    if (obj == nullptr) {
        throw Base::RuntimeError("TaskLeaderLine - new markup object not found");
    }
    if (obj->isDerivedFrom(TechDraw::DrawLeaderLine::getClassTypeId())) {
        m_lineFeat = static_cast<TechDraw::DrawLeaderLine*>(obj);
        m_lineFeat->setPosition(Rez::appX(m_attachPoint.x),Rez::appX(- m_attachPoint.y), true);
        if (!converted.empty()) {
            m_lineFeat->WayPoints.setValues(converted);
            if (m_lineFeat->AutoHorizontal.getValue()) {
                m_lineFeat->adjustLastSegment();
            }
        }
        commonFeatureUpdate();
    }
    
    Gui::Command::updateActive();
    Gui::Command::commitCommand();
    m_lineFeat->requestPaint();
}

void TaskLeaderLine::updateLeaderFeature(void)
{
//    Base::Console().Message("TTL::updateLeaderFeature()\n");
    Gui::Command::openCommand("Edit Leader");
    //waypoints & x,y are updated by QGILeaderLine (for edits only!)
    commonFeatureUpdate();
    App::Color ac;
    ac.setValue<QColor>(ui->cpLineColor->color());
    m_lineVP->Color.setValue(ac);
    m_lineVP->LineWidth.setValue(ui->dsbWeight->value());
    m_lineVP->LineStyle.setValue(ui->cboxStyle->currentIndex());

    Gui::Command::commitCommand();
    m_lineFeat->requestPaint();
}

void TaskLeaderLine::commonFeatureUpdate(void)
{
    int start = ui->cboxStartSym->currentIndex() - 1;
    int end   = ui->cboxEndSym->currentIndex() - 1;
    m_lineFeat->StartSymbol.setValue(start);
    m_lineFeat->EndSymbol.setValue(end);
}

void TaskLeaderLine::removeFeature(void)
{
//    Base::Console().Message("TTL::removeFeature()\n");
    if (m_lineFeat != nullptr) {
        if (m_createMode) {
            try {
                std::string PageName = m_basePage->getNameInDocument();
                Gui::Command::doCommand(Gui::Command::Gui,"App.activeDocument().%s.removeView(App.activeDocument().%s)",
                                        PageName.c_str(),m_lineFeat->getNameInDocument());
                Gui::Command::doCommand(Gui::Command::Gui,"App.activeDocument().removeObject('%s')",
                                         m_lineFeat->getNameInDocument());
            }
            catch (...) {
                Base::Console().Message("TTL::removeFeature - failed to delete feature\n");
                return;
            }
        } else {
            if (Gui::Command::hasPendingCommand()) {
                std::vector<std::string> undos = Gui::Application::Instance->activeDocument()->getUndoVector();
                Gui::Application::Instance->activeDocument()->undo(1);
            } else {
                Base::Console().Log("TaskLeaderLine: Edit mode - NO command is active\n");
            }
        }
    }
}

//********** Tracker routines *******************************************************************
void TaskLeaderLine::onTrackerClicked(bool b)
{
    Q_UNUSED(b);
//    Base::Console().Message("TTL::onTrackerClicked() m_pbTrackerState: %d\n",
//                            m_pbTrackerState);
    if ( (m_pbTrackerState == TRACKERSAVE) &&
         (getCreateMode())  ){
        if (m_tracker != nullptr) {
            m_tracker->terminateDrawing();
        }

        m_pbTrackerState = TRACKERPICK;
        ui->pbTracker->setText(QString::fromUtf8("Pick Points"));
        ui->pbCancelEdit->setEnabled(false);
        enableTaskButtons(true);

        setEditCursor(Qt::ArrowCursor);
        return;
    } else  if ( (m_pbTrackerState == TRACKERSAVE) &&
                 (!getCreateMode()) ) {
        if (m_qgLine != nullptr) {
            m_qgLine->closeEdit();
        }

        m_pbTrackerState = TRACKERPICK;
        ui->pbTracker->setText(QString::fromUtf8("Pick Points"));
        ui->pbCancelEdit->setEnabled(false);
        enableTaskButtons(true);

        setEditCursor(Qt::ArrowCursor);
        return;
    }

    //TRACKERPICK or TRACKEREDIT
    if (getCreateMode()) {
        m_inProgressLock = true;
        m_saveContextPolicy = m_mdi->contextMenuPolicy();
        m_mdi->setContextMenuPolicy(Qt::PreventContextMenu);
        m_trackerMode = QGTracker::TrackerMode::Line;
        setEditCursor(Qt::CrossCursor);
        startTracker();

        QString msg = tr("Pick a starting point for leader line");
        getMainWindow()->statusBar()->show();
        Gui::getMainWindow()->showMessage(msg,3000);
        ui->pbTracker->setText(QString::fromUtf8("Save Points"));
        ui->pbTracker->setEnabled(true);
        ui->pbCancelEdit->setEnabled(true);
        m_pbTrackerState = TRACKERSAVE;
        enableTaskButtons(false);
    } else {    //edit mode
        m_trackerPoints = m_lineFeat->WayPoints.getValues();
        if (!m_trackerPoints.empty()) {    //regular edit session
            m_inProgressLock = true;
            m_saveContextPolicy = m_mdi->contextMenuPolicy();
            m_mdi->setContextMenuPolicy(Qt::PreventContextMenu);
            QGVPage* qgvp = m_mdi->getQGVPage();
            QGIView* qgiv = qgvp->findQViewForDocObj(m_lineFeat);
            QGILeaderLine* qgLead = dynamic_cast<QGILeaderLine*>(qgiv);
            
            if (qgLead == nullptr) {
                //tarfu
                Base::Console().Error("TaskLeaderLine - can't find leader graphic\n");
                //now what? throw will generate "unknown unhandled exception"
            } else {
                m_qgLine = qgLead;
                connect(qgLead, SIGNAL(editComplete()),
                this, SLOT(onPointEditComplete()));
                qgLead->startPathEdit();
                QString msg = tr("Click and drag markers to adjust leader line");
                getMainWindow()->statusBar()->show();
                Gui::getMainWindow()->showMessage(msg,3000);
                ui->pbTracker->setText(QString::fromUtf8("Save changes"));
                ui->pbTracker->setEnabled(true);
                ui->pbCancelEdit->setEnabled(true);
                m_pbTrackerState = TRACKERSAVE;
                enableTaskButtons(false);
            }
        } else { // need to recreate leaderline
            m_inProgressLock = true;
            m_saveContextPolicy = m_mdi->contextMenuPolicy();
            m_mdi->setContextMenuPolicy(Qt::PreventContextMenu);
            m_trackerMode = QGTracker::TrackerMode::Line;
            setEditCursor(Qt::CrossCursor);
            startTracker();

            QString msg = tr("Pick a starting point for leader line");
            getMainWindow()->statusBar()->show();
            Gui::getMainWindow()->showMessage(msg,3000);
            ui->pbTracker->setText(QString::fromUtf8("Save changes"));
            ui->pbTracker->setEnabled(true);
            ui->pbCancelEdit->setEnabled(true);
            m_pbTrackerState = TRACKERSAVE;
            enableTaskButtons(false);
        }
    }
}

void TaskLeaderLine::startTracker(void)
{
//    Base::Console().Message("TTL::startTracker()\n");
    if (m_trackerMode == QGTracker::TrackerMode::None) {
        return;
    }

    if (m_tracker == nullptr) {
        m_tracker = new QGTracker(m_scene, m_trackerMode);
        QObject::connect(
            m_tracker, SIGNAL(drawingFinished(std::vector<QPointF>, QGIView*)),
            this     , SLOT  (onTrackerFinished(std::vector<QPointF>, QGIView*))
           );
    } else {
        //this is too harsh. but need to avoid restarting process
        throw Base::RuntimeError("TechDrawNewLeader - tracker already active\n");
    }
    setEditCursor(Qt::CrossCursor);
    QString msg = tr("Left click to set a point");
    Gui::getMainWindow()->statusBar()->show();
    Gui::getMainWindow()->showMessage(msg,3000);
}

void TaskLeaderLine::onTrackerFinished(std::vector<QPointF> pts, QGIView* qgParent)
{
//    Base::Console().Message("TTL::onTrackerFinished() - parent: %X\n",qgParent);
    if (pts.empty()) {
        Base::Console().Error("TaskLeaderLine - no points available\n");
        return;
    }

    if (qgParent == nullptr) {
        //do something;
        m_qgParent = findParentQGIV();
    } else {
        QGIView* qgiv = dynamic_cast<QGIView*>(qgParent);
        if (qgiv != nullptr) {
            m_qgParent = qgiv;
        } else {
            Base::Console().Message("TTL::onTrackerFinished - can't find parent graphic!\n");
            //blow up!?
            throw Base::RuntimeError("TaskLeaderLine - can not find parent graphic");
        }
    }

    if (m_qgParent != nullptr) {
        double scale = m_qgParent->getScale();
        QPointF mapped = m_qgParent->mapFromScene(pts.front()) / scale;
        m_attachPoint = Base::Vector3d(mapped.x(), mapped.y(), 0.0);
        trackerPointsFromQPoints(pts);
    } else {
        Base::Console().Message("TTL::onTrackerFinished - can't find parent graphic!\n");
        //blow up!?
        throw Base::RuntimeError("TaskLeaderLine - can not find parent graphic");
    }

    QString msg = tr("Press OK or Cancel to continue");
    getMainWindow()->statusBar()->show();
    Gui::getMainWindow()->showMessage(msg, 3000);
    enableTaskButtons(true);

    m_tracker->sleep(true);
    m_inProgressLock = false;
    ui->pbTracker->setEnabled(false);
    ui->pbCancelEdit->setEnabled(false);
    enableTaskButtons(true);
    setEditCursor(Qt::ArrowCursor);
}

void TaskLeaderLine::removeTracker(void)
{
//    Base::Console().Message("TTL::removeTracker()\n");
    if ( (m_tracker != nullptr) &&
         (m_tracker->scene() != nullptr) ) {
        m_scene->removeItem(m_tracker);
        delete m_tracker;
        m_tracker = nullptr;
    }
}

void TaskLeaderLine::onCancelEditClicked(bool b)
{
    Q_UNUSED(b);
//    Base::Console().Message("TTL::onCancelEditClicked() m_pbTrackerState: %d\n",
//                            m_pbTrackerState);
    abandonEditSession();
    if (m_lineFeat != nullptr) {
        m_lineFeat->requestPaint();
    }

    m_pbTrackerState = TRACKEREDIT;
    ui->pbTracker->setText(QString::fromUtf8("Edit points"));
    ui->pbCancelEdit->setEnabled(false);
    enableTaskButtons(true);

    m_inProgressLock = false;
    setEditCursor(Qt::ArrowCursor);
}

QGIView* TaskLeaderLine::findParentQGIV()
{
    QGIView* result = nullptr;
    if (m_baseFeat != nullptr) {
        Gui::ViewProvider* gvp = QGIView::getViewProvider(m_baseFeat);
        ViewProviderDrawingView* vpdv = dynamic_cast<ViewProviderDrawingView*>(gvp);
        if (vpdv != nullptr) {
            result = vpdv->getQView();
        }
    }
    return result;
}

void TaskLeaderLine::setEditCursor(QCursor c)
{
    if (m_baseFeat != nullptr) {
        QGIView* qgivBase = m_view->findQViewForDocObj(m_baseFeat);
        qgivBase->setCursor(c);
    }
}

//from 1:1 scale scene QPointF to zero origin Vector3d points
void TaskLeaderLine::trackerPointsFromQPoints(std::vector<QPointF> pts)
{
//    Base::Console().Message("TTL::trackerPointsFromQPoints(%d)\n", pts.size());
    m_trackerPoints.clear();
    for (auto& p: pts) {
        QPointF mapped = p - pts.front();
        Base::Vector3d convert(mapped.x(),mapped.y(),0.0);
        m_trackerPoints.push_back(convert);
    }
}

//******************************************************************************
//void TaskLeaderLine::onPointEditComplete(std::vector<QPointF> pts, QGIView* parent)
void TaskLeaderLine::onPointEditComplete(void)
{
//    Base::Console().Message("TTL::onPointEditComplete()\n");
    m_inProgressLock = false;
    
    m_pbTrackerState = TRACKEREDIT;
    ui->pbTracker->setText(QString::fromUtf8("Edit points"));
    ui->pbTracker->setEnabled(true);
    ui->pbCancelEdit->setEnabled(true);
    enableTaskButtons(true);
}

void TaskLeaderLine::abandonEditSession(void)
{
//    Base::Console().Message("TTL::abandonEditSession()\n");
    if (m_qgLine != nullptr) {
        m_qgLine->abandonEdit();
    }
    QString msg = tr("In progress edit abandoned. Start over.");
    getMainWindow()->statusBar()->show();
    Gui::getMainWindow()->showMessage(msg,4000);

    m_pbTrackerState = TRACKEREDIT;
    ui->pbTracker->setText(QString::fromUtf8("Edit points"));
    enableTaskButtons(true);
    ui->pbTracker->setEnabled(true);
    ui->pbCancelEdit->setEnabled(false);

    setEditCursor(Qt::ArrowCursor);
}

void TaskLeaderLine::saveButtons(QPushButton* btnOK,
                             QPushButton* btnCancel)
{
    m_btnOK = btnOK;
    m_btnCancel = btnCancel;
}

void TaskLeaderLine::enableTaskButtons(bool b)
{
    m_btnOK->setEnabled(b);
    m_btnCancel->setEnabled(b);
}

int TaskLeaderLine::getPrefArrowStyle()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter().
                                         GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/Dimensions");
    int style = hGrp->GetInt("ArrowStyle", 0);
    return style;
}

//******************************************************************************

bool TaskLeaderLine::accept()
{
//    Base::Console().Message("TTL::accept()\n");
    if (m_inProgressLock) {
        //accept() button shouldn't be available if there is an edit in progress.
        abandonEditSession();
        removeTracker();
        return true;
    }

    Gui::Document* doc = Gui::Application::Instance->getDocument(m_basePage->getDocument());
    if (!doc) return false;

    if (!getCreateMode())  {
//        removeTracker();
        updateLeaderFeature();
    } else {
//        removeTracker();
        createLeaderFeature(m_trackerPoints);
    }
    m_mdi->setContextMenuPolicy(m_saveContextPolicy);
    m_trackerMode = QGTracker::TrackerMode::None;
    removeTracker();

    Gui::Command::doCommand(Gui::Command::Gui,"Gui.ActiveDocument.resetEdit()");

    return true;
}

bool TaskLeaderLine::reject()
{
    if (m_inProgressLock) {
//        Base::Console().Message("TTL::reject - edit in progress!!\n");
        //reject() button shouldn't be available if there is an edit in progress.
        abandonEditSession();
        removeTracker();
        return false;
    }
    
    Gui::Document* doc = Gui::Application::Instance->getDocument(m_basePage->getDocument());
    if (!doc) return false;

    if (m_mdi != nullptr) {
        m_mdi->setContextMenuPolicy(m_saveContextPolicy);
    }
    if (getCreateMode() &&
        (m_lineFeat != nullptr) )  {
        removeFeature();
    }

    if (!getCreateMode() &&
        (m_lineFeat != nullptr) )  {
        restoreState();
    }

    m_trackerMode = QGTracker::TrackerMode::None;
    removeTracker();

    //make sure any dangling objects are cleaned up 
    Gui::Command::doCommand(Gui::Command::Gui,"App.activeDocument().recompute()");
    Gui::Command::doCommand(Gui::Command::Gui,"Gui.ActiveDocument.resetEdit()");

    return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TaskDlgLeaderLine::TaskDlgLeaderLine(TechDraw::DrawView* baseFeat,
                                     TechDraw::DrawPage* page)
    : TaskDialog()
{
    widget  = new TaskLeaderLine(baseFeat,page);
    taskbox = new Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("actions/techdraw-mline"),
                                             widget->windowTitle(), true, 0);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

TaskDlgLeaderLine::TaskDlgLeaderLine(TechDrawGui::ViewProviderLeader* leadVP)
    : TaskDialog()
{
    widget  = new TaskLeaderLine(leadVP);
    taskbox = new Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("actions/techdraw-mline"),
                                             widget->windowTitle(), true, 0);
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
