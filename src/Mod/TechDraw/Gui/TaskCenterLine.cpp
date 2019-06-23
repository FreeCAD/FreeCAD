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
TaskCenterLine::TaskCenterLine(TechDraw::DrawViewPart* partFeat,
                               TechDraw::DrawPage* page,
                               std::string edgeName) :
    ui(new Ui_TaskCenterLine),
    m_partFeat(partFeat),
    m_basePage(page),
    m_createMode(false),
    m_edgeName(edgeName)
{
//    Base::Console().Message("TCL::TCL() - edit mode\n");
    ui->setupUi(this);

    m_geomIndex = DrawUtil::getIndexFromName(m_edgeName);
    const std::vector<TechDraw::BaseGeom  *> &geoms = partFeat->getEdgeGeometry();
    BaseGeom* bg = geoms.at(m_geomIndex);
    m_clIdx = bg->sourceIndex();
    m_cl = partFeat->getCenterLineByIndex(m_clIdx);
    if (m_cl == nullptr) {         //checked by CommandAnnotate.  Should never happen.
        Base::Console().Message("TCL::TCL() - no centerline found\n");
    }

    setUiEdit();
}

//ctor for creation
TaskCenterLine::TaskCenterLine(TechDraw::DrawViewPart* partFeat,
                               TechDraw::DrawPage* page,
                               std::vector<std::string> subNames) :
    ui(new Ui_TaskCenterLine),
    m_partFeat(partFeat),
    m_basePage(page),
    m_createMode(true),
    m_subNames(subNames)
{
//    Base::Console().Message("TCL::TCL() - create mode\n");
    if ( (m_basePage == nullptr) ||
         (m_partFeat == nullptr) )  {
        //should be caught in CMD caller
        Base::Console().Error("TaskCenterLine - bad parameters.  Can not proceed.\n");
        return;
    }

    ui->setupUi(this);

    setUiPrimary();
}

TaskCenterLine::~TaskCenterLine()
{
    delete ui;
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
    setWindowTitle(QObject::tr("Create Center Line"));

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
    Base::Quantity qVal;
    qVal.setUnit(Base::Unit::Length);
    qVal.setValue(getExtendBy());
    ui->qsbExtend->setValue(qVal);
    int precision = Base::UnitsApi::getDecimals();
    ui->dsbRotate->setDecimals(precision);
}

void TaskCenterLine::setUiEdit()
{
//    Base::Console().Message("TCL::setUiEdit()\n");
    setWindowTitle(QObject::tr("Edit Center Line"));
    if (m_partFeat != nullptr) {
        std::string baseName = m_partFeat->getNameInDocument();
        ui->leBaseView->setText(Base::Tools::fromStdString(baseName));
        QString listItem = Base::Tools::fromStdString(m_edgeName);
        ui->lstSubList->addItem(listItem);
    }

    ui->cpLineColor->setColor(m_cl->fmt.m_color.asValue<QColor>());
    ui->dsbWeight->setValue(m_cl->fmt.m_weight);
    ui->cboxStyle->setCurrentIndex(m_cl->fmt.m_style);

    int precision = Base::UnitsApi::getDecimals();
    ui->dsbRotate->setDecimals(precision);

    ui->rbVertical->setChecked(false);
    ui->rbHorizontal->setChecked(false);
    ui->rbAligned->setChecked(false);
    if (m_cl->mode == 0) {
        ui->rbVertical->setChecked(true);
    } else if (m_cl->mode == 1) {
        ui->rbHorizontal->setChecked(true);
    } else if (m_cl->mode ==2) {
        ui->rbAligned->setChecked(true);
    }
    ui->dsbRotate->setValue(m_cl->rotate);
    Base::Quantity qVal;
    qVal.setUnit(Base::Unit::Length);
    qVal.setValue(m_cl->vShift);
    ui->qsbVertShift->setValue(qVal);
    qVal.setValue(m_cl->hShift);
    ui->qsbHorizShift->setValue(qVal);
    qVal.setValue(m_cl->extendBy);
    ui->qsbExtend->setValue(qVal);
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
    double hShift = ui->qsbHorizShift->rawValue();
    double vShift = ui->qsbVertShift->rawValue();
    double rotate = ui->dsbRotate->value();
    double extendBy = ui->qsbExtend->rawValue();
    std::pair<Base::Vector3d, Base::Vector3d> ends;
    ends = TechDraw::CenterLine::calcEndPoints(m_partFeat,
                         m_subNames,
                         vertical,
                         extendBy,
                         hShift, vShift, rotate);
    TechDraw::CenterLine* cl = new TechDraw::CenterLine(ends.first, ends.second);
    cl->start = ends.first;
    cl->end = ends.second;

    App::Color ac;
    ac.setValue<QColor>(ui->cpLineColor->color());
    cl->fmt.m_color = ac;
    cl->fmt.m_weight = ui->dsbWeight->value();
    cl->fmt.m_style = ui->cboxStyle->currentIndex();
    cl->fmt.m_visible = true;

    if (ui->rbVertical->isChecked()) {
        cl->mode = 0;
    } else if (ui->rbHorizontal->isChecked()) {
        cl->mode = 1;
    } else if (ui->rbAligned->isChecked()) {
        cl->mode = 2;
    }
    cl->m_faces = m_subNames;
    cl->rotate = rotate;
    cl->vShift = vShift;
    cl->hShift = hShift;
    cl->extendBy = extendBy;

    m_partFeat->addCenterLine(cl);
    
//    m_partFeat->requestPaint();       //if execute has not run, then CL will not be in Geoms.
    m_partFeat->recomputeFeature();
    Gui::Command::updateActive();
    Gui::Command::commitCommand();
}

