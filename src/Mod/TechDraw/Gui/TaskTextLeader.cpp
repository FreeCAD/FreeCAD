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
#include <Mod/TechDraw/App/DrawTextLeader.h>

#include <Mod/TechDraw/Gui/ui_TaskTextLeader.h>

#include "DrawGuiStd.h"
#include "QGVPage.h"
#include "QGIView.h"
#include "QGIPrimPath.h"
#include "MDIViewPage.h"
#include "ViewProviderPage.h"
#include "ViewProviderLeader.h"
#include "QGTracker.h"
#include "QGEPath.h"
#include "QGMText.h"
#include "QGITextLeader.h"
#include "Rez.h"
#include "mrichtextedit.h"
#include "mtextedit.h"

#include "TaskTextLeader.h"

using namespace Gui;
using namespace TechDraw;
using namespace TechDrawGui;

//ctor for edit
TaskTextLeader::TaskTextLeader(int mode,
                               TechDrawGui::ViewProviderLeader* leadVP) :
    ui(new Ui_TaskTextLeader),
    m_tracker(nullptr),
    m_lineVP(leadVP),
    m_textVP(nullptr),
    m_baseFeat(nullptr),
    m_basePage(nullptr),
    m_lineFeat(nullptr),
    m_createMode(false),
    m_leadLine(nullptr),
    m_inProgressLock(false),
    m_qgLine(nullptr),
    m_qgText(nullptr),
    m_textDialog(nullptr),
    m_rte(nullptr),
    m_mode(mode),
    m_pbTrackerState(TRACKEREDIT)
{
    if (m_lineVP == nullptr)  {
        //should be caught in CMD caller
        Base::Console().Error("TaskTextLeader - bad parameters.  Can not proceed.\n");
        return;
    }
    ui->setupUi(this);
    
    m_lineFeat = m_lineVP->getFeature();
    if (m_mode == TEXTMODE) {
        m_textFeat = static_cast<DrawTextLeader*>(m_lineFeat);
        m_textVP = static_cast<ViewProviderTextLeader*>(m_lineVP);
    }

    App::DocumentObject* obj = m_lineFeat->LeaderParent.getValue();
    if ( obj->isDerivedFrom(TechDraw::DrawView::getClassTypeId()) )  {
        m_baseFeat = static_cast<TechDraw::DrawView*>(m_lineFeat->LeaderParent.getValue());
    }
    m_basePage = m_lineFeat->findParentPage();

    if ( (m_lineFeat == nullptr) ||
         (m_baseFeat == nullptr) ||
         (m_basePage == nullptr) ) {
        Base::Console().Error("TaskTextLeader - bad parameters (2).  Can not proceed.\n");
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
    if (m_mode == TEXTMODE) {
        connect(ui->pbEditor, SIGNAL(clicked(bool)),
                this, SLOT(onEditorClicked(bool)));
    }
    
    m_trackerMode = QGTracker::TrackerMode::Line;
}

//ctor for creation
TaskTextLeader::TaskTextLeader(int mode,
                               TechDraw::DrawView* baseFeat,
                               TechDraw::DrawPage* page) :
    ui(new Ui_TaskTextLeader),
    m_tracker(nullptr),
    m_lineVP(nullptr),
    m_baseFeat(baseFeat),
    m_basePage(page),
    m_lineFeat(nullptr),
    m_createMode(true),
    m_leadLine(nullptr),
    m_inProgressLock(false),
    m_qgLine(nullptr),
    m_qgText(nullptr),
    m_textDialog(nullptr),
    m_rte(nullptr),
    m_mode(mode),
    m_pbTrackerState(TRACKERPICK)
{
    if ( (m_basePage == nullptr) ||
         (m_baseFeat == nullptr) )  {
        //should be caught in CMD caller
        Base::Console().Error("TaskTextLeader - bad parameters.  Can not proceed.\n");
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
    if (mode == TEXTMODE) {
        connect(ui->pbEditor, SIGNAL(clicked(bool)),
                this, SLOT(onEditorClicked(bool)));
    }

    m_trackerMode = QGTracker::TrackerMode::Line;
}

TaskTextLeader::~TaskTextLeader()
{
    delete ui;
}

void TaskTextLeader::updateTask()
{
//    blockUpdate = true;

//    blockUpdate = false;
}

void TaskTextLeader::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
}

void TaskTextLeader::setUiPrimary()
{
//    Base::Console().Message("TTL::setUiPrimary() - m_mode: %d\n",m_mode);
    enableVPUi(false);
    if (m_mode == TEXTMODE) {
        setWindowTitle(QObject::tr("New Text Leader"));
        enableTextUi(true);
    } else {
        setWindowTitle(QObject::tr("New Leader Line"));
        enableTextUi(false);
    }

    if (m_baseFeat != nullptr) {
        std::string baseName = m_baseFeat->getNameInDocument();
        ui->leBaseView->setText(Base::Tools::fromStdString(baseName));
    }
    ui->pbTracker->setText(QString::fromUtf8("Pick points"));
    ui->pbTracker->setEnabled(true);
    int aSize = getPrefArrowStyle() + 1;
    ui->cboxStartSym->setCurrentIndex(aSize);
}

void TaskTextLeader::enableTextUi(bool b) 
{
    ui->pbEditor->setEnabled(b);
    ui->teLeaderText->setEnabled(b);
}

void TaskTextLeader::enableVPUi(bool b)
{
    ui->cpLineColor->setEnabled(b);
    ui->dsbWeight->setEnabled(b);
    ui->cboxStyle->setEnabled(b);
}

void TaskTextLeader::setUiEdit()
{
//    Base::Console().Message("TTL::setUiEdit() - m_mode: %d\n",m_mode);
    enableVPUi(true);
    if (m_mode == TEXTMODE) {
        setWindowTitle(QObject::tr("Edit Text Leader"));
        enableTextUi(true);
    } else {
        setWindowTitle(QObject::tr("Edit Leader Line"));
        enableTextUi(false);
    }
    
    if (m_lineFeat != nullptr) {
        std::string baseName = m_lineFeat->LeaderParent.getValue()->getNameInDocument();
        ui->leBaseView->setText(Base::Tools::fromStdString(baseName));
        ui->cboxStartSym->setCurrentIndex(m_lineFeat->StartSymbol.getValue() + 1);
        ui->cboxEndSym->setCurrentIndex(m_lineFeat->EndSymbol.getValue() + 1);
        ui->pbTracker->setText(QString::fromUtf8("Edit points"));
        ui->pbTracker->setEnabled(true);
    }
    
    if (m_lineVP != nullptr) {
        ui->cpLineColor->setColor(m_lineVP->Color.getValue().asValue<QColor>());
        ui->dsbWeight->setValue(m_lineVP->LineWidth.getValue());
        ui->cboxStyle->setCurrentIndex(m_lineVP->LineStyle.getValue());
    }
    
    if (m_textFeat != nullptr) {
        ui->teLeaderText->setHtml(QString::fromUtf8(m_textFeat->LeaderText.getValue()));
    }
}

void TaskTextLeader::onEditorClicked(bool b)
{
//    Base::Console().Message("TL::onEditorClicked(%d)\n",b);
    Q_UNUSED(b);
    m_textDialog = new QDialog(0);
    QString leadText = ui->teLeaderText->toHtml();
    m_rte = new MRichTextEdit(m_textDialog, leadText);
    //m_rte->setTextWidth(m_lineVP->MaxWidth);
    QGridLayout* gl = new QGridLayout(m_textDialog);
    gl->addWidget(m_rte,0,0,1,1);
    m_textDialog->setWindowTitle(QObject::tr("Leader text editor"));
    m_textDialog->setMinimumWidth (400);
    m_textDialog->setMinimumHeight(400);

    connect(m_rte, SIGNAL(saveText(QString)),
            this, SLOT(onSaveAndExit(QString)));
    connect(m_rte, SIGNAL(editorFinished(void)),
            this, SLOT(onEditorExit(void)));

    m_textDialog->show();
}

void TaskTextLeader::onSaveAndExit(QString qs)
{
    ui->teLeaderText->setHtml(qs);
    //dialog clean up should be handled by accept() call in dialog
    m_textDialog->accept();
    m_textDialog = nullptr;
    m_rte = nullptr;
}

void TaskTextLeader::onEditorExit(void)
{
    m_textDialog->reject();
    m_textDialog = nullptr;
    m_rte = nullptr;
}

//******************************************************************************
void TaskTextLeader::createLeaderFeature(std::vector<Base::Vector3d> converted)
{
    Base::Console().Message("TTL::createLeaderFeature() - m_mode: %d\n",m_mode);
    if (m_mode == TEXTMODE) {
        m_leaderName = m_basePage->getDocument()->getUniqueObjectName("DrawTextLeader");
        m_leaderType = "TechDraw::DrawTextLeader";
    } else {
        m_leaderName = m_basePage->getDocument()->getUniqueObjectName("DrawLeaderLine");
        m_leaderType = "TechDraw::DrawLeaderLine";
    }

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
        throw Base::RuntimeError("TaskTextLeader - new markup object not found");
    }
    if (obj->isDerivedFrom(TechDraw::DrawTextLeader::getClassTypeId())) {
        m_lineFeat = static_cast<TechDraw::DrawLeaderLine*>(obj);
        m_textFeat = static_cast<TechDraw::DrawTextLeader*>(obj);
        commonFeatureUpdate(converted);
        QPointF qTemp = calcTextStartPos(m_lineFeat->getScale());
        Base::Vector3d vTemp(qTemp.x(), qTemp.y());
        m_textFeat->TextPosition.setValue(Rez::appX(vTemp));
    } else if (obj->isDerivedFrom(TechDraw::DrawLeaderLine::getClassTypeId())) {
        m_lineFeat = static_cast<TechDraw::DrawLeaderLine*>(obj);
        m_textFeat = nullptr;
        commonFeatureUpdate(converted);
    }
    
    Gui::Command::updateActive();
    Gui::Command::commitCommand();
    m_lineFeat->requestPaint();
}

