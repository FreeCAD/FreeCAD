/***************************************************************************
 *   Copyright (c) 2007 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
#include <Python.h>
#include <gp_Ax1.hxx>
#include <gp_Ax3.hxx>
#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>
#include <GC_MakeArcOfCircle.hxx>
#include <Geom_Circle.hxx>
#include <Geom_TrimmedCurve.hxx>
#include <Handle_Geom_Circle.hxx>
#include <Handle_Geom_TrimmedCurve.hxx>
#include <Inventor/SoPickedPoint.h>
#include <Inventor/events/SoMouseButtonEvent.h>
#endif

#include <Base/Interpreter.h>
#include <Base/Rotation.h>
#include <Base/Tools.h>
#include <App/Application.h>
#include <App/Document.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/Command.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
#include <Gui/SoFCUnifiedSelection.h>
#include <Gui/TaskView/TaskView.h>
#include <Mod/Part/App/Tools.h>

#include "DlgPrimitives.h"

using namespace PartGui;

namespace PartGui {

const char* gce_ErrorStatusText(gce_ErrorType et)
{
    switch (et)
    {
    case gce_Done:
        return "Construction was successful";
    case gce_ConfusedPoints:
        return "Two points are coincident";
    case gce_NegativeRadius:
        return "Radius value is negative";
    case gce_ColinearPoints:
        return "Three points are collinear";
    case gce_IntersectionError:
        return "Intersection cannot be computed";
    case gce_NullAxis:
        return "Axis is undefined";
    case gce_NullAngle:
        return "Angle value is invalid (usually null)";
    case gce_NullRadius:
        return "Radius is null";
    case gce_InvertAxis:
        return "Axis value is invalid";
    case gce_BadAngle:
        return "Angle value is invalid";
    case gce_InvertRadius:
        return "Radius value is incorrect (usually with respect to another radius)";
    case gce_NullFocusLength:
        return "Focal distance is null";
    case gce_NullVector:
        return "Vector is null";
    case gce_BadEquation:
        return "Coefficients are incorrect (applies to the equation of a geometric object)";
    default:
        return "Creation of geometry failed";
    }
}

void Picker::createPrimitive(QWidget* widget, const QString& descr, Gui::Document* doc)
{
    try {
        App::Document* app = doc->getDocument();
        QString cmd = this->command(app);

        // Execute the Python block
        doc->openCommand(descr.toUtf8());
        Gui::Command::doCommand(Gui::Command::Doc, (const char*)cmd.toAscii());
        doc->commitCommand();
        Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.recompute()");
        Gui::Command::doCommand(Gui::Command::Gui, "Gui.SendMsgToActiveView(\"ViewFit\")");
    }
    catch (const Base::Exception& e) {
        QMessageBox::warning(widget, descr, QString::fromLatin1(e.what()));
    }
}

QString Picker::toPlacement(const gp_Ax2& axis) const
{
    gp_Dir dir = axis.Direction();
    gp_Pnt pnt = gp_Pnt(0.0,0.0,0.0);
    gp_Ax3 ax3(pnt, dir, axis.XDirection());

    gp_Trsf Trf;
    Trf.SetTransformation(ax3);
    Trf.Invert();

    gp_XYZ theAxis(0,0,1);
    Standard_Real theAngle = 0.0;
    Trf.GetRotation(theAxis,theAngle);

    Base::Rotation rot(Base::convertTo<Base::Vector3d>(theAxis), theAngle);
    gp_Pnt loc = axis.Location();

    return QString::fromAscii("Base.Placement(Base.Vector(%1,%2,%3),Base.Rotation(%4,%5,%6,%7))")
        .arg(loc.X(),0,'f',2)
        .arg(loc.Y(),0,'f',2)
        .arg(loc.Z(),0,'f',2)
        .arg(rot[0],0,'f',2)
        .arg(rot[1],0,'f',2)
        .arg(rot[2],0,'f',2)
        .arg(rot[3],0,'f',2);
}

class CircleFromThreePoints : public Picker
{
public:
    CircleFromThreePoints() : Picker()
    {
    }
    bool pickedPoint(const SoPickedPoint * point)
    {
        SbVec3f pnt = point->getPoint();
        points.push_back(gp_Pnt(pnt[0],pnt[1],pnt[2]));
        return points.size() == 3;
    }
    QString command(App::Document* doc) const
    {
        GC_MakeArcOfCircle arc(points[0], points[1], points[2]);
        if (!arc.IsDone())
            throw Base::Exception(gce_ErrorStatusText(arc.Status()));
        Handle_Geom_TrimmedCurve trim = arc.Value();
        Handle_Geom_Circle circle = Handle_Geom_Circle::DownCast(trim->BasisCurve());

        QString name = QString::fromAscii(doc->getUniqueObjectName("Circle").c_str());
        return QString::fromAscii(
            "App.ActiveDocument.addObject(\"Part::Circle\",\"%1\")\n"
            "App.ActiveDocument.%1.Radius=%2\n"
            "App.ActiveDocument.%1.Angle0=%3\n"
            "App.ActiveDocument.%1.Angle1=%4\n"
            "App.ActiveDocument.%1.Placement=%5\n")
            .arg(name)
            .arg(circle->Radius(),0,'f',2)
            .arg(Base::toDegrees(trim->FirstParameter()),0,'f',2)
            .arg(Base::toDegrees(trim->LastParameter ()),0,'f',2)
            .arg(toPlacement(circle->Position()));
    }

private:
    std::vector<gp_Pnt> points;
};

}

/* TRANSLATOR PartGui::DlgPrimitives */