void TaskCenterLine::updateCenterLine(void)
{
//    Base::Console().Message("TCL::updateCenterLine()\n");
    Gui::Command::openCommand("Edit CenterLine");
    m_cl->fmt.m_color.setValue<QColor>(ui->cpLineColor->color() );
    m_cl->fmt.m_weight = ui->dsbWeight->value();
    m_cl->fmt.m_style = ui->cboxStyle->currentIndex();
    m_cl->fmt.m_visible = true;

    if (ui->rbVertical->isChecked()) {
        m_cl->mode = 0;
    } else if (ui->rbHorizontal->isChecked()) {
        m_cl->mode = 1;
    } else if (ui->rbAligned->isChecked()) {
        m_cl->mode = 2;
    }
    m_cl->rotate = ui->dsbRotate->value();
    m_cl->vShift = ui->qsbVertShift->rawValue();
    m_cl->hShift = ui->qsbHorizShift->rawValue();
    m_cl->extendBy = ui->qsbExtend->rawValue();
    m_partFeat->replaceCenterLine(m_clIdx, m_cl);
    m_partFeat->requestPaint();         //is requestPaint enough here?
//    m_partFeat->recomputeFeature();

    Gui::Command::updateActive();
    Gui::Command::commitCommand();
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
    Qt::PenStyle centerStyle = static_cast<Qt::PenStyle> (hGrp->GetInt("CosmoCLStyle", 2));
    return centerStyle;
}

QColor TaskCenterLine::getCenterColor()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/Decorations");
    App::Color fcColor = App::Color((uint32_t) hGrp->GetUnsigned("CosmoCLColor", 0x00000000));
    return fcColor.asValue<QColor>();
}

double TaskCenterLine::getExtendBy(void)
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter().GetGroup("BaseApp")->
                                         GetGroup("Preferences")->GetGroup("Mod/TechDraw/Decorations");
    double ext = hGrp->GetFloat("CosmoCLExtend", 3.0);
    return ext;
}

//******************************************************************************

bool TaskCenterLine::accept()
{
//    Base::Console().Message("TCL::accept()\n");
    Gui::Document* doc = Gui::Application::Instance->getDocument(m_basePage->getDocument());
    if (!doc) return false;

    if (!getCreateMode())  {
        updateCenterLine();
    } else {
        createCenterLine();
    }
    Gui::Command::doCommand(Gui::Command::Gui,"Gui.ActiveDocument.resetEdit()");

    return true;
}

bool TaskCenterLine::reject()
{
    Gui::Document* doc = Gui::Application::Instance->getDocument(m_basePage->getDocument());
    if (!doc) return false;

    if (getCreateMode() &&
        (m_partFeat != nullptr) )  {
//        Base::Console().Message("TCL::reject - credit Mode!!\n");
        //nothing to remove. 
    }

    if (!getCreateMode() &&
        (m_partFeat != nullptr) )  {
//        Base::Console().Message("TCL::reject - edit Mode!!\n");
          //nothing to un-update
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
    taskbox = new Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("actions/techdraw-facecenterline"),
                                             widget->windowTitle(), true, 0);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

TaskDlgCenterLine::TaskDlgCenterLine(TechDraw::DrawViewPart* partFeat,
                                     TechDraw::DrawPage* page,
                                     std::string edgeName)
    : TaskDialog()
{
    widget  = new TaskCenterLine(partFeat,page, edgeName);
    taskbox = new Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("actions/techdraw-facecenterline"),
                                             widget->windowTitle(), true, 0);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

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
