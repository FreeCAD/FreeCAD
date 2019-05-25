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
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>

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
#include <Mod/TechDraw/App/DrawViewPart.h>
#include <Mod/TechDraw/App/Geometry.h>
#include <Mod/TechDraw/App/Cosmetic.h>

#include <Mod/TechDraw/Gui/ui_TaskCenterLine.h>

#include "DrawGuiStd.h"
#include "QGVPage.h"
#include "QGIView.h"
#include "QGIPrimPath.h"
#include "MDIViewPage.h"
#include "ViewProviderPage.h"
#include "ViewProviderViewPart.h"
#include "Rez.h"

#include "TaskCenterLine.h"

using namespace Gui;
using namespace TechDraw;
using namespace TechDrawGui;

//ctor for edit
//TaskCenterLine::TaskCenterLine(TechDrawGui::ViewProviderViewPart* partVP) :
//    ui(new Ui_TaskCenterLine),
//    m_partVP(partVp),
//    m_partFeat(nullptr),
//    m_basePage(nullptr),
//    m_createMode(false),
//    m_inProgressLock(false)
//{
//    if (m_partVP == nullptr)  {
//        //should be caught in CMD caller
//        Base::Console().Error("TaskCenterLine - bad parameters.  Can not proceed.\n");
//        return;
//    }
//    ui->setupUi(this);
//    
//    m_partFeat = m_partVP->getFeature();
//    m_basePage = m_partFeat->findParentPage();

//    //TODO: when/if leaders are allowed to be parented to Page, check for m_partFeat will be removed
//    if ( (m_partFeat == nullptr) ||
//         (m_basePage == nullptr) ) {
//        Base::Console().Error("TaskCenterLine - bad parameters (2).  Can not proceed.\n");
//        return;
//    }

//    //m_subNames = m_partFeat->get?????();

//    setUiEdit();

////    m_mdi = m_partVP->getMDIViewPage();
////    m_scene = m_mdi->m_scene;
////    m_view = m_mdi->getQGVPage();

////    connect(ui->pbTracker, SIGNAL(clicked(bool)),
////            this, SLOT(onTrackerClicked(bool)));
////    connect(ui->pbCancelEdit, SIGNAL(clicked(bool)),
////            this, SLOT(onCancelEditClicked(bool)));
////    ui->pbCancelEdit->setEnabled(false);

//    saveState();

//}

//ctor for creation
TaskCenterLine::TaskCenterLine(TechDraw::DrawViewPart* partFeat,
                               TechDraw::DrawPage* page,
                               std::vector<std::string> subNames) :
    ui(new Ui_TaskCenterLine),
    m_partVP(nullptr),
    m_partFeat(partFeat),
    m_basePage(page),
    m_createMode(true),
    m_subNames(subNames)
{
    if ( (m_basePage == nullptr) ||
         (m_partFeat == nullptr) )  {
        //should be caught in CMD caller
        Base::Console().Error("TaskCenterLine - bad parameters.  Can not proceed.\n");
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
    
//    connect(ui->pbTracker, SIGNAL(clicked(bool)),
//            this, SLOT(onTrackerClicked(bool)));
//    connect(ui->pbCancelEdit, SIGNAL(clicked(bool)),
//            this, SLOT(onCancelEditClicked(bool)));
}

TaskCenterLine::~TaskCenterLine()
{
    delete ui;
}

void TaskCenterLine::saveState()
{
    if (m_partFeat != nullptr) {
    }
}

void TaskCenterLine::restoreState()
{
    if (m_partFeat != nullptr) {
    }
}

void TaskCenterLine::updateTask()
{
//    blockUpdate = true;

//    blockUpdate = false;
}

void TaskCenterLine::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
}

void TaskCenterLine::setUiPrimary()
{
//    Base::Console().Message("TCL::setUiPrimary()\n");
//    enableVPUi(false);
    setWindowTitle(QObject::tr("New Center Line"));

    if (m_partFeat != nullptr) {
        std::string baseName = m_partFeat->getNameInDocument();
        ui->leBaseView->setText(Base::Tools::fromStdString(baseName));
        for (auto& s: m_subNames) {
            QString listItem = Base::Tools::fromStdString(s);
            ui->lstSubList->addItem(listItem);
        }
    }
    ui->cpLineColor->setColor(getCenterColor());
    ui->dsbWeight->setValue(getCenterWidth());
    ui->cboxStyle->setCurrentIndex(getCenterStyle());
    ui->qsbExtend->setValue(getExtendBy());
}

//void TaskCenterLine::enableVPUi(bool b)

//{
//}