DlgPrimitives::DlgPrimitives(QWidget* parent)
  : QWidget(parent)
{
    ui.setupUi(this);
    Gui::Command::doCommand(Gui::Command::Doc, "from FreeCAD import Base");
    Gui::Command::doCommand(Gui::Command::Doc, "import Part,PartGui");

    // set limits
    //
    // plane
    ui.planeLength->setMaximum(INT_MAX);
    ui.planeWidth->setMaximum(INT_MAX);
    // box
    ui.boxLength->setMaximum(INT_MAX);
    ui.boxWidth->setMaximum(INT_MAX);
    ui.boxHeight->setMaximum(INT_MAX);
    // cylinder
    ui.cylinderRadius->setMaximum(INT_MAX);
    ui.cylinderHeight->setMaximum(INT_MAX);
    // cone
    ui.coneRadius1->setMaximum(INT_MAX);
    ui.coneRadius2->setMaximum(INT_MAX);
    ui.coneHeight->setMaximum(INT_MAX);
    // sphere
    ui.sphereRadius->setMaximum(INT_MAX);
    // ellipsoid
    ui.ellipsoidRadius1->setMaximum(INT_MAX);
    ui.ellipsoidRadius2->setMaximum(INT_MAX);
    // torus
    ui.torusRadius1->setMaximum(INT_MAX);
    ui.torusRadius2->setMaximum(INT_MAX);
    // wedge
    ui.wedgeXmin->setMinimum(INT_MIN);
    ui.wedgeXmin->setMaximum(INT_MAX);
    ui.wedgeYmin->setMinimum(INT_MIN);
    ui.wedgeYmin->setMaximum(INT_MAX);
    ui.wedgeZmin->setMinimum(INT_MIN);
    ui.wedgeZmin->setMaximum(INT_MAX);
    ui.wedgeX2min->setMinimum(INT_MIN);
    ui.wedgeX2min->setMaximum(INT_MAX);
    ui.wedgeZ2min->setMinimum(INT_MIN);
    ui.wedgeZ2min->setMaximum(INT_MAX);
    ui.wedgeXmax->setMinimum(INT_MIN);
    ui.wedgeXmax->setMaximum(INT_MAX);
    ui.wedgeYmax->setMinimum(INT_MIN);
    ui.wedgeYmax->setMaximum(INT_MAX);
    ui.wedgeZmax->setMinimum(INT_MIN);
    ui.wedgeZmax->setMaximum(INT_MAX);
    ui.wedgeX2max->setMinimum(INT_MIN);
    ui.wedgeX2max->setMaximum(INT_MAX);
    ui.wedgeZ2max->setMinimum(INT_MIN);
    ui.wedgeZ2max->setMaximum(INT_MAX);
    // helix
    ui.helixPitch->setMaximum(INT_MAX);
    ui.helixHeight->setMaximum(INT_MAX);
    ui.helixRadius->setMaximum(INT_MAX);
    // circle
    ui.circleRadius->setMaximum(INT_MAX);
    // vertex
    ui.vertexX->setMaximum(INT_MAX);
    ui.vertexY->setMaximum(INT_MAX);
    ui.vertexZ->setMaximum(INT_MAX);
    ui.vertexX->setMinimum(INT_MIN);
    ui.vertexY->setMinimum(INT_MIN);
    ui.vertexZ->setMinimum(INT_MIN);
    // line
    ui.edgeX1->setMaximum(INT_MAX);
    ui.edgeX1->setMinimum(INT_MIN);
    ui.edgeY1->setMaximum(INT_MAX);
    ui.edgeY1->setMinimum(INT_MIN);
    ui.edgeZ1->setMaximum(INT_MAX);
    ui.edgeZ1->setMinimum(INT_MIN);
    ui.edgeX2->setMaximum(INT_MAX);
    ui.edgeX2->setMinimum(INT_MIN);
    ui.edgeY2->setMaximum(INT_MAX);
    ui.edgeY2->setMinimum(INT_MIN);
    ui.edgeZ2->setMaximum(INT_MAX);
    ui.edgeZ2->setMinimum(INT_MIN);
    // RegularPolygon
}

