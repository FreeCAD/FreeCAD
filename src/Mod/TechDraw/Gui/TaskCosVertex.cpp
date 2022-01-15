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

#include "PreCompiled.h"

#ifndef _PreComp_
#include <cmath>
#include <QStatusBar>
#include <QGraphicsScene>
#endif // #ifndef _PreComp_

#include <Base/Console.h>
#include <Base/Tools.h>
#include <Base/Quantity.h>
#include <Base/UnitsApi.h>

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
#include <Mod/TechDraw/App/DrawViewPart.h>
#include <Mod/TechDraw/App/DrawProjGroup.h>
#include <Mod/TechDraw/App/DrawProjGroupItem.h>
#include <Mod/TechDraw/App/Cosmetic.h>

#include <Mod/TechDraw/Gui/ui_TaskCosVertex.h>

#include "DrawGuiStd.h"
#include "QGVPage.h"
#include "QGIView.h"
#include "QGIPrimPath.h"
#include "MDIViewPage.h"
#include "ViewProviderPage.h"
#include "QGTracker.h"
#include "QGEPath.h"
#include "Rez.h"

#include "TaskCosVertex.h"

using namespace Gui;
using namespace TechDraw;
using namespace TechDrawGui;

TaskCosVertex::TaskCosVertex(TechDraw::DrawViewPart* baseFeat,
                               TechDraw::DrawPage* page) :
    ui(new Ui_TaskCosVertex),
    blockUpdate(false),
    m_tracker(nullptr),
    m_mdi(nullptr),
    m_scene(nullptr),
    m_view(nullptr),
    m_baseFeat(baseFeat),
    m_basePage(page),
    m_qgParent(nullptr),
    m_trackerMode(QGTracker::None),
    m_saveContextPolicy(Qt::DefaultContextMenu),
    m_inProgressLock(false),
    m_btnOK(nullptr),
    m_btnCancel(nullptr),
    m_pbTrackerState(TRACKERPICK),
    m_savePoint(QPointF(0.0, 0.0))
{
    //baseFeat and page existence checked in cosmetic vertex command (CommandAnnotate.cpp)

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

    m_trackerMode = QGTracker::TrackerMode::Point;
}

TaskCosVertex::~TaskCosVertex()
{
}

void TaskCosVertex::updateTask()
{
    //    blockUpdate = true;

    //    blockUpdate = false;
}

void TaskCosVertex::changeEvent(QEvent* e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
}

void TaskCosVertex::setUiPrimary()
{
//    Base::Console().Message("TCV::setUiPrimary()\n");
    setWindowTitle(QObject::tr("New Cosmetic Vertex"));

    if (m_baseFeat != nullptr) {
        std::string baseName = m_baseFeat->getNameInDocument();
        ui->leBaseView->setText(Base::Tools::fromStdString(baseName));
    }
    ui->pbTracker->setText(QString::fromUtf8("Point Picker"));
    ui->pbTracker->setEnabled(true);
    ui->dsbX->setEnabled(true);
    ui->dsbY->setEnabled(true);
    int decimals = Base::UnitsApi::getDecimals();
    ui->dsbX->setDecimals(decimals);
    ui->dsbY->setDecimals(decimals);
    ui->dsbX->setUnit(Base::Unit::Length);
    ui->dsbY->setUnit(Base::Unit::Length);
}

void TaskCosVertex::updateUi(void)
{
    double x = m_savePoint.x();
    double y = - m_savePoint.y();
    ui->dsbX->setValue(x);
    ui->dsbY->setValue(y);
}

void TaskCosVertex::addCosVertex(QPointF qPos)
{
    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Add Cosmetic Vertex"));

//    Base::Console().Message("TCV::addCosVertex(%s)\n", TechDraw::DrawUtil::formatVector(qPos).c_str());
    Base::Vector3d pos(qPos.x(), -qPos.y());
//    int idx =
    (void) m_baseFeat->addCosmeticVertex(pos);
    m_baseFeat->requestPaint();

    Gui::Command::commitCommand();
}


