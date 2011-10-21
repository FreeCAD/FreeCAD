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
#include <Inventor/SoPickedPoint.h>
#include <Inventor/events/SoMouseButtonEvent.h>
#endif

#include <Base/Interpreter.h>
#include <Base/Rotation.h>
#include <App/Application.h>
#include <App/Document.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/Command.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>

#include "DlgPrimitives.h"

using namespace PartGui;

/* TRANSLATOR PartGui::DlgPrimitives */

DlgPrimitives::DlgPrimitives(QWidget* parent, Qt::WFlags fl)
  : Gui::LocationDialogComp<Ui_DlgPrimitives>(parent, fl)
{
    Gui::Command::doCommand(Gui::Command::Doc, "from FreeCAD import Base");
    Gui::Command::doCommand(Gui::Command::Doc, "import Part,PartGui");

    connect(ui.viewPositionButton, SIGNAL(clicked()),
            this, SLOT(on_viewPositionButton_clicked()));

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
    ui.VertexXAxisValue->setMaximum(INT_MAX);
    ui.VertexYAxisValue->setMaximum(INT_MAX);
    ui.VertexZAxisValue->setMaximum(INT_MAX);
    ui.VertexXAxisValue->setMinimum(-INT_MAX);
    ui.VertexYAxisValue->setMinimum(-INT_MAX);
    ui.VertexZAxisValue->setMinimum(-INT_MAX);
}

/*  
 *  Destroys the object and frees any allocated resources
 */
DlgPrimitives::~DlgPrimitives()
{
    // no need to delete child widgets, Qt does it all for us
    if (!this->activeView.isNull()) {
        Gui::View3DInventorViewer* viewer = static_cast<Gui::View3DInventor*>
            (this->activeView.data())->getViewer();
        viewer->setEditing(false);
        viewer->setRedirectToSceneGraph(false);
        viewer->removeEventCallback(SoMouseButtonEvent::getClassTypeId(), pickCallback,this);
    }
}

QString DlgPrimitives::toPlacement() const
{
    Base::Vector3f d = ui.getDirection();
    Base::Rotation rot(Base::Vector3d(0.0,0.0,1.0),
                       Base::Vector3d(d.x,d.y,d.z));
    return QString::fromAscii("Base.Placement(Base.Vector(%1,%2,%3),Base.Rotation(%4,%5,%6,%7))")
        .arg(ui.xPos->value(),0,'f',2)
        .arg(ui.yPos->value(),0,'f',2)
        .arg(ui.zPos->value(),0,'f',2)
        .arg(rot[0],0,'f',2)
        .arg(rot[1],0,'f',2)
        .arg(rot[2],0,'f',2)
        .arg(rot[3],0,'f',2);
}

void DlgPrimitives::pickCallback(void * ud, SoEventCallback * n)
{
    const SoMouseButtonEvent * mbe = static_cast<const SoMouseButtonEvent*>(n->getEvent());
    Gui::View3DInventorViewer* view  = reinterpret_cast<Gui::View3DInventorViewer*>(n->getUserData());

    // Mark all incoming mouse button events as handled, especially, to deactivate the selection node
    n->getAction()->setHandled();
    if (mbe->getButton() == SoMouseButtonEvent::BUTTON1) {
        if (mbe->getState() == SoButtonEvent::UP) {
            n->setHandled();
            view->setEditing(false);
            view->setRedirectToSceneGraph(false);
            DlgPrimitives* dlg = reinterpret_cast<DlgPrimitives*>(ud);
            dlg->activeView = 0;
            view->removeEventCallback(SoMouseButtonEvent::getClassTypeId(), pickCallback,ud);
        }
        else if (mbe->getState() == SoButtonEvent::DOWN) {
            const SoPickedPoint * point = n->getPickedPoint();
            if (point) {
                SbVec3f pnt = point->getPoint();
                SbVec3f nor = point->getNormal();
                DlgPrimitives* dlg = reinterpret_cast<DlgPrimitives*>(ud);
                dlg->ui.xPos->setValue(pnt[0]);
                dlg->ui.yPos->setValue(pnt[1]);
                dlg->ui.zPos->setValue(pnt[2]);
                dlg->ui.setDirection(Base::Vector3f(nor[0],nor[1],nor[2]));
                n->setHandled();
            }
        }
    }
}