/*  
 *  Destroys the object and frees any allocated resources
 */
DlgPrimitives::~DlgPrimitives()
{
}

void DlgPrimitives::pickCallback(void * ud, SoEventCallback * n)
{
    const SoMouseButtonEvent * mbe = static_cast<const SoMouseButtonEvent*>(n->getEvent());
    Picker* pick = reinterpret_cast<Picker*>(ud);
    if (pick->exitCode >= 0)
        pick->loop.exit(pick->exitCode);

    // Mark all incoming mouse button events as handled, especially, to deactivate the selection node
    n->setHandled();
    if (mbe->getButton() == SoMouseButtonEvent::BUTTON1) {
        if (mbe->getState() == SoButtonEvent::DOWN) {
            const SoPickedPoint * point = n->getPickedPoint();
            if (point) {
                if (pick->pickedPoint(point)) {
                    pick->exitCode = 0;
                }
            }
        }
    }
    else if (mbe->getButton() == SoMouseButtonEvent::BUTTON2) {
        if (mbe->getState() == SoButtonEvent::UP) {
            pick->loop.exit(1);
        }
    }
}

void DlgPrimitives::executeCallback(Picker* p)
{
    Gui::Document* doc = Gui::Application::Instance->activeDocument();
    if (!doc) {
        return;
    }

    Gui::View3DInventor* view = static_cast<Gui::View3DInventor*>(doc->getActiveView());
    if (view) {
        Gui::View3DInventorViewer* viewer = view->getViewer();
        if (!viewer->isEditing()) {
            viewer->setEditing(true);
            viewer->setRedirectToSceneGraph(true);
            SoNode* root = viewer->getSceneGraph();
            int mode;
            if (root && root->getTypeId().isDerivedFrom(Gui::SoFCUnifiedSelection::getClassTypeId())) {
                mode = static_cast<Gui::SoFCUnifiedSelection*>(root)->selectionMode.getValue();
                static_cast<Gui::SoFCUnifiedSelection*>(root)->selectionMode.setValue(Gui::SoFCUnifiedSelection::OFF);
            }
            viewer->addEventCallback(SoMouseButtonEvent::getClassTypeId(), pickCallback, p);
            this->setDisabled(true);
            int ret = p->loop.exec();
            if (root && root->getTypeId().isDerivedFrom(Gui::SoFCUnifiedSelection::getClassTypeId()))
                static_cast<Gui::SoFCUnifiedSelection*>(root)->selectionMode.setValue(mode);
            this->setEnabled(true);
            viewer->setEditing(false);
            viewer->setRedirectToSceneGraph(false);
            viewer->removeEventCallback(SoMouseButtonEvent::getClassTypeId(), pickCallback, p);

            if (ret == 0) {
                p->createPrimitive(this, ui.comboBox1->currentText(), doc);
            }
        }
    }
}

void DlgPrimitives::on_buttonCircleFromThreePoints_clicked()
{
    CircleFromThreePoints pp;
    executeCallback(&pp);
}

