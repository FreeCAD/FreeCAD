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
#ifndef _PreComp_
# include <cmath>
# include <BRepBuilderAPI_MakeEdge.hxx>
#endif

#include <Base/Console.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/Selection.h>
#include <Mod/TechDraw/App/DrawUtil.h>
#include <Mod/TechDraw/App/DrawViewPart.h>
#include <Mod/TechDraw/App/Cosmetic.h>
#include <Mod/TechDraw/App/Geometry.h>

#include "ui_TaskCosmeticLine.h"
#include "TaskCosmeticLine.h"


using namespace Gui;
using namespace TechDraw;
using namespace TechDrawGui;
using DU = DrawUtil;

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
    if (!m_ce) {
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
    if (m_saveCE) {
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

    double rotDeg = m_partFeat->Rotation.getValue();
    double rotRad = rotDeg * M_PI / 180.0;
    Base::Vector3d centroid = m_partFeat->getCurrentCentroid();
    Base::Vector3d p1, p2;
    if (m_is3d.front()) {
        // center, project and invert the 3d point
        p1 = DrawUtil::invertY(m_partFeat->projectPoint(m_points.front() - centroid));
        ui->rb2d1->setChecked(false);
        ui->rb3d1->setChecked(true);
    } else {
        // invert, unscale and unrotate the selected 2d point
        p1 = DU::invertY(m_points.front()) / m_partFeat->getScale();
        if (rotDeg != 0.0) {
            // we always rotate around the origin.
            p1.RotateZ(-rotRad);
        }
        ui->rb2d1->setChecked(true);
        ui->rb3d1->setChecked(false);
    }

    if (m_is3d.back()) {
        p2 = DrawUtil::invertY(m_partFeat->projectPoint(m_points.back() - centroid));
        ui->rb2d2->setChecked(false);
        ui->rb3d2->setChecked(true);
    } else {
        p2 = DU::invertY(m_points.back()) / m_partFeat->getScale();
        if (rotDeg != 0.0) {
            p2.RotateZ(-rotRad);
        }
        ui->rb2d2->setChecked(true);
        ui->rb3d2->setChecked(false);
    }

    ui->qsbx1->setUnit(Base::Unit::Length);
    ui->qsbx1->setValue(p1.x);
    ui->qsby1->setUnit(Base::Unit::Length);
    ui->qsby1->setValue(p1.y);
    ui->qsby1->setUnit(Base::Unit::Length);
    ui->qsbz1->setValue(p1.z);

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
//    Base::Console().Message("TCL::createCosmeticLine()\n");
    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Create Cosmetic Line"));

    double x = ui->qsbx1->value().getValue();
    double y = ui->qsby1->value().getValue();
    double z = ui->qsbz1->value().getValue();
    Base::Vector3d p0(x, y, z);

    x = ui->qsbx2->value().getValue();
    y = ui->qsby2->value().getValue();
    z = ui->qsbz2->value().getValue();
    Base::Vector3d p1(x, y, z);

    m_tag = m_partFeat->addCosmeticEdge(p0, p1);
    m_ce = m_partFeat->getCosmeticEdge(m_tag);

    Gui::Command::commitCommand();
}

void TaskCosmeticLine::updateCosmeticLine(void)
{
//    Base::Console().Message("TCL::updateCosmeticLine()\n");
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
    m_ce->m_geometry = TechDraw::BaseGeom::baseFactory(e);
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

    Gui::Command::doCommand(Gui::Command::Gui, "Gui.ActiveDocument.resetEdit()");

    return true;
}

bool TaskCosmeticLine::reject()
{
    //there's nothing to do.
    Gui::Command::doCommand(Gui::Command::Gui, "Gui.ActiveDocument.resetEdit()");
    return false;
}
////////////////////////////////////////////////////////////////////////////////
TaskDlgCosmeticLine::TaskDlgCosmeticLine(TechDraw::DrawViewPart* partFeat,
                                     std::vector<Base::Vector3d> points,
                                     std::vector<bool> is3d)
    : TaskDialog()
{
    widget  = new TaskCosmeticLine(partFeat, points, is3d);
    taskbox = new Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("actions/TechDraw_Line2Points"),
                                             widget->windowTitle(), true, nullptr);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

TaskDlgCosmeticLine::TaskDlgCosmeticLine(TechDraw::DrawViewPart* partFeat,
                                     std::string edgeName)
    : TaskDialog()
{
    widget  = new TaskCosmeticLine(partFeat, edgeName);
    taskbox = new Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("actions/TechDraw_Line2Points"),
                                             widget->windowTitle(), true, nullptr);
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