void TaskTextLeader::updateLeaderFeature(std::vector<Base::Vector3d> converted)
{
//    Base::Console().Message("TTL::updateLeaderFeature(%d) - m_mode: %d\n",converted.size(),m_mode);
    Gui::Command::openCommand("Edit Leader");
    commonFeatureUpdate(converted);
    App::Color ac;
    ac.setValue<QColor>(ui->cpLineColor->color());
    m_lineVP->Color.setValue(ac);
    m_lineVP->LineWidth.setValue(ui->dsbWeight->value());
    m_lineVP->LineStyle.setValue(ui->cboxStyle->currentIndex());

    Gui::Command::commitCommand();
    m_lineFeat->requestPaint();
}

void TaskTextLeader::commonFeatureUpdate(std::vector<Base::Vector3d> converted)
{
//    Base::Console().Message("TTL::commonFeatureUpdate()\n");
    m_lineFeat->setPosition(Rez::appX(m_attachPoint.x),Rez::appX(- m_attachPoint.y), true);
    if (!converted.empty()) {
        m_lineFeat->WayPoints.setValues(converted);
    }
    int start = ui->cboxStartSym->currentIndex() - 1;
    int end   = ui->cboxEndSym->currentIndex() - 1;
    m_lineFeat->StartSymbol.setValue(start);
    m_lineFeat->EndSymbol.setValue(end);
    if (m_mode == TEXTMODE) {
        m_textFeat->LeaderText.setValue(ui->teLeaderText->toHtml().toUtf8()); 
    }
}