void DlgPrimitives::createPrimitive(const QString& placement)
{
    try {
        QString cmd; QString name;
        App::Document* doc = App::GetApplication().getActiveDocument();
        if (!doc) {
            QMessageBox::warning(this, tr("Create %1")
                .arg(ui.comboBox1->currentText()), tr("No active document"));
            return;
        }
        if (ui.comboBox1->currentIndex() == 0) {         // plane
            name = QString::fromAscii(doc->getUniqueObjectName("Plane").c_str());
            cmd = QString::fromAscii(
                "App.ActiveDocument.addObject(\"Part::Plane\",\"%1\")\n"
                "App.ActiveDocument.%1.Length=%2\n"
                "App.ActiveDocument.%1.Width=%3\n"
                "App.ActiveDocument.%1.Placement=%4\n"
                "App.ActiveDocument.%1.Label='%5'\n")
                .arg(name)
                .arg(ui.planeLength->value(),0,'f',2)
                .arg(ui.planeWidth->value(),0,'f',2)
                .arg(placement)
                .arg(tr("Plane"));
        }
        else if (ui.comboBox1->currentIndex() == 1) {         // box
            name = QString::fromAscii(doc->getUniqueObjectName("Box").c_str());
            cmd = QString::fromAscii(
                "App.ActiveDocument.addObject(\"Part::Box\",\"%1\")\n"
                "App.ActiveDocument.%1.Length=%2\n"
                "App.ActiveDocument.%1.Width=%3\n"
                "App.ActiveDocument.%1.Height=%4\n"
                "App.ActiveDocument.%1.Placement=%5\n"
                "App.ActiveDocument.%1.Label='%6'\n")
                .arg(name)
                .arg(ui.boxLength->value(),0,'f',2)
                .arg(ui.boxWidth->value(),0,'f',2)
                .arg(ui.boxHeight->value(),0,'f',2)
                .arg(placement)
                .arg(tr("Box"));
        }
        else if (ui.comboBox1->currentIndex() == 2) {  // cylinder
            name = QString::fromAscii(doc->getUniqueObjectName("Cylinder").c_str());
            cmd = QString::fromAscii(
                "App.ActiveDocument.addObject(\"Part::Cylinder\",\"%1\")\n"
                "App.ActiveDocument.%1.Radius=%2\n"
                "App.ActiveDocument.%1.Height=%3\n"
                "App.ActiveDocument.%1.Angle=%4\n"
                "App.ActiveDocument.%1.Placement=%5\n"
                "App.ActiveDocument.%1.Label='%6'\n")
                .arg(name)
                .arg(ui.cylinderRadius->value(),0,'f',2)
                .arg(ui.cylinderHeight->value(),0,'f',2)
                .arg(ui.cylinderAngle->value(),0,'f',2)
                .arg(placement)
                .arg(tr("Cylinder"));
        }
        else if (ui.comboBox1->currentIndex() == 3) {  // cone
            name = QString::fromAscii(doc->getUniqueObjectName("Cone").c_str());
            cmd = QString::fromAscii(
                "App.ActiveDocument.addObject(\"Part::Cone\",\"%1\")\n"
                "App.ActiveDocument.%1.Radius1=%2\n"
                "App.ActiveDocument.%1.Radius2=%3\n"
                "App.ActiveDocument.%1.Height=%4\n"
                "App.ActiveDocument.%1.Angle=%5\n"
                "App.ActiveDocument.%1.Placement=%6\n"
                "App.ActiveDocument.%1.Label='%7'\n")
                .arg(name)
                .arg(ui.coneRadius1->value(),0,'f',2)
                .arg(ui.coneRadius2->value(),0,'f',2)
                .arg(ui.coneHeight->value(),0,'f',2)
                .arg(ui.coneAngle->value(),0,'f',2)
                .arg(placement)
                .arg(tr("Cone"));
        }
        else if (ui.comboBox1->currentIndex() == 4) {  // sphere
            name = QString::fromAscii(doc->getUniqueObjectName("Sphere").c_str());
            cmd = QString::fromAscii(
                "App.ActiveDocument.addObject(\"Part::Sphere\",\"%1\")\n"
                "App.ActiveDocument.%1.Radius=%2\n"
                "App.ActiveDocument.%1.Angle1=%3\n"
                "App.ActiveDocument.%1.Angle2=%4\n"
                "App.ActiveDocument.%1.Angle3=%5\n"
                "App.ActiveDocument.%1.Placement=%6\n"
                "App.ActiveDocument.%1.Label='%7'\n")
                .arg(name)
                .arg(ui.sphereRadius->value(),0,'f',2)
                .arg(ui.sphereAngle1->value(),0,'f',2)
                .arg(ui.sphereAngle2->value(),0,'f',2)
                .arg(ui.sphereAngle3->value(),0,'f',2)
                .arg(placement)
                .arg(tr("Sphere"));
        }
        else if (ui.comboBox1->currentIndex() == 5) {  // ellipsoid
            name = QString::fromAscii(doc->getUniqueObjectName("Ellipsoid").c_str());
            cmd = QString::fromAscii(
                "App.ActiveDocument.addObject(\"Part::Ellipsoid\",\"%1\")\n"
                "App.ActiveDocument.%1.Radius1=%2\n"
                "App.ActiveDocument.%1.Radius2=%3\n"
                "App.ActiveDocument.%1.Angle1=%4\n"
                "App.ActiveDocument.%1.Angle2=%5\n"
                "App.ActiveDocument.%1.Angle3=%6\n"
                "App.ActiveDocument.%1.Placement=%7\n"
                "App.ActiveDocument.%1.Label='%8'\n")
                .arg(name)
                .arg(ui.ellipsoidRadius1->value(),0,'f',2)
                .arg(ui.ellipsoidRadius2->value(),0,'f',2)
                .arg(ui.ellipsoidAngle1->value(),0,'f',2)
                .arg(ui.ellipsoidAngle2->value(),0,'f',2)
                .arg(ui.ellipsoidAngle3->value(),0,'f',2)
                .arg(placement)
                .arg(tr("Ellipsoid"));
        }
        else if (ui.comboBox1->currentIndex() == 6) {  // torus
            name = QString::fromAscii(doc->getUniqueObjectName("Torus").c_str());
            cmd = QString::fromAscii(
                "App.ActiveDocument.addObject(\"Part::Torus\",\"%1\")\n"
                "App.ActiveDocument.%1.Radius1=%2\n"
                "App.ActiveDocument.%1.Radius2=%3\n"
                "App.ActiveDocument.%1.Angle1=%4\n"
                "App.ActiveDocument.%1.Angle2=%5\n"
                "App.ActiveDocument.%1.Angle3=%6\n"
                "App.ActiveDocument.%1.Placement=%7\n"
                "App.ActiveDocument.%1.Label='%8'\n")
                .arg(name)
                .arg(ui.torusRadius1->value(),0,'f',2)
                .arg(ui.torusRadius2->value(),0,'f',2)
                .arg(ui.torusAngle1->value(),0,'f',2)
                .arg(ui.torusAngle2->value(),0,'f',2)
                .arg(ui.torusAngle3->value(),0,'f',2)
                .arg(placement)
                .arg(tr("Torus"));
        }
        else if (ui.comboBox1->currentIndex() == 7) {  // prism
            name = QString::fromAscii(doc->getUniqueObjectName("Prism").c_str());
            cmd = QString::fromAscii(
                "App.ActiveDocument.addObject(\"Part::Prism\",\"%1\")\n"
                "App.ActiveDocument.%1.Polygon=%2\n"
                "App.ActiveDocument.%1.Circumradius=%3\n"
                "App.ActiveDocument.%1.Height=%4\n"
                "App.ActiveDocument.%1.Placement=%5\n"
                "App.ActiveDocument.%1.Label='%6'\n")
                .arg(name)
                .arg(ui.prismPolygon->value())
                .arg(ui.prismCircumradius->value(),0,'f',2)
                .arg(ui.prismHeight->value(),0,'f',2)
                .arg(placement)
                .arg(tr("Prism"));
        }
        else if (ui.comboBox1->currentIndex() == 8) {  // wedge
            name = QString::fromAscii(doc->getUniqueObjectName("Wedge").c_str());
            cmd = QString::fromAscii(
                "App.ActiveDocument.addObject(\"Part::Wedge\",\"%1\")\n"
                "App.ActiveDocument.%1.Xmin=%2\n"
                "App.ActiveDocument.%1.Ymin=%3\n"
                "App.ActiveDocument.%1.Zmin=%4\n"
                "App.ActiveDocument.%1.X2min=%5\n"
                "App.ActiveDocument.%1.Z2min=%6\n"
                "App.ActiveDocument.%1.Xmax=%7\n"
                "App.ActiveDocument.%1.Ymax=%8\n"
                "App.ActiveDocument.%1.Zmax=%9\n"
                "App.ActiveDocument.%1.X2max=%10\n"
                "App.ActiveDocument.%1.Z2max=%11\n"
                "App.ActiveDocument.%1.Placement=%12\n"
                "App.ActiveDocument.%1.Label='%13'\n")
                .arg(name)
                .arg(ui.wedgeXmin->value(),0,'f',2)
                .arg(ui.wedgeYmin->value(),0,'f',2)
                .arg(ui.wedgeZmin->value(),0,'f',2)
                .arg(ui.wedgeX2min->value(),0,'f',2)
                .arg(ui.wedgeZ2min->value(),0,'f',2)
                .arg(ui.wedgeXmax->value(),0,'f',2)
                .arg(ui.wedgeYmax->value(),0,'f',2)
                .arg(ui.wedgeZmax->value(),0,'f',2)
                .arg(ui.wedgeX2max->value(),0,'f',2)
                .arg(ui.wedgeZ2max->value(),0,'f',2)
                .arg(placement)
                .arg(tr("Wedge"));
        }
        else if (ui.comboBox1->currentIndex() == 9) {  // helix
            name = QString::fromAscii(doc->getUniqueObjectName("Helix").c_str());
            cmd = QString::fromAscii(
                "App.ActiveDocument.addObject(\"Part::Helix\",\"%1\")\n"
                "App.ActiveDocument.%1.Pitch=%2\n"
                "App.ActiveDocument.%1.Height=%3\n"
                "App.ActiveDocument.%1.Radius=%4\n"
                "App.ActiveDocument.%1.Angle=%5\n"
                "App.ActiveDocument.%1.LocalCoord=%6\n"
                "App.ActiveDocument.%1.Style=1\n"
                "App.ActiveDocument.%1.Placement=%7\n"
                "App.ActiveDocument.%1.Label='%8'\n")
                .arg(name)
                .arg(ui.helixPitch->value(),0,'f',2)
                .arg(ui.helixHeight->value(),0,'f',2)
                .arg(ui.helixRadius->value(),0,'f',2)
                .arg(ui.helixAngle->value(),0,'f',2)
                .arg(ui.helixLocalCS->currentIndex())
                .arg(placement)
                .arg(tr("Helix"));
        }
        else if (ui.comboBox1->currentIndex() == 10) {  // spiral
            name = QString::fromAscii(doc->getUniqueObjectName("Spiral").c_str());
            cmd = QString::fromAscii(
                "App.ActiveDocument.addObject(\"Part::Spiral\",\"%1\")\n"
                "App.ActiveDocument.%1.Growth=%2\n"
                "App.ActiveDocument.%1.Rotations=%3\n"
                "App.ActiveDocument.%1.Radius=%4\n"
                "App.ActiveDocument.%1.Placement=%5\n"
                "App.ActiveDocument.%1.Label='%6'\n")
                .arg(name)
                .arg(ui.spiralGrowth->value(),0,'f',2)
                .arg(ui.spiralRotation->value(),0,'f',2)
                .arg(ui.spiralRadius->value(),0,'f',2)
                .arg(placement)
                .arg(tr("Spiral"));
        }
        else if (ui.comboBox1->currentIndex() == 11) {  // circle
            name = QString::fromAscii(doc->getUniqueObjectName("Circle").c_str());
            cmd = QString::fromAscii(
                "App.ActiveDocument.addObject(\"Part::Circle\",\"%1\")\n"
                "App.ActiveDocument.%1.Radius=%2\n"
                "App.ActiveDocument.%1.Angle0=%3\n"
                "App.ActiveDocument.%1.Angle1=%4\n"
                "App.ActiveDocument.%1.Placement=%5\n"
                "App.ActiveDocument.%1.Label='%6'\n")
                .arg(name)
                .arg(ui.circleRadius->value(),0,'f',2)
                .arg(ui.circleAngle0->value(),0,'f',2)
                .arg(ui.circleAngle1->value(),0,'f',2)
                .arg(placement)
                .arg(tr("Circle"));
        }
        else if (ui.comboBox1->currentIndex() == 12) {  // ellipse
            name = QString::fromAscii(doc->getUniqueObjectName("Ellipse").c_str());
            cmd = QString::fromAscii(
                "App.ActiveDocument.addObject(\"Part::Ellipse\",\"%1\")\n"
                "App.ActiveDocument.%1.MajorRadius=%2\n"
                "App.ActiveDocument.%1.MinorRadius=%3\n"
                "App.ActiveDocument.%1.Angle0=%4\n"
                "App.ActiveDocument.%1.Angle1=%5\n"
                "App.ActiveDocument.%1.Placement=%6\n"
                "App.ActiveDocument.%1.Label='%7'\n")
                .arg(name)
                .arg(ui.ellipseMajorRadius->value(),0,'f',2)
                .arg(ui.ellipseMinorRadius->value(),0,'f',2)
                .arg(ui.ellipseAngle0->value(),0,'f',2)
                .arg(ui.ellipseAngle1->value(),0,'f',2)
                .arg(placement)
                .arg(tr("Ellipse"));
        }
        else if (ui.comboBox1->currentIndex() == 13) {  // vertex
            name = QString::fromAscii(doc->getUniqueObjectName("Vertex").c_str());
            cmd = QString::fromAscii(
                "App.ActiveDocument.addObject(\"Part::Vertex\",\"%1\")\n"
                "App.ActiveDocument.%1.X=%2\n"
                "App.ActiveDocument.%1.Y=%3\n"
                "App.ActiveDocument.%1.Z=%4\n"
                "App.ActiveDocument.%1.Placement=%5\n"
                "App.ActiveDocument.%1.Label='%6'\n")
                .arg(name)
                .arg(ui.vertexX->value(),0,'f',2)
                .arg(ui.vertexY->value(),0,'f',2)
                .arg(ui.vertexZ->value(),0,'f',2)
                .arg(placement)
                .arg(tr("Vertex"));
        }
        else if (ui.comboBox1->currentIndex() == 14) {  // line
            name = QString::fromAscii(doc->getUniqueObjectName("Line").c_str());
            cmd = QString::fromAscii(
                "App.ActiveDocument.addObject(\"Part::Line\",\"%1\")\n"
                "App.ActiveDocument.%1.X1=%2\n"
                "App.ActiveDocument.%1.Y1=%3\n"
                "App.ActiveDocument.%1.Z1=%4\n"
                "App.ActiveDocument.%1.X2=%5\n"
                "App.ActiveDocument.%1.Y2=%6\n"
                "App.ActiveDocument.%1.Z2=%7\n"
                "App.ActiveDocument.%1.Placement=%8\n"
                "App.ActiveDocument.%1.Label='%9'\n")
                .arg(name)
                .arg(ui.edgeX1->value(),0,'f',2)
                .arg(ui.edgeY1->value(),0,'f',2)
                .arg(ui.edgeZ1->value(),0,'f',2)
                .arg(ui.edgeX2->value(),0,'f',2)
                .arg(ui.edgeY2->value(),0,'f',2)
                .arg(ui.edgeZ2->value(),0,'f',2)
                .arg(placement)
                .arg(tr("Line"));
        }
        else if (ui.comboBox1->currentIndex() == 15) {  // RegularPolygon
            name = QString::fromAscii(doc->getUniqueObjectName("RegularPolygon").c_str());
            cmd = QString::fromAscii(
                "App.ActiveDocument.addObject(\"Part::RegularPolygon\",\"%1\")\n"
                "App.ActiveDocument.%1.Polygon=%2\n"
                "App.ActiveDocument.%1.Circumradius=%3\n"
                "App.ActiveDocument.%1.Placement=%4\n"
                "App.ActiveDocument.%1.Label='%5'\n")
                .arg(name)
                .arg(ui.regularPolygonPolygon->value())
                .arg(ui.regularPolygonCircumradius->value(),0,'f',2)
                .arg(placement)
                .arg(tr("Regular polygon"));
        }

        // Execute the Python block
        QString prim = tr("Create %1").arg(ui.comboBox1->currentText());
        Gui::Application::Instance->activeDocument()->openCommand(prim.toUtf8());
        Gui::Command::doCommand(Gui::Command::Doc, (const char*)cmd.toAscii());
        Gui::Application::Instance->activeDocument()->commitCommand();
        Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.recompute()");
        Gui::Command::doCommand(Gui::Command::Gui, "Gui.SendMsgToActiveView(\"ViewFit\")");
    }
    catch (const Base::PyException& e) {
        QMessageBox::warning(this, tr("Create %1")
            .arg(ui.comboBox1->currentText()), QString::fromLatin1(e.what()));
    }
}