void DlgPrimitives::on_viewPositionButton_clicked()
{
    Gui::Document* doc = Gui::Application::Instance->activeDocument();
    if (!doc) {
        QMessageBox::warning(this, tr("Create %1")
            .arg(ui.comboBox1->currentText()), tr("No active document"));
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
        }
     }
}

void DlgPrimitives::accept()
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
                "App.ActiveDocument.%1.Placement=%4\n")
                .arg(name)
                .arg(ui.planeLength->value(),0,'f',2)
                .arg(ui.planeWidth->value(),0,'f',2)
                .arg(this->toPlacement());
        }
        else if (ui.comboBox1->currentIndex() == 1) {         // box
            name = QString::fromAscii(doc->getUniqueObjectName("Box").c_str());
            cmd = QString::fromAscii(
                "App.ActiveDocument.addObject(\"Part::Box\",\"%1\")\n"
                "App.ActiveDocument.%1.Length=%2\n"
                "App.ActiveDocument.%1.Width=%3\n"
                "App.ActiveDocument.%1.Height=%4\n"
                "App.ActiveDocument.%1.Placement=%5\n")
                .arg(name)
                .arg(ui.boxLength->value(),0,'f',2)
                .arg(ui.boxWidth->value(),0,'f',2)
                .arg(ui.boxHeight->value(),0,'f',2)
                .arg(this->toPlacement());
        }
        else if (ui.comboBox1->currentIndex() == 2) {  // cylinder
            name = QString::fromAscii(doc->getUniqueObjectName("Cylinder").c_str());
            cmd = QString::fromAscii(
                "App.ActiveDocument.addObject(\"Part::Cylinder\",\"%1\")\n"
                "App.ActiveDocument.%1.Radius=%2\n"
                "App.ActiveDocument.%1.Height=%3\n"
                "App.ActiveDocument.%1.Angle=%4\n"
                "App.ActiveDocument.%1.Placement=%5\n")
                .arg(name)
                .arg(ui.cylinderRadius->value(),0,'f',2)
                .arg(ui.cylinderHeight->value(),0,'f',2)
                .arg(ui.cylinderAngle->value(),0,'f',2)
                .arg(this->toPlacement());
        }
        else if (ui.comboBox1->currentIndex() == 3) {  // cone
            name = QString::fromAscii(doc->getUniqueObjectName("Cone").c_str());
            cmd = QString::fromAscii(
                "App.ActiveDocument.addObject(\"Part::Cone\",\"%1\")\n"
                "App.ActiveDocument.%1.Radius1=%2\n"
                "App.ActiveDocument.%1.Radius2=%3\n"
                "App.ActiveDocument.%1.Height=%4\n"
                "App.ActiveDocument.%1.Angle=%5\n"
                "App.ActiveDocument.%1.Placement=%6\n")
                .arg(name)
                .arg(ui.coneRadius1->value(),0,'f',2)
                .arg(ui.coneRadius2->value(),0,'f',2)
                .arg(ui.coneHeight->value(),0,'f',2)
                .arg(ui.coneAngle->value(),0,'f',2)
                .arg(this->toPlacement());
        }
        else if (ui.comboBox1->currentIndex() == 4) {  // sphere
            name = QString::fromAscii(doc->getUniqueObjectName("Sphere").c_str());
            cmd = QString::fromAscii(
                "App.ActiveDocument.addObject(\"Part::Sphere\",\"%1\")\n"
                "App.ActiveDocument.%1.Radius=%2\n"
                "App.ActiveDocument.%1.Angle1=%3\n"
                "App.ActiveDocument.%1.Angle2=%4\n"
                "App.ActiveDocument.%1.Angle3=%5\n"
                "App.ActiveDocument.%1.Placement=%6\n")
                .arg(name)
                .arg(ui.sphereRadius->value(),0,'f',2)
                .arg(ui.sphereAngle1->value(),0,'f',2)
                .arg(ui.sphereAngle2->value(),0,'f',2)
                .arg(ui.sphereAngle3->value(),0,'f',2)
                .arg(this->toPlacement());
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
                "App.ActiveDocument.%1.Placement=%7\n")
                .arg(name)
                .arg(ui.ellipsoidRadius1->value(),0,'f',2)
                .arg(ui.ellipsoidRadius2->value(),0,'f',2)
                .arg(ui.ellipsoidAngle1->value(),0,'f',2)
                .arg(ui.ellipsoidAngle2->value(),0,'f',2)
                .arg(ui.ellipsoidAngle3->value(),0,'f',2)
                .arg(this->toPlacement());
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
                "App.ActiveDocument.%1.Placement=%7\n")
                .arg(name)
                .arg(ui.torusRadius1->value(),0,'f',2)
                .arg(ui.torusRadius2->value(),0,'f',2)
                .arg(ui.torusAngle1->value(),0,'f',2)
                .arg(ui.torusAngle2->value(),0,'f',2)
                .arg(ui.torusAngle3->value(),0,'f',2)
                .arg(this->toPlacement());
        }
        else if (ui.comboBox1->currentIndex() == 7) {  // wedge
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
                "App.ActiveDocument.%1.Placement=%12\n")
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
                .arg(this->toPlacement());
        }
        else if (ui.comboBox1->currentIndex() == 8) {  // helix
            name = QString::fromAscii(doc->getUniqueObjectName("Helix").c_str());
            cmd = QString::fromAscii(
                "App.ActiveDocument.addObject(\"Part::Helix\",\"%1\")\n"
                "App.ActiveDocument.%1.Pitch=%2\n"
                "App.ActiveDocument.%1.Height=%3\n"
                "App.ActiveDocument.%1.Radius=%4\n"
                "App.ActiveDocument.%1.Angle=%5\n"
                "App.ActiveDocument.%1.Placement=%6\n")
                .arg(name)
                .arg(ui.helixPitch->value(),0,'f',2)
                .arg(ui.helixHeight->value(),0,'f',2)
                .arg(ui.helixRadius->value(),0,'f',2)
                .arg(ui.helixAngle->value(),0,'f',2)
                .arg(this->toPlacement());
        }
        else if (ui.comboBox1->currentIndex() == 9) {  // circle
            name = QString::fromAscii(doc->getUniqueObjectName("Circle").c_str());
            cmd = QString::fromAscii(
                "App.ActiveDocument.addObject(\"Part::Circle\",\"%1\")\n"
                "App.ActiveDocument.%1.Radius=%2\n"
                "App.ActiveDocument.%1.Angle0=%3\n"
                "App.ActiveDocument.%1.Angle1=%4\n"
                "App.ActiveDocument.%1.Placement=%5\n")
                .arg(name)
                .arg(ui.circleRadius->value(),0,'f',2)
                .arg(ui.circleAngle0->value(),0,'f',2)
                .arg(ui.circleAngle1->value(),0,'f',2)
                .arg(this->toPlacement());
        }
        else if (ui.comboBox1->currentIndex() == 10) {  // vertex
            name = QString::fromAscii(doc->getUniqueObjectName("Vertex").c_str());
            cmd = QString::fromAscii(
                "App.ActiveDocument.addObject(\"Part::Vertex\",\"%1\")\n"
                "App.ActiveDocument.%1.X=%2\n"
                "App.ActiveDocument.%1.Y=%3\n"
                "App.ActiveDocument.%1.Z=%4\n"
                "App.ActiveDocument.%1.Placement=%5\n")
                .arg(name)
                .arg(ui.VertexXAxisValue->value(),0,'f',2)
                .arg(ui.VertexYAxisValue->value(),0,'f',2)
                .arg(ui.VertexZAxisValue->value(),0,'f',2)
                .arg(this->toPlacement());
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

#include "moc_DlgPrimitives.cpp"