void TaskTextLeader::removeFeature(void)
{
//    Base::Console().Message("TTL::removeFeature()\n");
    if (m_lineFeat != nullptr) {
        if (m_createMode) {
            try {
                // this doesn't remove the QGMText item??
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
                Base::Console().Log("TaskTextLeader: Edit mode - NO command is active\n");
            }
        }
    }
}

//we don't know the bounding rect of the text, so we have to calculate a reasonable
//guess at the size of the text block.
QPointF TaskTextLeader::calcTextStartPos(double scale)
{
//    Base::Console().Message("TTL::calcTextStartPos(%.3f) - m_mode: %d\n", scale, m_mode);
    double textWidth = 100.0;
    double textHeight = 20.0;
    double horizGap(20.0);
    double tPosX(0.0);
    double tPosY(0.0);

    Gui::ViewProvider* vp = QGIView::getViewProvider(m_textFeat);
    if (vp != nullptr) {
        ViewProviderTextLeader* vpl = dynamic_cast<ViewProviderTextLeader*>(vp);
        if (vpl != nullptr) {
            double adjust = 2.0;
            double fsize = vpl->Fontsize.getValue();
            textHeight = fsize * adjust;
            double width = vpl->MaxWidth.getValue();
            if (width > 0 ) {
                textWidth = width;
            }
        }
    }

    if (!m_trackerPoints.empty() ) {
        QPointF lastPoint(m_trackerPoints.back().x, m_trackerPoints.back().y);
        QPointF firstPoint(m_trackerPoints.front().x, m_trackerPoints.front().y);
        QPointF lastOffset = lastPoint;
        lastPoint = m_qgParent->mapFromScene(lastPoint) * scale;
        firstPoint = m_qgParent->mapFromScene(firstPoint) * scale;

        if (lastPoint.x() < firstPoint.x()) {                 //last is left of first
            tPosX = lastOffset.x() - horizGap - textWidth;    //left of last
            tPosY = lastOffset.y() - textHeight;
        } else {                                             //last is right of first
            tPosX = lastOffset.x() + horizGap;               //right of last
            tPosY = lastOffset.y() - textHeight;
        }
    }
    QPointF result(tPosX, -tPosY);
    return result;
}