//void TaskCenterLine::setUiEdit()
//{
////    Base::Console().Message("TCL::setUiEdit()\n");
//    enableVPUi(true);
//    setWindowTitle(QObject::tr("Edit Center Line"));
//}

void TaskCenterLine::addCenterLine(void)
{
//    TechDraw::CosmeticEdge* ce = new TechDrawCosmeticEdge();
}


//******************************************************************************
void TaskCenterLine::createCenterLine(void)
{
//    Base::Console().Message("TCL::createCenterLine()\n");

    Gui::Command::openCommand("Create CenterLine");
    bool vertical = false;
    if (ui->rbVertical->isChecked()) {
        vertical = true;
    }
    m_extendBy = ui->qsbExtend->rawValue();
    //TODO: (if m_partFeat->getGeomTypeFromName(m_subNames.at(0)) == "Face") {
    TechDraw::CosmeticEdge* ce = makeMidLine(m_subNames.at(0),vertical,m_extendBy);
    m_partFeat->addRandomEdge(ce);
    m_partFeat->requestPaint();
    Gui::Command::updateActive();
    Gui::Command::commitCommand();
}

void TaskCenterLine::updateCenterLine(void)
{
//    Base::Console().Message("TCL::updateCenterLine()\n");
    Gui::Command::openCommand("Edit CenterLine");

    Gui::Command::updateActive();
    Gui::Command::commitCommand();
}

void TaskCenterLine::removeCenterLine(void)
{
//    Base::Console().Message("TCL::removeCenterLine()\n");
    if (m_partFeat != nullptr) {
        if (m_createMode) {
            //don't know!
        } else {
            if (Gui::Command::hasPendingCommand()) {
                std::vector<std::string> undos = Gui::Application::Instance->activeDocument()->getUndoVector();
                Gui::Application::Instance->activeDocument()->undo(1);
            } else {
                Base::Console().Log("TaskCenterLine: Edit mode - NO command is active\n");
            }
        }
    }
}

QGIView* TaskCenterLine::findParentQGIV()
{
    QGIView* result = nullptr;
    if (m_partFeat != nullptr) {
        Gui::ViewProvider* gvp = QGIView::getViewProvider(m_partFeat);
        ViewProviderDrawingView* vpdv = dynamic_cast<ViewProviderDrawingView*>(gvp);
        if (vpdv != nullptr) {
            result = vpdv->getQView();
        }
    }
    return result;
}

void TaskCenterLine::saveButtons(QPushButton* btnOK,
                             QPushButton* btnCancel)
{
    m_btnOK = btnOK;
    m_btnCancel = btnCancel;
}

void TaskCenterLine::enableTaskButtons(bool b)
{
    m_btnOK->setEnabled(b);
    m_btnCancel->setEnabled(b);
}

TechDraw::CosmeticEdge* TaskCenterLine::makeMidLine(std::string faceName, bool vert, double ext)
{
//    Base::Console().Message("TCL::makeMidLine(%s, %d) \n",faceName.c_str(), vert);
    TechDraw::CosmeticEdge* result = nullptr;
    double scale = m_partFeat->getScale();
    Base::Vector3d p1, p2;
    int idx = TechDraw::DrawUtil::getIndexFromName(faceName);
    std::vector<TechDrawGeometry::BaseGeom*> faceEdges = 
        m_partFeat->getFaceEdgesByIndex(idx);
    Bnd_Box faceBox;
    faceBox.SetGap(0.0);
    for (auto& fe: faceEdges) {
        if (!fe->cosmetic) {
            BRepBndLib::Add(fe->occEdge, faceBox);
        }
    }
    double Xmin,Ymin,Zmin,Xmax,Ymax,Zmax;
    faceBox.Get(Xmin,Ymin,Zmin,Xmax,Ymax,Zmax);

    double Xspan = fabs(Xmax - Xmin);
    Xspan = (Xspan / 2.0) + (ext * scale);
    double Xmid = Xmin + fabs(Xmax - Xmin) / 2.0;

    double Yspan = fabs(Ymax - Ymin);
    Yspan = (Yspan / 2.0) + (ext * scale);
    double Ymid = Ymin + fabs(Ymax - Ymin) / 2.0;

    Base::Vector3d bbxCenter(Xmid, Ymid, 0.0);

    if (vert) {
        Base::Vector3d top(Xmid, Ymid - Yspan, 0.0);
        Base::Vector3d bottom(Xmid, Ymid + Yspan, 0.0);
        p1 = top;
        p2 = bottom;
    } else {
        Base::Vector3d left(Xmid - Xspan, Ymid, 0.0);
        Base::Vector3d right(Xmid + Xspan, Ymid, 0.0);
        p1 = left;
        p2 = right;
    }

    result = new TechDraw::CosmeticEdge(p1,p2);
    App::Color ac;
    ac.setValue<QColor>(ui->cpLineColor->color());
    result->color = ac;
    result->width = ui->dsbWeight->value();
    result->style = ui->cboxStyle->currentIndex();
    result->visible = true;
    return result;
}

