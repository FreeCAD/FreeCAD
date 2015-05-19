/***************************************************************************
 *   Copyright (c) 2015 Stefan Tröger <stefantroeger@gmx.net>              *
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
# include <sstream>
# include <QRegExp>
# include <QTextStream>
# include <QMessageBox>
# include <Precision.hxx>
#endif

#include "TaskPrimitiveParameters.h"
#include "ViewProviderDatumCS.h"
#include <Mod/PartDesign/App/FeaturePrimitive.h>
#include <Mod/PartDesign/App/DatumCS.h>
#include <Mod/Part/App/DatumFeature.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/Command.h>
#include <Gui/BitmapFactory.h>

using namespace PartDesignGui;

TaskBoxPrimitives::TaskBoxPrimitives(PartDesign::FeaturePrimitive::Type t, QWidget* parent)
  : TaskBox(QPixmap(),tr("Primitive parameters"), true, parent)
{
    proxy = new QWidget(this);
    ui.setupUi(proxy);
    
    // set limits
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
    ui.ellipsoidRadius3->setMaximum(INT_MAX);
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
    
    this->groupLayout()->addWidget(proxy);
    
    int index = 0;
    switch(t) {
        
        case PartDesign::FeaturePrimitive::Box:
            index = 1;
            break;
        case PartDesign::FeaturePrimitive::Cylinder:
            index = 2;
            break;
        case PartDesign::FeaturePrimitive::Sphere:
            index = 4;
            break;
    }
    
    ui.widgetStack->setCurrentIndex(index);
    ui.widgetStack->setMinimumSize(ui.widgetStack->widget(index)->minimumSize());
    for(int i=0; i<ui.widgetStack->count(); ++i) {
    
        if(i != index)
            ui.widgetStack->widget(i)->setSizePolicy(QSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored));
    }
}

/*  
 *  Destroys the object and frees any allocated resources
 */
TaskBoxPrimitives::~TaskBoxPrimitives()
{
}

/*
void  TaskBoxPrimitives::createPrimitive(const QString& placement)
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
                .arg(ui.planeLength->value().getValue(),0,'f',2)
                .arg(ui.planeWidth->value().getValue(),0,'f',2)
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
                .arg(ui.boxLength->value().getValue(),0,'f',2)
                .arg(ui.boxWidth->value().getValue(),0,'f',2)
                .arg(ui.boxHeight->value().getValue(),0,'f',2)
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
                .arg(ui.cylinderRadius->value().getValue(),0,'f',2)
                .arg(ui.cylinderHeight->value().getValue(),0,'f',2)
                .arg(ui.cylinderAngle->value().getValue(),0,'f',2)
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
                .arg(ui.coneRadius1->value().getValue(),0,'f',2)
                .arg(ui.coneRadius2->value().getValue(),0,'f',2)
                .arg(ui.coneHeight->value().getValue(),0,'f',2)
                .arg(ui.coneAngle->value().getValue(),0,'f',2)
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
                .arg(ui.sphereRadius->value().getValue(),0,'f',2)
                .arg(ui.sphereAngle1->value().getValue(),0,'f',2)
                .arg(ui.sphereAngle2->value().getValue(),0,'f',2)
                .arg(ui.sphereAngle3->value().getValue(),0,'f',2)
                .arg(placement)
                .arg(tr("Sphere"));
        }
        else if (ui.comboBox1->currentIndex() == 5) {  // ellipsoid
            name = QString::fromAscii(doc->getUniqueObjectName("Ellipsoid").c_str());
            cmd = QString::fromAscii(
                "App.ActiveDocument.addObject(\"Part::Ellipsoid\",\"%1\")\n"
                "App.ActiveDocument.%1.Radius1=%2\n"
                "App.ActiveDocument.%1.Radius2=%3\n"
                "App.ActiveDocument.%1.Radius3=%4\n"
                "App.ActiveDocument.%1.Angle1=%5\n"
                "App.ActiveDocument.%1.Angle2=%6\n"
                "App.ActiveDocument.%1.Angle3=%7\n"
                "App.ActiveDocument.%1.Placement=%8\n"
                "App.ActiveDocument.%1.Label='%9'\n")
                .arg(name)
                .arg(ui.ellipsoidRadius1->value().getValue(),0,'f',2)
                .arg(ui.ellipsoidRadius2->value().getValue(),0,'f',2)
                .arg(ui.ellipsoidRadius3->value().getValue(),0,'f',2)
                .arg(ui.ellipsoidAngle1->value().getValue(),0,'f',2)
                .arg(ui.ellipsoidAngle2->value().getValue(),0,'f',2)
                .arg(ui.ellipsoidAngle3->value().getValue(),0,'f',2)
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
                .arg(ui.torusRadius1->value().getValue(),0,'f',2)
                .arg(ui.torusRadius2->value().getValue(),0,'f',2)
                .arg(ui.torusAngle1->value().getValue(),0,'f',2)
                .arg(ui.torusAngle2->value().getValue(),0,'f',2)
                .arg(ui.torusAngle3->value().getValue(),0,'f',2)
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
                .arg(ui.prismCircumradius->value().getValue(),0,'f',2)
                .arg(ui.prismHeight->value().getValue(),0,'f',2)
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
                .arg(ui.wedgeXmin->value().getValue(),0,'f',2)
                .arg(ui.wedgeYmin->value().getValue(),0,'f',2)
                .arg(ui.wedgeZmin->value().getValue(),0,'f',2)
                .arg(ui.wedgeX2min->value().getValue(),0,'f',2)
                .arg(ui.wedgeZ2min->value().getValue(),0,'f',2)
                .arg(ui.wedgeXmax->value().getValue(),0,'f',2)
                .arg(ui.wedgeYmax->value().getValue(),0,'f',2)
                .arg(ui.wedgeZmax->value().getValue(),0,'f',2)
                .arg(ui.wedgeX2max->value().getValue(),0,'f',2)
                .arg(ui.wedgeZ2max->value().getValue(),0,'f',2)
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
                .arg(ui.helixPitch->value().getValue(),0,'f',2)
                .arg(ui.helixHeight->value().getValue(),0,'f',2)
                .arg(ui.helixRadius->value().getValue(),0,'f',2)
                .arg(ui.helixAngle->value().getValue(),0,'f',2)
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
                .arg(ui.spiralGrowth->value().getValue(),0,'f',2)
                .arg(ui.spiralRotation->value(),0,'f',2)
                .arg(ui.spiralRadius->value().getValue(),0,'f',2)
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
                .arg(ui.circleRadius->value().getValue(),0,'f',2)
                .arg(ui.circleAngle0->value().getValue(),0,'f',2)
                .arg(ui.circleAngle1->value().getValue(),0,'f',2)
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
                .arg(ui.ellipseMajorRadius->value().getValue(),0,'f',2)
                .arg(ui.ellipseMinorRadius->value().getValue(),0,'f',2)
                .arg(ui.ellipseAngle0->value().getValue(),0,'f',2)
                .arg(ui.ellipseAngle1->value().getValue(),0,'f',2)
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
                .arg(ui.vertexX->value().getValue(),0,'f',2)
                .arg(ui.vertexY->value().getValue(),0,'f',2)
                .arg(ui.vertexZ->value().getValue(),0,'f',2)
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
                .arg(ui.edgeX1->value().getValue(),0,'f',2)
                .arg(ui.edgeY1->value().getValue(),0,'f',2)
                .arg(ui.edgeZ1->value().getValue(),0,'f',2)
                .arg(ui.edgeX2->value().getValue(),0,'f',2)
                .arg(ui.edgeY2->value().getValue(),0,'f',2)
                .arg(ui.edgeZ2->value().getValue(),0,'f',2)
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
                .arg(ui.regularPolygonCircumradius->value().getValue(),0,'f',2)
                .arg(placement)
                .arg(tr("Regular polygon"));
        }

        // Execute the Python block
        QString prim = tr("Create %1").arg(ui.comboBox1->currentText());
        Gui::Application::Instance->activeDocument()->openCommand(prim.toUtf8());
        Gui::Command::doCommand(Gui::Command::Doc, (const char*)cmd.toUtf8());
        Gui::Application::Instance->activeDocument()->commitCommand();
        Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.recompute()");
        Gui::Command::doCommand(Gui::Command::Gui, "Gui.SendMsgToActiveView(\"ViewFit\")");
    }
    catch (const Base::PyException& e) {
        QMessageBox::warning(this, tr("Create %1")
            .arg(ui.comboBox1->currentText()), QString::fromLatin1(e.what()));
    }
}*/