//********** Tracker routines *******************************************************************
void TaskTextLeader::onTrackerClicked(bool b)
{
    Q_UNUSED(b);
//    Base::Console().Message("TTL::onTrackerClicked() - m_mode: %d, m_pbTrackerState: %d\n",
//                            m_mode, m_pbTrackerState);
    if (m_pbTrackerState == TRACKERCANCEL) {
        removeTracker();

        m_pbTrackerState = TRACKERPICK;
        ui->pbTracker->setText(QString::fromUtf8("Pick Points"));
        enableTaskButtons(true);

        setEditCursor(Qt::ArrowCursor);
        return;
    } else if (m_pbTrackerState == TRACKERCANCELEDIT) {
        abandonEditSession();

        m_pbTrackerState = TRACKEREDIT;
        ui->pbTracker->setText(QString::fromUtf8("Edit points"));
        enableTaskButtons(true);

        setEditCursor(Qt::ArrowCursor);
        return;
    }
    
    if (getCreateMode()) {
        m_inProgressLock = true;
        m_saveContextPolicy = m_mdi->contextMenuPolicy();
        m_mdi->setContextMenuPolicy(Qt::PreventContextMenu);
        m_trackerMode = QGTracker::TrackerMode::Line;
        startTracker();

        QString msg = tr("Pick a starting point for leader line");
        getMainWindow()->statusBar()->show();
        Gui::getMainWindow()->showMessage(msg,3000);
        ui->pbTracker->setText(QString::fromUtf8("Escape picking"));
        ui->pbTracker->setEnabled(true);
        m_pbTrackerState = TRACKERCANCEL;
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
                Base::Console().Error("TaskTextLeader - can't find leader graphic\n");
                //now what? throw will generate "unknown unhandled exception"
            } else {
                m_qgLine = qgLead;
                connect(qgLead, SIGNAL(editComplete(std::vector<QPointF>, QGIView*)),
                this, SLOT(onPointEditComplete(std::vector<QPointF>, QGIView*)));

                m_attachPoint = Rez::guiX(Base::Vector3d(m_lineFeat->X.getValue(),  //don't need this?
                                               -m_lineFeat->Y.getValue(),
                                               0.0));
                qgLead->startPathEdit();
                QString msg = tr("Click and drag markers to adjust leader line");
                getMainWindow()->statusBar()->show();
                Gui::getMainWindow()->showMessage(msg,3000);
                msg = tr("ESC or RMB to exit");
                getMainWindow()->statusBar()->show();
                Gui::getMainWindow()->showMessage(msg,3000);
                ui->pbTracker->setText(QString::fromUtf8("Escape edit"));
                ui->pbTracker->setEnabled(true);
                m_pbTrackerState = TRACKERCANCELEDIT;
                enableTaskButtons(false);
            }
        } else { // need to recreate leaderline
            // same as create mode??
            m_inProgressLock = true;
            m_saveContextPolicy = m_mdi->contextMenuPolicy();
            m_mdi->setContextMenuPolicy(Qt::PreventContextMenu);
            m_trackerMode = QGTracker::TrackerMode::Line;
            startTracker();

            QString msg = tr("Pick a starting point for leader line");
            getMainWindow()->statusBar()->show();
            Gui::getMainWindow()->showMessage(msg,3000);
            ui->pbTracker->setText(QString::fromUtf8("Escape picking"));
            ui->pbTracker->setEnabled(true);
            m_pbTrackerState = TRACKERCANCEL;
            enableTaskButtons(false);
        }
    } //end edit mode
}