// ----------------------------------------------

/* TRANSLATOR PartGui::Location */

Location::Location(QWidget* parent)
{
    ui.setupUi(this);
}

Location::~Location()
{
    // no need to delete child widgets, Qt does it all for us
    if (!this->activeView.isNull()) {
        Gui::View3DInventorViewer* viewer = static_cast<Gui::View3DInventor*>
            (this->activeView.data())->getViewer();
        viewer->setEditing(false);
        viewer->setRedirectToSceneGraph(false);
        viewer->removeEventCallback(SoMouseButtonEvent::getClassTypeId(), pickCallback,this);
        SoNode* root = viewer->getSceneGraph();
        if (root && root->getTypeId().isDerivedFrom(Gui::SoFCUnifiedSelection::getClassTypeId()))
            static_cast<Gui::SoFCUnifiedSelection*>(root)->selectionMode.setValue(this->mode);
    }
}

void Location::on_viewPositionButton_clicked()
{
    Gui::Document* doc = Gui::Application::Instance->activeDocument();
    if (!doc) {
        return;
    }

    Gui::View3DInventor* view = static_cast<Gui::View3DInventor*>(doc->getActiveView());
    if (view && !this->activeView) {
        Gui::View3DInventorViewer* viewer = view->getViewer();
        if (!viewer->isEditing()) {
            this->activeView = view;
            viewer->setEditing(true);
            viewer->setRedirectToSceneGraph(true);
            viewer->addEventCallback(SoMouseButtonEvent::getClassTypeId(), pickCallback, this);
            SoNode* root = viewer->getSceneGraph();
            if (root && root->getTypeId().isDerivedFrom(Gui::SoFCUnifiedSelection::getClassTypeId())) {
                this->mode = static_cast<Gui::SoFCUnifiedSelection*>(root)->selectionMode.getValue();
                static_cast<Gui::SoFCUnifiedSelection*>(root)->selectionMode.setValue(Gui::SoFCUnifiedSelection::OFF);
            }
        }
     }
}

