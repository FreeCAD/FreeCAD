/***************************************************************************
 *   Copyright (c) 2023 WandererFan <wandererfan@gmail.com>                *
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

#include "ui_TaskCosmeticCircle.h"
#include "TaskCosmeticCircle.h"


using namespace Gui;
using namespace TechDraw;
using namespace TechDrawGui;
using DU = DrawUtil;

//ctor for edit
TaskCosmeticCircle::TaskCosmeticCircle(TechDraw::DrawViewPart* partFeat,
                                   std::string circleName) :
    ui(new Ui_TaskCosmeticCircle),
    m_partFeat(partFeat),
    m_circleName(circleName),
    m_ce(nullptr),
    m_saveCE(nullptr),
    m_createMode(false)
{
    //existence of partFeat is checked in calling command

    m_ce = m_partFeat->getCosmeticEdgeBySelection(m_circleName);
    if (!m_ce) {
        Base::Console().Error("TaskCosmeticCircle - bad parameters.  Can not proceed.\n");
        return;
    }

    ui->setupUi(this);

    setUiEdit();
}

//ctor for creation
TaskCosmeticCircle::TaskCosmeticCircle(TechDraw::DrawViewPart* partFeat,
                                   Base::Vector3d center, bool is3d) :
    ui(new Ui_TaskCosmeticCircle),
    m_partFeat(partFeat),
    m_ce(nullptr),
    m_saveCE(nullptr),
    m_center(center),
    m_createMode(true),
    m_is3d(is3d)
{
    //existence of partFeat is checked in calling command

    ui->setupUi(this);

    setUiPrimary();
}

TaskCosmeticCircle::~TaskCosmeticCircle()
{
    if (m_saveCE) {
        delete m_saveCE;
    }
}

void TaskCosmeticCircle::updateTask()
{
//    blockUpdate = true;

//    blockUpdate = false;
}

void TaskCosmeticCircle::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
}

void TaskCosmeticCircle::setUiPrimary()
{
    setWindowTitle(QObject::tr("Create Cosmetic Line"));
//    Base::Console().Message("TCC::setUiPrimary() - m_center: %s is3d: %d\n",
//        DU::formatVector(m_center).c_str(), m_is3d);
    double rotDeg = m_partFeat->Rotation.getValue();
    double rotRad = rotDeg * M_PI / 180.0;
    Base::Vector3d centroid = m_partFeat->getCurrentCentroid();
    Base::Vector3d p1;
    if (m_is3d) {
        // center, project and invert the 3d point
        p1 = DrawUtil::invertY(m_partFeat->projectPoint(m_center - centroid));
        ui->rb2d1->setChecked(false);
        ui->rb3d1->setChecked(true);
    } else {
        // invert, unscale and unrotate the selected 2d point
        // shift by centroid?
        p1 = DU::invertY(m_center) / m_partFeat->getScale();
        if (rotDeg != 0.0) {
            // we always rotate around the origin.
            p1.RotateZ(-rotRad);
        }
        ui->rb2d1->setChecked(true);
        ui->rb3d1->setChecked(false);
    }

    ui->qsbCenterX->setUnit(Base::Unit::Length);
    ui->qsbCenterX->setValue(p1.x);
    ui->qsbCenterY->setUnit(Base::Unit::Length);
    ui->qsbCenterY->setValue(p1.y);
    ui->qsbCenterY->setUnit(Base::Unit::Length);
    ui->qsbCenterZ->setValue(p1.z);
}

void TaskCosmeticCircle::setUiEdit()
{
    setWindowTitle(QObject::tr("Edit Cosmetic Line"));

    ui->rb2d1->setChecked(true);
    ui->rb3d1->setChecked(false);

    Base::Vector3d p1 = DrawUtil::invertY(m_ce->permaStart);
    ui->qsbCenterX->setValue(p1.x);
    ui->qsbCenterY->setValue(p1.y);
    ui->qsbCenterZ->setValue(p1.z);

    ui->qsbRadius->setValue(m_ce->permaRadius);

    double angleDeg = m_ce->m_geometry->getStartAngle() * 180.0 / M_PI;
    ui->qsbStartAngle->setValue(angleDeg);
    angleDeg = m_ce->m_geometry->getEndAngle() * 180.0 / M_PI;
    ui->qsbEndAngle->setValue(angleDeg);
}

//******************************************************************************
void TaskCosmeticCircle::createCosmeticCircle(void)
{
//    Base::Console().Message("TCL::createCosmeticCircle()\n");
    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Create Cosmetic Line"));

    double x = ui->qsbCenterX->value().getValue();
    double y = ui->qsbCenterY->value().getValue();
    double z = ui->qsbCenterZ->value().getValue();
    Base::Vector3d p0(x, y, z);

    TechDraw::BaseGeomPtr bg;
    if (ui->qsbStartAngle->value().getValue() == 0.0 &&
        ui->qsbEndAngle->value().getValue() == 0.0)  {
        bg = std::make_shared<TechDraw::Circle> (p0, ui->qsbRadius->value().getValue());
    } else {
        bg = std::make_shared<TechDraw::AOC>(p0, ui->qsbRadius->value().getValue(),
                                     ui->qsbStartAngle->value().getValue(),
                                     ui->qsbEndAngle->value().getValue());
    }

    // note cEdges are inverted when added to the dvp geometry, so we need to
    // invert them here
    m_tag = m_partFeat->addCosmeticEdge(bg->inverted());
    m_ce = m_partFeat->getCosmeticEdge(m_tag);

    Gui::Command::commitCommand();
}

void TaskCosmeticCircle::updateCosmeticCircle(void)
{
//    Base::Console().Message("TCL::updateCosmeticCircle()\n");
    double x = ui->qsbCenterX->value().getValue();
    double y = ui->qsbCenterY->value().getValue();
    double z = ui->qsbCenterZ->value().getValue();
    Base::Vector3d p0(x, y, z);

    //replace the geometry
    m_ce->permaRadius = ui->qsbRadius->value().getValue();

    TechDraw::BaseGeomPtr bg;
    if (ui->qsbStartAngle->value().getValue() == 0.0 &&
        ui->qsbEndAngle->value().getValue() == 0.0)  {
        bg = std::make_shared<TechDraw::Circle> (p0, m_ce->permaRadius);
    } else {
        bg = std::make_shared<TechDraw::AOC>(p0, ui->qsbRadius->value().getValue(),
                                             ui->qsbStartAngle->value().getValue(),
                                             ui->qsbEndAngle->value().getValue());
    }
    m_ce->m_geometry = bg->inverted();
    p0 = DU::invertY(p0);
    m_ce->permaStart = p0;
    m_ce->permaEnd = p0;
}

//******************************************************************************

bool TaskCosmeticCircle::accept()
{
    if (m_createMode) {
        createCosmeticCircle();
        m_partFeat->add1CEToGE(m_tag);
        m_partFeat->refreshCEGeoms();
        m_partFeat->requestPaint();
    } else {
        //update mode
        Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Update CosmeticCircle"));
        updateCosmeticCircle();
        m_partFeat->refreshCEGeoms();
        m_partFeat->requestPaint();
        Gui::Command::updateActive();
        Gui::Command::commitCommand();
    }

    Gui::Command::doCommand(Gui::Command::Gui, "Gui.ActiveDocument.resetEdit()");

    return true;
}

bool TaskCosmeticCircle::reject()
{
    //there's nothing to do.
    Gui::Command::doCommand(Gui::Command::Gui, "Gui.ActiveDocument.resetEdit()");
    return false;
}
////////////////////////////////////////////////////////////////////////////////
TaskDlgCosmeticCircle::TaskDlgCosmeticCircle(TechDraw::DrawViewPart* partFeat,
                                     Base::Vector3d point,
                                     bool is3d)
    : TaskDialog()
{
    widget  = new TaskCosmeticCircle(partFeat, point, is3d);
    taskbox = new Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("actions/TechDraw_CosmeticCircle"),
                                             widget->windowTitle(), true, nullptr);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

TaskDlgCosmeticCircle::TaskDlgCosmeticCircle(TechDraw::DrawViewPart* partFeat,
                                     std::string circleName)
    : TaskDialog()
{
    widget  = new TaskCosmeticCircle(partFeat, circleName);
    taskbox = new Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("actions/TechDraw_CosmeticCircle"),
                                             widget->windowTitle(), true, nullptr);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

TaskDlgCosmeticCircle::~TaskDlgCosmeticCircle()
{
}

void TaskDlgCosmeticCircle::update()
{
//    widget->updateTask();
}

//==== calls from the TaskView ===============================================================
void TaskDlgCosmeticCircle::open()
{
}

void TaskDlgCosmeticCircle::clicked(int)
{
}

bool TaskDlgCosmeticCircle::accept()
{
    widget->accept();
    return true;
}

bool TaskDlgCosmeticCircle::reject()
{
    widget->reject();
    return true;
}

#include <Mod/TechDraw/Gui/moc_TaskCosmeticCircle.cpp>