void TaskTextLeader::startTracker(void)
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

void TaskTextLeader::onTrackerFinished(std::vector<QPointF> pts, QGIView* qgParent)
{
//    Base::Console().Message("TTL::onTrackerFinished()\n");
    if (pts.empty()) {
        Base::Console().Error("TaskTextLeader - no points available\n");
        return;
    }
    QGIView* qgiv = dynamic_cast<QGIView*>(qgParent);
    if (qgiv != nullptr) {
        m_qgParent = qgiv;
    }
    if (m_qgParent != nullptr) {
        double scale = m_qgParent->getScale();
        QPointF mapped = m_qgParent->mapFromScene(pts.front()) / scale;
        m_attachPoint = Base::Vector3d(mapped.x(), mapped.y(), 0.0);
        convertTrackerPoints(pts);
    }

    QString msg = tr("Press OK or Cancel to continue");
    getMainWindow()->statusBar()->show();
    Gui::getMainWindow()->showMessage(msg, 3000);
    enableTaskButtons(true);

    m_tracker->sleep(true);
    m_inProgressLock = false;
    ui->pbTracker->setEnabled(false);
    enableTaskButtons(true);
    setEditCursor(Qt::ArrowCursor);
}

void TaskTextLeader::removeTracker(void)
{
//    Base::Console().Message("TTL::removeTracker()\n");
    if ( (m_tracker != nullptr) &&
         (m_tracker->scene() != nullptr) ) {
        m_scene->removeItem(m_tracker);
        delete m_tracker;
        m_tracker = nullptr;
    }
}

void TaskTextLeader::setEditCursor(QCursor c)
{
    if (m_baseFeat != nullptr) {
        QGIView* qgivBase = m_view->findQViewForDocObj(m_baseFeat);
        qgivBase->setCursor(c);
    }
}

//from 1:1 scale scene QPointF to zero origin Vector3d points
void TaskTextLeader::convertTrackerPoints(std::vector<QPointF> pts)
{
//    Base::Console().Message("TTL::convertTrackerPoints(%d)\n", pts.size());
    m_trackerPoints.clear();
    for (auto& p: pts) {
        QPointF mapped = p - pts.front();
        Base::Vector3d convert(mapped.x(),mapped.y(),0.0);
        m_trackerPoints.push_back(convert);
    }
}

//******************************************************************************
void TaskTextLeader::onPointEditComplete(std::vector<QPointF> pts, QGIView* parent)
{
//    Base::Console().Message("TTL::onPointEditComplete(%d)\n", pts.size());
    if (pts.empty()) {
        return;
    }
    QPointF p0 = pts.front();

    if (parent == nullptr) {
//        Base::Console().Message("TTL::onPointEditComplete - passed parent is NULL!\n");
    } else {
        m_qgParent = parent;
//        double scale = m_qgParent->getScale();
        if ( !(TechDraw::DrawUtil::fpCompare(p0.x(),0.0) &&
               TechDraw::DrawUtil::fpCompare(p0.y(),0.0))  )  {
            //p0 was moved. need to change AttachPoint and intervals from p0
            QPointF mapped = m_qgParent->mapFromItem(m_qgLine,p0);
            m_attachPoint = Base::Vector3d(mapped.x(),mapped.y(),0.0);
            for (auto& p : pts) {
                p -= p0;
            }
            pts.at(0) = QPointF(0.0,0.0);
        }

        convertTrackerPoints(pts);
    }
    m_inProgressLock = false;
    
    m_pbTrackerState = TRACKEREDIT;
    ui->pbTracker->setText(QString::fromUtf8("Edit points"));
    ui->pbTracker->setEnabled(true);
    enableTaskButtons(true);
}

void TaskTextLeader::abandonEditSession(void)
{
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

    setEditCursor(Qt::ArrowCursor);
}