void Location::pickCallback(void * ud, SoEventCallback * n)
{
    const SoMouseButtonEvent * mbe = static_cast<const SoMouseButtonEvent*>(n->getEvent());
    Gui::View3DInventorViewer* view  = reinterpret_cast<Gui::View3DInventorViewer*>(n->getUserData());

    // Mark all incoming mouse button events as handled, especially, to deactivate the selection node
    n->getAction()->setHandled();
    if (mbe->getButton() == SoMouseButtonEvent::BUTTON1) {
        if (mbe->getState() == SoButtonEvent::DOWN) {
            const SoPickedPoint * point = n->getPickedPoint();
            if (point) {
                SbVec3f pnt = point->getPoint();
                SbVec3f nor = point->getNormal();
                Location* dlg = reinterpret_cast<Location*>(ud);
                dlg->ui.loc->setPosition(Base::Vector3d(pnt[0],pnt[1],pnt[2]));
                dlg->ui.loc->setDirection(Base::Vector3d(nor[0],nor[1],nor[2]));
                n->setHandled();
            }
        }
    }
    else if (mbe->getButton() == SoMouseButtonEvent::BUTTON2) {
        if (mbe->getState() == SoButtonEvent::UP) {
            n->setHandled();
            view->setEditing(false);
            view->setRedirectToSceneGraph(false);
            Location* dlg = reinterpret_cast<Location*>(ud);
            dlg->activeView = 0;
            view->removeEventCallback(SoMouseButtonEvent::getClassTypeId(), pickCallback,ud);
            SoNode* root = view->getSceneGraph();
            if (root && root->getTypeId().isDerivedFrom(Gui::SoFCUnifiedSelection::getClassTypeId()))
                static_cast<Gui::SoFCUnifiedSelection*>(root)->selectionMode.setValue(static_cast<Location*>(ud)->mode);
        }
    }
}

