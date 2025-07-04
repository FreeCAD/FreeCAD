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
# include <BRepBuilderAPI_MakeEdge.hxx>
#endif

#include <QMessageBox>

#include <Base/Console.h>
#include <Base/Tools.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/Selection/Selection.h>
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
        Base::Console().error("TaskCosmeticCircle - bad parameters. Cannot proceed.\n");
        return;
    }

    ui->setupUi(this);

    setUiEdit();

    connect(ui->qsbRadius, qOverload<double>(&QuantitySpinBox::valueChanged),
            this, &TaskCosmeticCircle::radiusChanged);
    connect(ui->rbArc, &QRadioButton::clicked, this, &TaskCosmeticCircle::arcButtonClicked);


}

//ctor for creation
TaskCosmeticCircle::TaskCosmeticCircle(TechDraw::DrawViewPart* partFeat,
                                   std::vector<Base::Vector3d> points, bool is3d) :
    ui(new Ui_TaskCosmeticCircle),
    m_partFeat(partFeat),
    m_ce(nullptr),
    m_saveCE(nullptr),
    m_createMode(true),
    m_is3d(is3d),
    m_points(points)
{
    //existence of partFeat is checked in calling command

    ui->setupUi(this);

    setUiPrimary();

    connect(ui->qsbRadius, qOverload<double>(&QuantitySpinBox::valueChanged),
            this, &TaskCosmeticCircle::radiusChanged);
    connect(ui->rbArc, &QRadioButton::clicked, this, &TaskCosmeticCircle::arcButtonClicked);

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
    setWindowTitle(QObject::tr("Create Cosmetic Circle"));
    std::vector<Base::Vector3d> displayPoints;
    std::vector<Base::Vector3d> mathPoints;
    for (auto& point : m_points) {
        // use conventional coordinates for calculations
        mathPoints.push_back(DU::invertY(point));
    }

    if (!m_points.empty()) {
        m_center = m_points.front();
    }

    Base::Vector3d displayCenter;
    if (m_is3d) {
        ui->rb2d1->setChecked(false);
        ui->rb3d1->setChecked(true);
        // center, project and invert the 3d point
        Base::Vector3d centroid = m_partFeat->getCurrentCentroid();
        displayCenter = m_partFeat->projectPoint(mathPoints[0] - centroid, false);
    } else {
        ui->rb2d1->setChecked(true);
        ui->rb3d1->setChecked(false);
        // if the points are selected from 2d, they are already inverted
        // unscale and unrotate the selected 2d point
        displayCenter = CosmeticVertex::makeCanonicalPointInverted(m_partFeat, m_center);
        displayCenter = DU::invertY(displayCenter);
    }

    ui->qsbCenterX->setUnit(Base::Unit::Length);
    ui->qsbCenterX->setValue(displayCenter.x);
    ui->qsbCenterY->setUnit(Base::Unit::Length);
    ui->qsbCenterY->setValue(displayCenter.y);
    ui->qsbCenterY->setUnit(Base::Unit::Length);
    ui->qsbCenterZ->setValue(displayCenter.z);

    double radius = (mathPoints[1] - mathPoints[0]).Length() / m_partFeat->getScale();
    ui->qsbRadius->setValue(radius);

    ui->qsbStartAngle->setValue(0);
    ui->qsbEndAngle->setValue(360);

    enableArcWidgets(false);
    ui->qsbStartAngle->setEnabled(false);
    ui->qsbEndAngle->setEnabled(false);
    ui->cbClockwise->setEnabled(false);
}

void TaskCosmeticCircle::setUiEdit()
{
    setWindowTitle(QObject::tr("Edit Cosmetic Circle"));

    ui->rb2d1->setChecked(true);
    ui->rb3d1->setChecked(false);

    Base::Vector3d p1 = DrawUtil::invertY(m_ce->permaStart);
    ui->qsbCenterX->setValue(p1.x);
    ui->qsbCenterY->setValue(p1.y);
    ui->qsbCenterZ->setValue(p1.z);

    ui->qsbRadius->setValue(m_ce->permaRadius);

    ui->qsbStartAngle->setValue(Base::toDegrees(m_ce->m_geometry->getStartAngle()));
    ui->qsbEndAngle->setValue(Base::toDegrees(m_ce->m_geometry->getEndAngle()));

    if (m_ce->m_geometry->getGeomType() == GeomType::ARCOFCIRCLE) {
        ui->rbArc->setChecked(true);
        enableArcWidgets(true);
    } else {
        ui->rbArc->setChecked(false);
        enableArcWidgets(false);
    }

}