void TaskTextLeader::saveButtons(QPushButton* btnOK,
                             QPushButton* btnCancel)
{
    m_btnOK = btnOK;
    m_btnCancel = btnCancel;
}

void TaskTextLeader::enableTaskButtons(bool b)
{
    m_btnOK->setEnabled(b);
    m_btnCancel->setEnabled(b);
}

int TaskTextLeader::getPrefArrowStyle()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter().
                                         GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/Dimensions");
    int style = hGrp->GetInt("ArrowStyle", 0);
    return style;
}


//******************************************************************************

bool TaskTextLeader::accept()
{
//    Base::Console().Message("TTL::accept() - m_mode: %d\n", m_mode);
    if (m_inProgressLock) {
//        Base::Console().Message("TTL::accept - edit in progress!!\n");
        abandonEditSession();
        return true;
    }

    Gui::Document* doc = Gui::Application::Instance->getDocument(m_basePage->getDocument());
    if (!doc) return false;

    if (!getCreateMode())  {
        updateLeaderFeature(m_trackerPoints);
    } else {
        removeTracker();
        createLeaderFeature(m_trackerPoints);
    }
    m_mdi->setContextMenuPolicy(m_saveContextPolicy);
    m_trackerMode = QGTracker::TrackerMode::None;
    Gui::Command::doCommand(Gui::Command::Gui,"Gui.ActiveDocument.resetEdit()");

    return true;
}

bool TaskTextLeader::reject()
{
    if (m_inProgressLock) {
//        Base::Console().Message("TTL::reject - edit in progress!!\n");
        abandonEditSession();
        removeTracker();
        return false;
    }
    
    Gui::Document* doc = Gui::Application::Instance->getDocument(m_basePage->getDocument());
    if (!doc) return false;

    m_mdi->setContextMenuPolicy(m_saveContextPolicy);
    if (getCreateMode() &&
        (m_lineFeat != nullptr) )  {
        removeFeature();
    }
    m_trackerMode = QGTracker::TrackerMode::None;

    //make sure any dangling objects are cleaned up 
    Gui::Command::doCommand(Gui::Command::Gui,"App.activeDocument().recompute()");
    Gui::Command::doCommand(Gui::Command::Gui,"Gui.ActiveDocument.resetEdit()");

    return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TaskDlgTextLeader::TaskDlgTextLeader(int mode,
                                     TechDraw::DrawView* baseFeat,
                                     TechDraw::DrawPage* page)
    : TaskDialog()
{
    widget  = new TaskTextLeader(mode,baseFeat,page);
    if (mode == TEXTMODE) {
        taskbox = new Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("actions/techdraw-textleader"),
                                              widget->windowTitle(), true, 0);
    } else { //if (mode == 1)
        taskbox = new Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("actions/techdraw-mline"),
                                             widget->windowTitle(), true, 0);
    }
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

TaskDlgTextLeader::TaskDlgTextLeader(int mode,
                                     TechDrawGui::ViewProviderLeader* leadVP)
    : TaskDialog()
{
    widget  = new TaskTextLeader(mode,leadVP);
    if (mode == TEXTMODE) {
        taskbox = new Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("actions/techdraw-textleader"),
                                             widget->windowTitle(), true, 0);
    } else { //if (mode == 1) 
        taskbox = new Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("actions/techdraw-mline"),
                                             widget->windowTitle(), true, 0);
    }
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

TaskDlgTextLeader::~TaskDlgTextLeader()
{
}

void TaskDlgTextLeader::update()
{
//    widget->updateTask();
}

void TaskDlgTextLeader::modifyStandardButtons(QDialogButtonBox* box)
{
    QPushButton* btnOK = box->button(QDialogButtonBox::Ok);
    QPushButton* btnCancel = box->button(QDialogButtonBox::Cancel);
    widget->saveButtons(btnOK, btnCancel);
}

//==== calls from the TaskView ===============================================================
void TaskDlgTextLeader::open()
{
}

void TaskDlgTextLeader::clicked(int)
{
}

bool TaskDlgTextLeader::accept()
{
    widget->accept();
    return true;
}

bool TaskDlgTextLeader::reject()
{
    widget->reject();
    return true;
}

#include <Mod/TechDraw/Gui/moc_TaskTextLeader.cpp>