TaskPrimitiveParameters::TaskPrimitiveParameters(ViewProviderPrimitive* PrimitiveView)
{
    
    assert(PrimitiveView);
    
    PartDesign::FeaturePrimitive* prm = static_cast<PartDesign::FeaturePrimitive*>(PrimitiveView->getObject());
    cs  = static_cast<PartDesign::CoordinateSystem*>(prm->CoordinateSystem.getValue());
    
    //if no coordinate system exist we need to add one, it is highly important that it exists!
    if(!cs) {
        std::string CSName = App::GetApplication().getActiveDocument()->getUniqueObjectName("CoordinateSystem");
        cs = static_cast<PartDesign::CoordinateSystem*>(
                App::GetApplication().getActiveDocument()->addObject("PartDesign::CoordinateSystem", CSName.c_str()));
        prm->CoordinateSystem.setValue(cs);
    }
            
    ViewProviderDatumCoordinateSystem* vp = static_cast<ViewProviderDatumCoordinateSystem*>(
            Gui::Application::Instance->activeDocument()->getViewProvider(cs)); 
    
    assert(vp);    
    cs_visibility = vp->isVisible();
    vp->Visibility.setValue(true);
    parameter  = new TaskDatumParameters(vp);
    Content.push_back(parameter);
    
    primitive = new TaskBoxPrimitives(prm->getPrimitiveType());
    Content.push_back(primitive);
}

TaskPrimitiveParameters::~TaskPrimitiveParameters()
{
    ViewProviderDatumCoordinateSystem* vp = static_cast<ViewProviderDatumCoordinateSystem*>(
            Gui::Application::Instance->activeDocument()->getViewProvider(cs)); 
    vp->setVisible(cs_visibility);
}

bool TaskPrimitiveParameters::accept()
{
    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.recompute()");
    Gui::Command::doCommand(Gui::Command::Gui,"Gui.activeDocument().resetEdit()");
        
    return true;
}

bool TaskPrimitiveParameters::reject() {
    
    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.recompute()");
    Gui::Command::doCommand(Gui::Command::Gui,"Gui.activeDocument().resetEdit()");
    return true;
}

QDialogButtonBox::StandardButtons TaskPrimitiveParameters::getStandardButtons(void) const {
    return Gui::TaskView::TaskDialog::getStandardButtons();
}


#include "moc_TaskPrimitiveParameters.cpp"