//********** Tracker routines *******************************************************************
void TaskCosVertex::onTrackerClicked(bool b)
{
    Q_UNUSED(b);
//    Base::Console().Message("TCV::onTrackerClicked() m_pbTrackerState: %d\n",
//                            m_pbTrackerState);

    removeTracker();

    if (m_pbTrackerState == TRACKERCANCEL) {
        m_pbTrackerState = TRACKERPICK;
        ui->pbTracker->setText(QString::fromUtf8("Pick Points"));
        enableTaskButtons(true);

        setEditCursor(Qt::ArrowCursor);
        return;
    }

    m_inProgressLock = true;
    m_saveContextPolicy = m_mdi->contextMenuPolicy();
    m_mdi->setContextMenuPolicy(Qt::PreventContextMenu);
    m_trackerMode = QGTracker::TrackerMode::Point;
    setEditCursor(Qt::CrossCursor);
    startTracker();

    QString msg = tr("Pick a point for cosmetic vertex");
    getMainWindow()->statusBar()->show();
    Gui::getMainWindow()->showMessage(msg, 3000);
    ui->pbTracker->setText(QString::fromUtf8("Escape picking"));
    ui->pbTracker->setEnabled(true);
    m_pbTrackerState = TRACKERCANCEL;
    enableTaskButtons(false);
}

void TaskCosVertex::startTracker(void)
{
//    Base::Console().Message("TCV::startTracker()\n");
    if (m_trackerMode == QGTracker::TrackerMode::None) {
        return;
    }

    if (m_tracker == nullptr) {
        m_tracker = new QGTracker(m_scene, m_trackerMode);
        QObject::connect(
            m_tracker, SIGNAL(drawingFinished(std::vector<QPointF>, QGIView*)),
            this, SLOT(onTrackerFinished(std::vector<QPointF>, QGIView*))
        );
    }
    else {
        //this is too harsh. but need to avoid restarting process
        throw Base::RuntimeError("TechDrawNewLeader - tracker already active\n");
    }
    setEditCursor(Qt::CrossCursor);
    QString msg = tr("Left click to set a point");
    Gui::getMainWindow()->statusBar()->show();
    Gui::getMainWindow()->showMessage(msg, 3000);
}

void TaskCosVertex::onTrackerFinished(std::vector<QPointF> pts, QGIView* qgParent)
{
    //    Base::Console().Message("TCV::onTrackerFinished()\n");
    (void)qgParent;
    if (pts.empty()) {
        Base::Console().Error("TaskCosVertex - no points available\n");
        return;
    }

    QPointF dragEnd = pts.front();            //scene pos of mouse click

    double scale = m_baseFeat->getScale();
    double x = Rez::guiX(m_baseFeat->X.getValue());
    double y = Rez::guiX(m_baseFeat->Y.getValue());

    DrawViewPart* dvp = m_baseFeat;
    DrawProjGroupItem* dpgi = dynamic_cast<DrawProjGroupItem*>(dvp);
    if (dpgi != nullptr) {
        DrawProjGroup* dpg = dpgi->getPGroup();
        if (dpg == nullptr) {
            Base::Console().Message("TCV:onTrackerFinished - projection group is confused\n");
            //TODO::throw something.
            return;
        }
        x += Rez::guiX(dpg->X.getValue());
        y += Rez::guiX(dpg->Y.getValue());
    }
    //x,y are scene pos of dvp/dpgi

    QPointF basePosScene(x, -y);                 //base position in scene coords
    QPointF displace = dragEnd - basePosScene;
    QPointF scenePosCV = displace / scale;

    m_savePoint = Rez::appX(scenePosCV);
    updateUi();

    m_tracker->sleep(true);
    m_inProgressLock = false;
    m_pbTrackerState = TRACKERPICK;
    ui->pbTracker->setText(QString::fromUtf8("Pick Points"));
    ui->pbTracker->setEnabled(true);
    enableTaskButtons(true);
    setEditCursor(Qt::ArrowCursor);
    m_mdi->setContextMenuPolicy(m_saveContextPolicy);

}