void TaskCosmeticCircle::radiusChanged()
{
    if (ui->qsbRadius->value().getValue() <= 0.0) {
        QString msg = tr("Radius must be non-zero positive number");
        QMessageBox::critical(Gui::getMainWindow(), QObject::tr("Parameter Error"), msg);
    }
}

void TaskCosmeticCircle::arcButtonClicked()
{
    if (ui->rbArc->isChecked()) {
        enableArcWidgets(true);
    } else {
        enableArcWidgets(false);
    }
}

void TaskCosmeticCircle::enableArcWidgets(bool newState)
{
    ui->qsbStartAngle->setEnabled(newState);
    ui->qsbEndAngle->setEnabled(newState);
    ui->cbClockwise->setEnabled(newState);

}


//******************************************************************************
void TaskCosmeticCircle::createCosmeticCircle(void)
{
//    Base::Console().message("TCL::createCosmeticCircle()\n");

    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Create Cosmetic Circle"));

    // point from Page/View is conventional coordinates (Y+ up), unscaled, unrotated, but centered (Csriz)
    // this is Canonical form with out inversion.
    // point from 3d is OXYZ and needs to be projected.
    double x = ui->qsbCenterX->value().getValue();
    double y = ui->qsbCenterY->value().getValue();
    double z = ui->qsbCenterZ->value().getValue();
    Base::Vector3d center(x, y, z);
    if (ui->rb3d1->isChecked()) {
        Base::Vector3d centroid = m_partFeat->getCurrentCentroid();
        center = m_partFeat->projectPoint(center - centroid);
    }

    TechDraw::BaseGeomPtr bg;
    if (!ui->rbArc->isChecked()) {
        bg = std::make_shared<TechDraw::Circle> (center, ui->qsbRadius->value().getValue());
    } else {
        bg = std::make_shared<TechDraw::AOC>(center, ui->qsbRadius->value().getValue(),
                                            ui->qsbStartAngle->value().getValue(),
                                            ui->qsbEndAngle->value().getValue());
    }

    // after all the calculations are done, we invert the geometry
    m_tag = m_partFeat->addCosmeticEdge(bg->inverted());
    m_ce = m_partFeat->getCosmeticEdge(m_tag);
    m_ce->setFormat(LineFormat::getCurrentLineFormat());

    Gui::Command::commitCommand();
}

void TaskCosmeticCircle::updateCosmeticCircle(void)
{
    // Base::Console().message("TCL::updateCosmeticCircle()\n");
    double x = ui->qsbCenterX->value().getValue();
    double y = ui->qsbCenterY->value().getValue();
    double z = ui->qsbCenterZ->value().getValue();
    Base::Vector3d p0(x, y, z);

    //replace the geometry
    m_ce->permaRadius = ui->qsbRadius->value().getValue();

    TechDraw::BaseGeomPtr bg;
    if (!ui->rbArc->isChecked()) {
        bg = std::make_shared<TechDraw::Circle> (p0, m_ce->permaRadius);
    } else {
        bg = std::make_shared<TechDraw::AOC>(p0, ui->qsbRadius->value().getValue(),
                                             ui->qsbStartAngle->value().getValue(),
                                             ui->qsbEndAngle->value().getValue());
    }
    m_ce->m_geometry = bg->inverted();
    m_ce->permaStart = p0;
    m_ce->permaEnd = p0;
}

//******************************************************************************

bool TaskCosmeticCircle::accept()
{
    if (ui->qsbRadius->value().getValue() <= 0.0) {
        // this won't work!
        Base::Console().error("TaskCosmeticCircle - cannot create a circle with radius: %.3f\n",
                                ui->qsbRadius->value().getValue());
        return false;
    }
    if (m_createMode) {
        createCosmeticCircle();
        m_partFeat->add1CEToGE(m_tag);
        m_partFeat->refreshCEGeoms();
        m_partFeat->requestPaint();
    } else {
        //update mode
        Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Update Cosmetic Circle"));
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
                                     std::vector<Base::Vector3d> points,
                                     bool is3d)
    : TaskDialog()
{
    widget  = new TaskCosmeticCircle(partFeat, points, is3d);
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