double TaskCenterLine::getCenterWidth()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter().GetGroup("BaseApp")->
                                                    GetGroup("Preferences")->GetGroup("Mod/TechDraw/Decorations");
    std::string lgName = hGrp->GetASCII("LineGroup","FC 0.70mm");
    auto lg = TechDraw::LineGroup::lineGroupFactory(lgName);

    double width = lg->getWeight("Graphic");
    delete lg; 
    return width;
}

Qt::PenStyle TaskCenterLine::getCenterStyle()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter().GetGroup("BaseApp")->
                                         GetGroup("Preferences")->GetGroup("Mod/TechDraw/Decorations");
    Qt::PenStyle centerStyle = static_cast<Qt::PenStyle> (hGrp->GetInt("CenterLine", 2));
    return centerStyle;
}

QColor TaskCenterLine::getCenterColor()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/Decorations");
    App::Color fcColor = App::Color((uint32_t) hGrp->GetUnsigned("CenterColor", 0x00000000));
    return fcColor.asValue<QColor>();
}

double TaskCenterLine::getExtendBy(void)
{
    return 3.0;
}

//******************************************************************************

bool TaskCenterLine::accept()
{
//    Base::Console().Message("TCL::accept()\n");

    Gui::Document* doc = Gui::Application::Instance->getDocument(m_basePage->getDocument());
    if (!doc) return false;

    if (!getCreateMode())  {
        //
    } else {
        createCenterLine();
    }
//    m_mdi->setContextMenuPolicy(m_saveContextPolicy);
    Gui::Command::doCommand(Gui::Command::Gui,"Gui.ActiveDocument.resetEdit()");

    return true;
}

bool TaskCenterLine::reject()
{
    if (m_inProgressLock) {
//        Base::Console().Message("TCL::reject - edit in progress!!\n");
        return false;
    }
    
    Gui::Document* doc = Gui::Application::Instance->getDocument(m_basePage->getDocument());
    if (!doc) return false;

    if (m_mdi != nullptr) {
//        m_mdi->setContextMenuPolicy(m_saveContextPolicy);
    }
    if (getCreateMode() &&
        (m_partFeat != nullptr) )  {
        //
    }

    if (!getCreateMode() &&
        (m_partFeat != nullptr) )  {
        //
    }

    //make sure any dangling objects are cleaned up 
    Gui::Command::doCommand(Gui::Command::Gui,"App.activeDocument().recompute()");
    Gui::Command::doCommand(Gui::Command::Gui,"Gui.ActiveDocument.resetEdit()");

    return false;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TaskDlgCenterLine::TaskDlgCenterLine(TechDraw::DrawViewPart* partFeat,
                                     TechDraw::DrawPage* page,
                                     std::vector<std::string> subNames)
    : TaskDialog()
{
    widget  = new TaskCenterLine(partFeat,page,subNames);
    taskbox = new Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("actions/techdraw-centerline"),
                                             widget->windowTitle(), true, 0);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

//TaskDlgCenterLine::TaskDlgCenterLine(TechDrawGui::ViewProviderViewPart* partVP)
//    : TaskDialog()
//{
//    widget  = new TaskCenterLine(partVP);
//    taskbox = new Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("actions/techdraw-centerline"),
//                                             widget->windowTitle(), true, 0);
//    taskbox->groupLayout()->addWidget(widget);
//    Content.push_back(taskbox);
//}

TaskDlgCenterLine::~TaskDlgCenterLine()
{
}

void TaskDlgCenterLine::update()
{
//    widget->updateTask();
}

void TaskDlgCenterLine::modifyStandardButtons(QDialogButtonBox* box)
{
    QPushButton* btnOK = box->button(QDialogButtonBox::Ok);
    QPushButton* btnCancel = box->button(QDialogButtonBox::Cancel);
    widget->saveButtons(btnOK, btnCancel);
}

//==== calls from the TaskView ===============================================================
void TaskDlgCenterLine::open()
{
}

void TaskDlgCenterLine::clicked(int)
{
}

bool TaskDlgCenterLine::accept()
{
    widget->accept();
    return true;
}

bool TaskDlgCenterLine::reject()
{
    widget->reject();
    return true;
}

#include <Mod/TechDraw/Gui/moc_TaskCenterLine.cpp>