QString Location::toPlacement() const
{
    Base::Vector3d d = ui.loc->getDirection();
    gp_Dir dir = gp_Dir(d.x,d.y,d.z);
    gp_Pnt pnt = gp_Pnt(0.0,0.0,0.0);
    gp_Ax3 ax3;

    double cosNX = dir.Dot(gp::DX());
    double cosNY = dir.Dot(gp::DY());
    double cosNZ = dir.Dot(gp::DZ());
    std::vector<double> cosXYZ;
    cosXYZ.push_back(fabs(cosNX));
    cosXYZ.push_back(fabs(cosNY));
    cosXYZ.push_back(fabs(cosNZ));

    int pos = std::max_element(cosXYZ.begin(), cosXYZ.end()) - cosXYZ.begin();

    // +X/-X
    if (pos == 0) {
        if (cosNX > 0)
            ax3 = gp_Ax3(pnt, dir, gp_Dir(0,1,0));
        else
            ax3 = gp_Ax3(pnt, dir, gp_Dir(0,-1,0));
    }
    // +Y/-Y
    else if (pos == 1) {
        if (cosNY > 0)
            ax3 = gp_Ax3(pnt, dir, gp_Dir(0,0,1));
        else
            ax3 = gp_Ax3(pnt, dir, gp_Dir(0,0,-1));
    }
    // +Z/-Z
    else {
        ax3 = gp_Ax3(pnt, dir, gp_Dir(1,0,0));
    }

    gp_Trsf Trf;
    Trf.SetTransformation(ax3);
    Trf.Invert();

    gp_XYZ theAxis(0,0,1);
    Standard_Real theAngle = 0.0;
    Trf.GetRotation(theAxis,theAngle);

    Base::Rotation rot(Base::convertTo<Base::Vector3d>(theAxis), theAngle);
    Base::Vector3d loc = ui.loc->getPosition();

    return QString::fromAscii("Base.Placement(Base.Vector(%1,%2,%3),Base.Rotation(%4,%5,%6,%7))")
        .arg(loc.x,0,'f',2)
        .arg(loc.y,0,'f',2)
        .arg(loc.z,0,'f',2)
        .arg(rot[0],0,'f',2)
        .arg(rot[1],0,'f',2)
        .arg(rot[2],0,'f',2)
        .arg(rot[3],0,'f',2);
}

