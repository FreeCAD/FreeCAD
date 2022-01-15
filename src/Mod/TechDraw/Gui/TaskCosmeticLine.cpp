/***************************************************************************
 *   Copyright (c) 2020 WandererFan <wandererfan@gmail.com                 *
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

#include <BRepBuilderAPI_MakeEdge.hxx>

#include <QButtonGroup>
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

#include <Mod/TechDraw/Gui/ui_TaskCosmeticLine.h>

#include "DrawGuiStd.h"
#include "PreferencesGui.h"
#include "QGVPage.h"
#include "QGIView.h"
#include "QGIPrimPath.h"
#include "MDIViewPage.h"
#include "ViewProviderPage.h"
#include "ViewProviderViewPart.h"
#include "Rez.h"

#include "TaskCosmeticLine.h"

using namespace Gui;
using namespace TechDraw;
using namespace TechDrawGui;

//ctor for edit
TaskCosmeticLine::TaskCosmeticLine(TechDraw::DrawViewPart* partFeat,
                                   std::string edgeName) :
    ui(new Ui_TaskCosmeticLine),
    m_partFeat(partFeat),
    m_edgeName(edgeName),
    m_ce(nullptr),
    m_saveCE(nullptr),
    m_createMode(false)
{
    //existence of partFeat is checked in calling command

    m_ce = m_partFeat->getCosmeticEdgeBySelection(m_edgeName);
    if (m_ce == nullptr) {
        Base::Console().Error("TaskCosmeticLine - bad parameters.  Can not proceed.\n");
        return;
    }

    ui->setupUi(this);

    setUiEdit();
}

//ctor for creation
TaskCosmeticLine::TaskCosmeticLine(TechDraw::DrawViewPart* partFeat,
                                   std::vector<Base::Vector3d> points,
                                   std::vector<bool> is3d) :
    ui(new Ui_TaskCosmeticLine),
    m_partFeat(partFeat),
    m_ce(nullptr),
    m_saveCE(nullptr),
    m_points(points),
    m_is3d(is3d),
    m_createMode(true)
{
    //existence of partFeat is checked in calling command

    ui->setupUi(this);

    setUiPrimary();
}

TaskCosmeticLine::~TaskCosmeticLine()
{
    if (m_saveCE != nullptr) {
        delete m_saveCE;
    }
}

void TaskCosmeticLine::updateTask()
{
//    blockUpdate = true;

//    blockUpdate = false;
}

void TaskCosmeticLine::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
}

void TaskCosmeticLine::setUiPrimary()
{
    setWindowTitle(QObject::tr("Create Cosmetic Line"));

    if (m_is3d.front()) {
        ui->rb2d1->setChecked(false);
        ui->rb3d1->setChecked(true);
    } else {
        ui->rb2d1->setChecked(true);
        ui->rb3d1->setChecked(false);
    }
    if (m_is3d.back()) {
        ui->rb2d2->setChecked(false);
        ui->rb3d2->setChecked(true);
    } else {
        ui->rb2d2->setChecked(true);
        ui->rb3d2->setChecked(false);
    }
    Base::Vector3d p1 = m_points.front();
    ui->qsbx1->setUnit(Base::Unit::Length);
    ui->qsbx1->setValue(p1.x);
    ui->qsby1->setUnit(Base::Unit::Length);
    ui->qsby1->setValue(p1.y);
    ui->qsby1->setUnit(Base::Unit::Length);
    ui->qsbz1->setValue(p1.z);

    Base::Vector3d p2 = m_points.back();
    ui->qsbx2->setUnit(Base::Unit::Length);
    ui->qsbx2->setValue(p2.x);
    ui->qsby2->setUnit(Base::Unit::Length);
    ui->qsby2->setValue(p2.y);
    ui->qsbz2->setUnit(Base::Unit::Length);
    ui->qsbz2->setValue(p2.z);
}

void TaskCosmeticLine::setUiEdit()
{
    setWindowTitle(QObject::tr("Edit Cosmetic Line"));

    ui->rb2d1->setChecked(true);
    ui->rb3d1->setChecked(false);
    ui->rb2d2->setChecked(true);
    ui->rb3d2->setChecked(false);

    Base::Vector3d p1 = DrawUtil::invertY(m_ce->permaStart);
    ui->qsbx1->setValue(p1.x);
    ui->qsby1->setValue(p1.y);
    ui->qsbz1->setValue(p1.z);

    Base::Vector3d p2 = DrawUtil::invertY(m_ce->permaEnd);
    ui->qsbx2->setValue(p2.x);
    ui->qsby2->setValue(p2.y);
    ui->qsbz2->setValue(p2.z);
}

//******************************************************************************
void TaskCosmeticLine::createCosmeticLine(void)
{
    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Create Cosmetic Line"));

    double x = ui->qsbx1->value().getValue();
    double y = ui->qsby1->value().getValue();
    double z = ui->qsbz1->value().getValue();
    Base::Vector3d p0(x, y, z);

    x = ui->qsbx2->value().getValue();
    y = ui->qsby2->value().getValue();
    z = ui->qsbz2->value().getValue();
    Base::Vector3d p1(x, y, z);

    Base::Vector3d centroid = m_partFeat->getOriginalCentroid();

    if (ui->rb3d1->isChecked()) {
        p0 = p0 - centroid;
        p0 = DrawUtil::invertY(m_partFeat->projectPoint(p0));
    }

    if (ui->rb3d2->isChecked()) {
        p1 = p1 - centroid;
        p1 = DrawUtil::invertY(m_partFeat->projectPoint(p1));
    }

    m_tag = m_partFeat->addCosmeticEdge(p0, p1);
    m_ce = m_partFeat->getCosmeticEdge(m_tag);

    Gui::Command::commitCommand();
}

void TaskCosmeticLine::updateCosmeticLine(void)
{
    double x = ui->qsbx1->value().getValue();
    double y = ui->qsby1->value().getValue();
    double z = ui->qsbz1->value().getValue();
    Base::Vector3d p0(x, y, z);
    p0 = DrawUtil::invertY(p0);

    x = ui->qsbx2->value().getValue();
    y = ui->qsby2->value().getValue();
    z = ui->qsbz2->value().getValue();
    Base::Vector3d p1(x, y, z);
    p1 = DrawUtil::invertY(p1);

    //replace the geometry
    m_ce->permaStart = p0;
    m_ce->permaEnd = p1;
    gp_Pnt gp1(p0.x, p0.y, p0.z);
    gp_Pnt gp2(p1.x, p1.y, p1.z);
    TopoDS_Edge e = BRepBuilderAPI_MakeEdge(gp1, gp2);
//    auto oldGeom = m_ce->m_geometry;
    m_ce->m_geometry = TechDraw::BaseGeom::baseFactory(e);
//    delete oldGeom;

//    Gui::Command::updateActive();
//    Gui::Command::commitCommand();
}

//******************************************************************************

bool TaskCosmeticLine::accept()
{
    if (m_createMode) {
        createCosmeticLine();
        m_partFeat->add1CEToGE(m_tag);
        m_partFeat->refreshCEGeoms();
        m_partFeat->requestPaint();
    } else {
        //update mode
        Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Update CosmeticLine"));
        updateCosmeticLine();
        m_partFeat->refreshCEGeoms();
        m_partFeat->requestPaint();
        Gui::Command::updateActive();
        Gui::Command::commitCommand();
    }

    Gui::Command::doCommand(Gui::Command::Gui,"Gui.ActiveDocument.resetEdit()");

    return true;
}

bool TaskCosmeticLine::reject()
{
    //there's nothing to do.
    Gui::Command::doCommand(Gui::Command::Gui,"Gui.ActiveDocument.resetEdit()");
    return false;
}
////////////////////////////////////////////////////////////////////////////////
TaskDlgCosmeticLine::TaskDlgCosmeticLine(TechDraw::DrawViewPart* partFeat,
                                     std::vector<Base::Vector3d> points,
                                     std::vector<bool> is3d)
    : TaskDialog()
{
    widget  = new TaskCosmeticLine(partFeat, points, is3d);
    taskbox = new Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("actions/techdraw-line2points"),
                                             widget->windowTitle(), true, 0);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

TaskDlgCosmeticLine::TaskDlgCosmeticLine(TechDraw::DrawViewPart* partFeat,
                                     std::string edgeName)
    : TaskDialog()
{
    widget  = new TaskCosmeticLine(partFeat, edgeName);
    taskbox = new Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("actions/techdraw-line2points"),
                                             widget->windowTitle(), true, 0);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

TaskDlgCosmeticLine::~TaskDlgCosmeticLine()
{
}

void TaskDlgCosmeticLine::update()
{
//    widget->updateTask();
}

//==== calls from the TaskView ===============================================================
void TaskDlgCosmeticLine::open()
{
}

void TaskDlgCosmeticLine::clicked(int)
{
}

bool TaskDlgCosmeticLine::accept()
{
    widget->accept();
    return true;
}

bool TaskDlgCosmeticLine::reject()
{
    widget->reject();
    return true;
}

#include <Mod/TechDraw/Gui/moc_TaskCosmeticLine.cpp>