void TaskCosVertex::removeTracker(void)
{
//    Base::Console().Message("TCV::removeTracker()\n");
    if ((m_tracker != nullptr) &&
        (m_tracker->scene() != nullptr)) {
        m_scene->removeItem(m_tracker);
        delete m_tracker;
        m_tracker = nullptr;
    }
}

void TaskCosVertex::setEditCursor(QCursor c)
{
    if (m_baseFeat != nullptr) {
        QGIView* qgivBase = m_view->findQViewForDocObj(m_baseFeat);
        qgivBase->setCursor(c);
    }
}

void TaskCosVertex::abandonEditSession(void)
{
    QString msg = tr("In progress edit abandoned. Start over.");
    getMainWindow()->statusBar()->show();
    Gui::getMainWindow()->showMessage(msg, 4000);

    ui->pbTracker->setEnabled(true);

    setEditCursor(Qt::ArrowCursor);
}

void TaskCosVertex::saveButtons(QPushButton* btnOK,
                             QPushButton* btnCancel)
{
    m_btnOK = btnOK;
    m_btnCancel = btnCancel;
}

void TaskCosVertex::enableTaskButtons(bool b)
{
    m_btnOK->setEnabled(b);
    m_btnCancel->setEnabled(b);
}

//******************************************************************************
bool TaskCosVertex::accept()
{
    Gui::Document* doc = Gui::Application::Instance->getDocument(m_basePage->getDocument());
    if (!doc) return false;

    removeTracker();
    double x = ui->dsbX->value().getValue();
    double y = ui->dsbY->value().getValue();
    QPointF uiPoint(x, -y);
    addCosVertex(uiPoint);

    m_baseFeat->recomputeFeature();
    m_baseFeat->requestPaint();
    m_mdi->setContextMenuPolicy(m_saveContextPolicy);
    m_trackerMode = QGTracker::TrackerMode::None;
    Gui::Command::doCommand(Gui::Command::Gui, "Gui.ActiveDocument.resetEdit()");

    return true;
}

bool TaskCosVertex::reject()
{
    Gui::Document* doc = Gui::Application::Instance->getDocument(m_basePage->getDocument());
    if (!doc) return false;

    removeTracker();
    m_trackerMode = QGTracker::TrackerMode::None;
    if (m_mdi != nullptr) {
        m_mdi->setContextMenuPolicy(m_saveContextPolicy);
    }

    //make sure any dangling objects are cleaned up
    Gui::Command::doCommand(Gui::Command::Gui, "App.activeDocument().recompute()");
    Gui::Command::doCommand(Gui::Command::Gui, "Gui.ActiveDocument.resetEdit()");

    return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TaskDlgCosVertex::TaskDlgCosVertex(TechDraw::DrawViewPart* baseFeat,
                                     TechDraw::DrawPage* page)
    : TaskDialog()
{
    widget  = new TaskCosVertex(baseFeat, page);
    taskbox = new Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("actions/techdraw-LeaderLine"),
                                             widget->windowTitle(), true, 0);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

TaskDlgCosVertex::~TaskDlgCosVertex()
{
}

void TaskDlgCosVertex::update()
{
//    widget->updateTask();
}

void TaskDlgCosVertex::modifyStandardButtons(QDialogButtonBox* box)
{
    QPushButton* btnOK = box->button(QDialogButtonBox::Ok);
    QPushButton* btnCancel = box->button(QDialogButtonBox::Cancel);
    widget->saveButtons(btnOK, btnCancel);
}

//==== calls from the TaskView ===============================================================
void TaskDlgCosVertex::open()
{
}

void TaskDlgCosVertex::clicked(int)
{
}

bool TaskDlgCosVertex::accept()
{
    widget->accept();
    return true;
}

bool TaskDlgCosVertex::reject()
{
    widget->reject();
    return true;
}

#include <Mod/TechDraw/Gui/moc_TaskCosVertex.cpp>