// ----------------------------------------------

/* TRANSLATOR PartGui::TaskPrimitives */

TaskPrimitives::TaskPrimitives()
{
    Gui::TaskView::TaskBox* taskbox;
    widget = new DlgPrimitives();
    taskbox = new Gui::TaskView::TaskBox(QPixmap(), widget->windowTitle(),true, 0);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);

    location = new Location();
    taskbox = new Gui::TaskView::TaskBox(QPixmap(), location->windowTitle(),true, 0);
    taskbox->groupLayout()->addWidget(location);
    taskbox->hideGroupBox();
    Content.push_back(taskbox);
}

TaskPrimitives::~TaskPrimitives()
{
    // automatically deleted in the sub-class
}

QDialogButtonBox::StandardButtons TaskPrimitives::getStandardButtons() const
{ 
    return QDialogButtonBox::Close|
           QDialogButtonBox::Ok;
}

void TaskPrimitives::modifyStandardButtons(QDialogButtonBox* box)
{
    QPushButton* btn = box->button(QDialogButtonBox::Ok);
    btn->setText(QApplication::translate("PartGui::DlgPrimitives", "&Create"));
}

bool TaskPrimitives::accept()
{
    widget->createPrimitive(location->toPlacement());
    return false;
}

bool TaskPrimitives::reject()
{
    return true;
}

#include "moc_DlgPrimitives.cpp"
