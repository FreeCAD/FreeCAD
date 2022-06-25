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
#include <gp_Ax1.hxx>
#include <gp_Ax3.hxx>
#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>
#include <GC_MakeArcOfCircle.hxx>
#include <Geom_Circle.hxx>
#include <Geom_TrimmedCurve.hxx>
#include <QMessageBox>
#include <QSignalMapper>
#include <Inventor/SoPickedPoint.h>
#include <Inventor/events/SoMouseButtonEvent.h>
#endif

#include <Base/Interpreter.h>
#include <Base/Rotation.h>
#include <Base/Tools.h>
#include <Base/UnitsApi.h>
#include <App/Application.h>
#include <App/Part.h>
#include <App/Document.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/Command.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
#include <Gui/SoFCUnifiedSelection.h>
#include <Gui/TaskView/TaskView.h>
#include <Mod/Part/App/PrimitiveFeature.h>
#include <Mod/Part/App/FeaturePartBox.h>
#include <Mod/Part/App/FeaturePartCircle.h>
#include <Mod/Part/App/Tools.h>

#include "DlgPrimitives.h"
#include "ui_DlgPrimitives.h"
#include "ui_Location.h"

using namespace PartGui;

namespace PartGui {

    QString getAutoGroupCommandStr(QString objectName)
        // Helper function to get the python code to add the newly created object to the active Part object if present
    {
        App::Part* activePart = Gui::Application::Instance->activeView()->getActiveObject<App::Part*>("part");
        if (activePart) {
            QString activeObjectName = QString::fromLatin1(activePart->getNameInDocument());
            return QString::fromLatin1("App.ActiveDocument.getObject('%1\')."
                "addObject(App.ActiveDocument.getObject('%2\'))\n")
                .arg(activeObjectName)
                .arg(objectName);
        }
        return QString::fromLatin1("# Object %1 created at document root").arg(objectName);
    }

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
        Gui::Command::runCommand(Gui::Command::Doc, cmd.toLatin1());
        doc->commitCommand();
        Gui::Command::runCommand(Gui::Command::Doc, "App.ActiveDocument.recompute()");
        Gui::Command::runCommand(Gui::Command::Gui, "Gui.SendMsgToActiveView(\"ViewFit\")");
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

    return QString::fromLatin1("Base.Placement(Base.Vector(%1,%2,%3),Base.Rotation(%4,%5,%6,%7))")
        .arg(loc.X(),0,'g',Base::UnitsApi::getDecimals())
        .arg(loc.Y(),0,'g',Base::UnitsApi::getDecimals())
        .arg(loc.Z(),0,'g',Base::UnitsApi::getDecimals())
        .arg(rot[0],0,'g',Base::UnitsApi::getDecimals())
        .arg(rot[1],0,'g',Base::UnitsApi::getDecimals())
        .arg(rot[2],0,'g',Base::UnitsApi::getDecimals())
        .arg(rot[3],0,'g',Base::UnitsApi::getDecimals());
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
            throw Base::CADKernelError(gce_ErrorStatusText(arc.Status()));
        Handle(Geom_TrimmedCurve) trim = arc.Value();
        Handle(Geom_Circle) circle = Handle(Geom_Circle)::DownCast(trim->BasisCurve());

        QString name = QString::fromLatin1(doc->getUniqueObjectName("Circle").c_str());
        return QString::fromLatin1(
            "App.ActiveDocument.addObject(\"Part::Circle\",\"%1\")\n"
            "App.ActiveDocument.%1.Radius=%2\n"
            "App.ActiveDocument.%1.Angle1=%3\n"
            "App.ActiveDocument.%1.Angle2=%4\n"
            "App.ActiveDocument.%1.Placement=%5\n")
            .arg(name)
            .arg(circle->Radius(),0,'g',Base::UnitsApi::getDecimals())
            .arg(Base::toDegrees(trim->FirstParameter()),0,'g',Base::UnitsApi::getDecimals())
            .arg(Base::toDegrees(trim->LastParameter ()),0,'g',Base::UnitsApi::getDecimals())
            .arg(toPlacement(circle->Position()));
    }

private:
    std::vector<gp_Pnt> points;
};

}

/* TRANSLATOR PartGui::DlgPrimitives */

DlgPrimitives::DlgPrimitives(QWidget* parent, Part::Primitive* feature)
  : QWidget(parent)
  , ui(new Ui_DlgPrimitives)
  , featurePtr(feature)
{
    ui->setupUi(this);
    Gui::Command::doCommand(Gui::Command::Doc, "from FreeCAD import Base");
    Gui::Command::doCommand(Gui::Command::Doc, "import Part,PartGui");

    // set limits
    //
    // plane
    ui->planeLength->setRange(0, INT_MAX);
    ui->planeWidth->setRange(0, INT_MAX);
    // box
    ui->boxLength->setRange(0, INT_MAX);
    ui->boxWidth->setRange(0, INT_MAX);
    ui->boxHeight->setRange(0, INT_MAX);
    // cylinder
    ui->cylinderRadius->setRange(0, INT_MAX);
    ui->cylinderHeight->setRange(0, INT_MAX);
    ui->cylinderAngle->setRange(0, 360);
    // cone
    ui->coneRadius1->setRange(0, INT_MAX);
    ui->coneRadius2->setRange(0, INT_MAX);
    ui->coneHeight->setRange(0, INT_MAX);
    ui->coneAngle->setRange(0, 360);
    // sphere
    ui->sphereRadius->setRange(0, INT_MAX);
    ui->sphereAngle1->setRange(-90, 90);
    ui->sphereAngle2->setRange(-90, 90);
    ui->sphereAngle3->setRange(0, 360);
    // ellipsoid
    ui->ellipsoidRadius1->setRange(0, INT_MAX);
    ui->ellipsoidRadius2->setRange(0, INT_MAX);
    ui->ellipsoidRadius3->setRange(0, INT_MAX);
    ui->ellipsoidAngle1->setRange(-90, 90);
    ui->ellipsoidAngle2->setRange(-90, 90);
    ui->ellipsoidAngle3->setRange(0, 360);
    // torus
    ui->torusRadius1->setRange(0, INT_MAX);
    ui->torusRadius2->setRange(0, INT_MAX);
    ui->torusAngle1->setRange(-180, 180);
    ui->torusAngle2->setRange(-180, 180);
    ui->torusAngle3->setRange(0, 360);
    // prism
    ui->prismCircumradius->setRange(0, INT_MAX);
    ui->prismHeight->setRange(0, INT_MAX);
    // wedge
    ui->wedgeXmin->setMinimum(INT_MIN);
    ui->wedgeXmin->setMaximum(INT_MAX);
    ui->wedgeYmin->setMinimum(INT_MIN);
    ui->wedgeYmin->setMaximum(INT_MAX);
    ui->wedgeZmin->setMinimum(INT_MIN);
    ui->wedgeZmin->setMaximum(INT_MAX);
    ui->wedgeX2min->setMinimum(INT_MIN);
    ui->wedgeX2min->setMaximum(INT_MAX);
    ui->wedgeZ2min->setMinimum(INT_MIN);
    ui->wedgeZ2min->setMaximum(INT_MAX);
    ui->wedgeXmax->setMinimum(INT_MIN);
    ui->wedgeXmax->setMaximum(INT_MAX);
    ui->wedgeYmax->setMinimum(INT_MIN);
    ui->wedgeYmax->setMaximum(INT_MAX);
    ui->wedgeZmax->setMinimum(INT_MIN);
    ui->wedgeZmax->setMaximum(INT_MAX);
    ui->wedgeX2max->setMinimum(INT_MIN);
    ui->wedgeX2max->setMaximum(INT_MAX);
    ui->wedgeZ2max->setMinimum(INT_MIN);
    ui->wedgeZ2max->setMaximum(INT_MAX);
    // helix
    ui->helixPitch->setRange(0, INT_MAX);
    ui->helixHeight->setRange(0, INT_MAX);
    ui->helixRadius->setRange(0, INT_MAX);
    ui->helixAngle->setRange(-90, 90);
    // circle
    ui->circleRadius->setRange(0, INT_MAX);
    ui->circleAngle1->setRange(0, 360);
    ui->circleAngle2->setRange(0, 360);
    // ellipse
    ui->ellipseMajorRadius->setRange(0, INT_MAX);
    ui->ellipseMinorRadius->setRange(0, INT_MAX);
    ui->ellipseAngle1->setRange(0, 360);
    ui->ellipseAngle2->setRange(0, 360);
    // vertex
    ui->vertexX->setMaximum(INT_MAX);
    ui->vertexY->setMaximum(INT_MAX);
    ui->vertexZ->setMaximum(INT_MAX);
    ui->vertexX->setMinimum(INT_MIN);
    ui->vertexY->setMinimum(INT_MIN);
    ui->vertexZ->setMinimum(INT_MIN);
    // line
    ui->edgeX1->setMaximum(INT_MAX);
    ui->edgeX1->setMinimum(INT_MIN);
    ui->edgeY1->setMaximum(INT_MAX);
    ui->edgeY1->setMinimum(INT_MIN);
    ui->edgeZ1->setMaximum(INT_MAX);
    ui->edgeZ1->setMinimum(INT_MIN);
    ui->edgeX2->setMaximum(INT_MAX);
    ui->edgeX2->setMinimum(INT_MIN);
    ui->edgeY2->setMaximum(INT_MAX);
    ui->edgeY2->setMinimum(INT_MIN);
    ui->edgeZ2->setMaximum(INT_MAX);
    ui->edgeZ2->setMinimum(INT_MIN);
    // RegularPolygon
    ui->regularPolygonCircumradius->setRange(0, INT_MAX);

    // fill the dialog with data if the primitives already exists
    if (feature) {
        // must be the same order as of the stacked widget
        std::vector<Base::Type> types;
        types.emplace_back(Part::Plane::getClassTypeId());
        types.emplace_back(Part::Box::getClassTypeId());
        types.emplace_back(Part::Cylinder::getClassTypeId());
        types.emplace_back(Part::Cone::getClassTypeId());
        types.emplace_back(Part::Sphere::getClassTypeId());
        types.emplace_back(Part::Ellipsoid::getClassTypeId());
        types.emplace_back(Part::Torus::getClassTypeId());
        types.emplace_back(Part::Prism::getClassTypeId());
        types.emplace_back(Part::Wedge::getClassTypeId());
        types.emplace_back(Part::Helix::getClassTypeId());
        types.emplace_back(Part::Spiral::getClassTypeId());
        types.emplace_back(Part::Circle::getClassTypeId());
        types.emplace_back(Part::Ellipse::getClassTypeId());
        types.emplace_back(Part::Vertex::getClassTypeId());
        types.emplace_back(Part::Line::getClassTypeId());
        types.emplace_back(Part::RegularPolygon::getClassTypeId());

        Base::Type type = feature->getTypeId();
        int index = std::distance(types.begin(), std::find(types.begin(), types.end(), type));
        ui->PrimitiveTypeCB->setCurrentIndex(index);
        ui->widgetStack2->setCurrentIndex(index);

        // if existing, the primitive type can not be changed by the user
        ui->PrimitiveTypeCB->setDisabled(feature != nullptr);

        // ToDo: connect signal if there is a preview of primitives available
        // read values from the properties
        if (type == Part::Plane::getClassTypeId()) {
            Part::Plane* plane = static_cast<Part::Plane*>(feature);
            ui->planeLength->setValue(plane->Length.getQuantityValue());
            ui->planeLength->bind(plane->Length);
            ui->planeWidth->setValue(plane->Width.getQuantityValue());
            ui->planeWidth->bind(plane->Width);

            QSignalMapper* mapper = new QSignalMapper(this);
            connect(mapper, SIGNAL(mapped(QWidget*)), this, SLOT(onChangePlane(QWidget*)));
            connectSignalMapper(ui->planeLength, SIGNAL(valueChanged(double)), mapper);
            connectSignalMapper(ui->planeWidth, SIGNAL(valueChanged(double)), mapper);
        }
        else if (type == Part::Box::getClassTypeId()) {
            Part::Box* box = static_cast<Part::Box*>(feature);
            ui->boxLength->setValue(box->Length.getQuantityValue());
            ui->boxLength->bind(box->Length);
            ui->boxWidth->setValue(box->Width.getQuantityValue());
            ui->boxWidth->bind(box->Width);
            ui->boxHeight->setValue(box->Height.getQuantityValue());
            ui->boxHeight->bind(box->Height);

            QSignalMapper* mapper = new QSignalMapper(this);
            connect(mapper, SIGNAL(mapped(QWidget*)), this, SLOT(onChangeBox(QWidget*)));
            connectSignalMapper(ui->boxLength, SIGNAL(valueChanged(double)), mapper);
            connectSignalMapper(ui->boxWidth, SIGNAL(valueChanged(double)), mapper);
            connectSignalMapper(ui->boxHeight, SIGNAL(valueChanged(double)), mapper);
        }
        else if (type == Part::Cylinder::getClassTypeId()) {
            Part::Cylinder* cyl = static_cast<Part::Cylinder*>(feature);
            ui->cylinderRadius->setValue(cyl->Radius.getQuantityValue());
            ui->cylinderRadius->bind(cyl->Radius);
            ui->cylinderHeight->setValue(cyl->Height.getQuantityValue());
            ui->cylinderHeight->bind(cyl->Height);
            ui->cylinderXSkew->setValue(cyl->FirstAngle.getQuantityValue());
            ui->cylinderXSkew->bind(cyl->FirstAngle);
            ui->cylinderYSkew->setValue(cyl->SecondAngle.getQuantityValue());
            ui->cylinderYSkew->bind(cyl->SecondAngle);
            ui->cylinderAngle->setValue(cyl->Angle.getQuantityValue());
            ui->cylinderAngle->bind(cyl->Angle);

            QSignalMapper* mapper = new QSignalMapper(this);
            connect(mapper, SIGNAL(mapped(QWidget*)), this, SLOT(onChangeCylinder(QWidget*)));
            connectSignalMapper(ui->cylinderRadius, SIGNAL(valueChanged(double)), mapper);
            connectSignalMapper(ui->cylinderHeight, SIGNAL(valueChanged(double)), mapper);
            connectSignalMapper(ui->cylinderXSkew, SIGNAL(valueChanged(double)), mapper);
            connectSignalMapper(ui->cylinderYSkew, SIGNAL(valueChanged(double)), mapper);
            connectSignalMapper(ui->cylinderAngle, SIGNAL(valueChanged(double)), mapper);
        }
        else if (type == Part::Cone::getClassTypeId()) {
            Part::Cone* cone = static_cast<Part::Cone*>(feature);
            ui->coneRadius1->setValue(cone->Radius1.getQuantityValue());
            ui->coneRadius1->bind(cone->Radius1);
            ui->coneRadius2->setValue(cone->Radius2.getQuantityValue());
            ui->coneRadius2->bind(cone->Radius2);
            ui->coneHeight->setValue(cone->Height.getQuantityValue());
            ui->coneHeight->bind(cone->Height);
            ui->coneAngle->setValue(cone->Angle.getQuantityValue());
            ui->coneAngle->bind(cone->Angle);

            QSignalMapper* mapper = new QSignalMapper(this);
            connect(mapper, SIGNAL(mapped(QWidget*)), this, SLOT(onChangeCone(QWidget*)));
            connectSignalMapper(ui->coneRadius1, SIGNAL(valueChanged(double)), mapper);
            connectSignalMapper(ui->coneRadius2, SIGNAL(valueChanged(double)), mapper);
            connectSignalMapper(ui->coneHeight, SIGNAL(valueChanged(double)), mapper);
            connectSignalMapper(ui->coneAngle, SIGNAL(valueChanged(double)), mapper);
        }
        else if (type == Part::Sphere::getClassTypeId()) {
            Part::Sphere* sphere = static_cast<Part::Sphere*>(feature);
            ui->sphereRadius->setValue(sphere->Radius.getQuantityValue());
            ui->sphereRadius->bind(sphere->Radius);
            ui->sphereAngle1->setValue(sphere->Angle1.getQuantityValue());
            ui->sphereAngle1->bind(sphere->Angle1);
            ui->sphereAngle2->setValue(sphere->Angle2.getQuantityValue());
            ui->sphereAngle2->bind(sphere->Angle2);
            ui->sphereAngle3->setValue(sphere->Angle3.getQuantityValue());
            ui->sphereAngle3->bind(sphere->Angle3);

            QSignalMapper* mapper = new QSignalMapper(this);
            connect(mapper, SIGNAL(mapped(QWidget*)), this, SLOT(onChangeSphere(QWidget*)));
            connectSignalMapper(ui->sphereRadius, SIGNAL(valueChanged(double)), mapper);
            connectSignalMapper(ui->sphereAngle1, SIGNAL(valueChanged(double)), mapper);
            connectSignalMapper(ui->sphereAngle2, SIGNAL(valueChanged(double)), mapper);
            connectSignalMapper(ui->sphereAngle3, SIGNAL(valueChanged(double)), mapper);
        }
        else if (type == Part::Ellipsoid::getClassTypeId()) {
            Part::Ellipsoid* ell = static_cast<Part::Ellipsoid*>(feature);
            ui->ellipsoidRadius1->setValue(ell->Radius1.getQuantityValue());
            ui->ellipsoidRadius1->bind(ell->Radius1);
            ui->ellipsoidRadius2->setValue(ell->Radius2.getQuantityValue());
            ui->ellipsoidRadius2->bind(ell->Radius2);
            ui->ellipsoidRadius3->setValue(ell->Radius3.getQuantityValue());
            ui->ellipsoidRadius3->bind(ell->Radius3);
            ui->ellipsoidAngle1->setValue(ell->Angle1.getQuantityValue());
            ui->ellipsoidAngle1->bind(ell->Angle1);
            ui->ellipsoidAngle2->setValue(ell->Angle2.getQuantityValue());
            ui->ellipsoidAngle2->bind(ell->Angle2);
            ui->ellipsoidAngle3->setValue(ell->Angle3.getQuantityValue());
            ui->ellipsoidAngle3->bind(ell->Angle3);

            QSignalMapper* mapper = new QSignalMapper(this);
            connect(mapper, SIGNAL(mapped(QWidget*)), this, SLOT(onChangeEllipsoid(QWidget*)));
            connectSignalMapper(ui->ellipsoidRadius1, SIGNAL(valueChanged(double)), mapper);
            connectSignalMapper(ui->ellipsoidRadius2, SIGNAL(valueChanged(double)), mapper);
            connectSignalMapper(ui->ellipsoidRadius3, SIGNAL(valueChanged(double)), mapper);
            connectSignalMapper(ui->ellipsoidAngle1, SIGNAL(valueChanged(double)), mapper);
            connectSignalMapper(ui->ellipsoidAngle2, SIGNAL(valueChanged(double)), mapper);
            connectSignalMapper(ui->ellipsoidAngle3, SIGNAL(valueChanged(double)), mapper);
        }
        else if (type == Part::Torus::getClassTypeId()) {
            Part::Torus* torus = static_cast<Part::Torus*>(feature);
            ui->torusRadius1->setValue(torus->Radius1.getQuantityValue());
            ui->torusRadius1->bind(torus->Radius1);
            ui->torusRadius2->setValue(torus->Radius2.getQuantityValue());
            ui->torusRadius2->bind(torus->Radius2);
            ui->torusAngle1->setValue(torus->Angle1.getQuantityValue());
            ui->torusAngle1->bind(torus->Angle1);
            ui->torusAngle2->setValue(torus->Angle2.getQuantityValue());
            ui->torusAngle2->bind(torus->Angle2);
            ui->torusAngle3->setValue(torus->Angle3.getQuantityValue());
            ui->torusAngle3->bind(torus->Angle3);

            QSignalMapper* mapper = new QSignalMapper(this);
            connect(mapper, SIGNAL(mapped(QWidget*)), this, SLOT(onChangeTorus(QWidget*)));
            connectSignalMapper(ui->torusRadius1, SIGNAL(valueChanged(double)), mapper);
            connectSignalMapper(ui->torusRadius2, SIGNAL(valueChanged(double)), mapper);
            connectSignalMapper(ui->torusAngle1, SIGNAL(valueChanged(double)), mapper);
            connectSignalMapper(ui->torusAngle2, SIGNAL(valueChanged(double)), mapper);
            connectSignalMapper(ui->torusAngle3, SIGNAL(valueChanged(double)), mapper);
        }
        else if (type == Part::Prism::getClassTypeId()) {
            Part::Prism* prism = static_cast<Part::Prism*>(feature);
            ui->prismPolygon->setValue(prism->Polygon.getValue());
            ui->prismCircumradius->setValue(prism->Circumradius.getQuantityValue());
            ui->prismCircumradius->bind(prism->Circumradius);
            ui->prismHeight->setValue(prism->Height.getQuantityValue());
            ui->prismHeight->bind(prism->Height);
            ui->prismXSkew->setValue(prism->FirstAngle.getQuantityValue());
            ui->prismXSkew->bind(prism->FirstAngle);
            ui->prismYSkew->setValue(prism->SecondAngle.getQuantityValue());
            ui->prismYSkew->bind(prism->SecondAngle);

            QSignalMapper* mapper = new QSignalMapper(this);
            connect(mapper, SIGNAL(mapped(QWidget*)), this, SLOT(onChangePrism(QWidget*)));
            connectSignalMapper(ui->prismPolygon, SIGNAL(valueChanged(int)), mapper);
            connectSignalMapper(ui->prismCircumradius, SIGNAL(valueChanged(double)), mapper);
            connectSignalMapper(ui->prismHeight, SIGNAL(valueChanged(double)), mapper);
            connectSignalMapper(ui->prismXSkew, SIGNAL(valueChanged(double)), mapper);
            connectSignalMapper(ui->prismYSkew, SIGNAL(valueChanged(double)), mapper);
        }
        else if (type == Part::Wedge::getClassTypeId()) {
            Part::Wedge* wedge = static_cast<Part::Wedge*>(feature);
            ui->wedgeXmin->setValue(wedge->Xmin.getQuantityValue());
            ui->wedgeXmin->bind(wedge->Xmin);
            ui->wedgeYmin->setValue(wedge->Ymin.getQuantityValue());
            ui->wedgeYmin->bind(wedge->Ymin);
            ui->wedgeZmin->setValue(wedge->Zmin.getQuantityValue());
            ui->wedgeZmin->bind(wedge->Zmin);
            ui->wedgeX2min->setValue(wedge->X2min.getQuantityValue());
            ui->wedgeX2min->bind(wedge->X2min);
            ui->wedgeZ2min->setValue(wedge->Z2min.getQuantityValue());
            ui->wedgeZ2min->bind(wedge->Z2min);
            ui->wedgeXmax->setValue(wedge->Xmax.getQuantityValue());
            ui->wedgeXmax->bind(wedge->Xmax);
            ui->wedgeYmax->setValue(wedge->Ymax.getQuantityValue());
            ui->wedgeYmax->bind(wedge->Ymax);
            ui->wedgeZmax->setValue(wedge->Zmax.getQuantityValue());
            ui->wedgeZmax->bind(wedge->Zmax);
            ui->wedgeX2max->setValue(wedge->X2max.getQuantityValue());
            ui->wedgeX2max->bind(wedge->X2max);
            ui->wedgeZ2max->setValue(wedge->Z2max.getQuantityValue());
            ui->wedgeZ2max->bind(wedge->Z2max);

            QSignalMapper* mapper = new QSignalMapper(this);
            connect(mapper, SIGNAL(mapped(QWidget*)), this, SLOT(onChangeWedge(QWidget*)));
            connectSignalMapper(ui->wedgeXmin, SIGNAL(valueChanged(double)), mapper);
            connectSignalMapper(ui->wedgeYmin, SIGNAL(valueChanged(double)), mapper);
            connectSignalMapper(ui->wedgeZmin, SIGNAL(valueChanged(double)), mapper);
            connectSignalMapper(ui->wedgeX2min, SIGNAL(valueChanged(double)), mapper);
            connectSignalMapper(ui->wedgeZ2min, SIGNAL(valueChanged(double)), mapper);
            connectSignalMapper(ui->wedgeXmax, SIGNAL(valueChanged(double)), mapper);
            connectSignalMapper(ui->wedgeYmax, SIGNAL(valueChanged(double)), mapper);
            connectSignalMapper(ui->wedgeZmax, SIGNAL(valueChanged(double)), mapper);
            connectSignalMapper(ui->wedgeX2max, SIGNAL(valueChanged(double)), mapper);
            connectSignalMapper(ui->wedgeZ2max, SIGNAL(valueChanged(double)), mapper);
        }
        else if (type == Part::Helix::getClassTypeId()) {
            Part::Helix* helix = static_cast<Part::Helix*>(feature);
            ui->helixPitch->setValue(helix->Pitch.getQuantityValue());
            ui->helixPitch->bind(helix->Pitch);
            ui->helixHeight->setValue(helix->Height.getQuantityValue());
            ui->helixHeight->bind(helix->Height);
            ui->helixRadius->setValue(helix->Radius.getQuantityValue());
            ui->helixRadius->bind(helix->Radius);
            ui->helixAngle->setValue(helix->Angle.getQuantityValue());
            ui->helixAngle->bind(helix->Angle);
            ui->helixLocalCS->setCurrentIndex(helix->LocalCoord.getValue());

            QSignalMapper* mapper = new QSignalMapper(this);
            connect(mapper, SIGNAL(mapped(QWidget*)), this, SLOT(onChangeHelix(QWidget*)));
            connectSignalMapper(ui->helixPitch, SIGNAL(valueChanged(double)), mapper);
            connectSignalMapper(ui->helixHeight, SIGNAL(valueChanged(double)), mapper);
            connectSignalMapper(ui->helixRadius, SIGNAL(valueChanged(double)), mapper);
            connectSignalMapper(ui->helixAngle, SIGNAL(valueChanged(double)), mapper);
            connectSignalMapper(ui->helixLocalCS, SIGNAL(currentIndexChanged(int)), mapper);
        }
        else if (type == Part::Spiral::getClassTypeId()) {
            Part::Spiral* spiral = static_cast<Part::Spiral*>(feature);
            ui->spiralGrowth->setValue(spiral->Growth.getQuantityValue());
            ui->spiralGrowth->bind(spiral->Growth);
            ui->spiralRotation->setValue(spiral->Rotations.getQuantityValue().getValue());
            ui->spiralRadius->setValue(spiral->Radius.getQuantityValue());
            ui->spiralRadius->bind(spiral->Radius);

            QSignalMapper* mapper = new QSignalMapper(this);
            connect(mapper, SIGNAL(mapped(QWidget*)), this, SLOT(onChangeSpiral(QWidget*)));
            connectSignalMapper(ui->spiralGrowth, SIGNAL(valueChanged(double)), mapper);
            connectSignalMapper(ui->spiralRotation, SIGNAL(valueChanged(double)), mapper);
            connectSignalMapper(ui->spiralRadius, SIGNAL(valueChanged(double)), mapper);
        }
        else if (type == Part::Circle::getClassTypeId()) {
            Part::Circle* circle = static_cast<Part::Circle*>(feature);
            ui->circleRadius->setValue(circle->Radius.getQuantityValue());
            ui->circleRadius->bind(circle->Radius);
            ui->circleAngle1->setValue(circle->Angle1.getQuantityValue());
            ui->circleAngle1->bind(circle->Angle1);
            ui->circleAngle2->setValue(circle->Angle2.getQuantityValue());
            ui->circleAngle2->bind(circle->Angle2);

            QSignalMapper* mapper = new QSignalMapper(this);
            connect(mapper, SIGNAL(mapped(QWidget*)), this, SLOT(onChangeCircle(QWidget*)));
            connectSignalMapper(ui->circleRadius, SIGNAL(valueChanged(double)), mapper);
            connectSignalMapper(ui->circleAngle1, SIGNAL(valueChanged(double)), mapper);
            connectSignalMapper(ui->circleAngle2, SIGNAL(valueChanged(double)), mapper);
        }
        else if (type == Part::Ellipse::getClassTypeId()) {
            Part::Ellipse* ell = static_cast<Part::Ellipse*>(feature);
            ui->ellipseMajorRadius->setValue(ell->MajorRadius.getQuantityValue());
            ui->ellipseMajorRadius->bind(ell->MajorRadius);
            ui->ellipseMinorRadius->setValue(ell->MinorRadius.getQuantityValue());
            ui->ellipseMinorRadius->bind(ell->MinorRadius);
            ui->ellipseAngle1->setValue(ell->Angle1.getQuantityValue());
            ui->ellipseAngle1->bind(ell->Angle1);
            ui->ellipseAngle2->setValue(ell->Angle2.getQuantityValue());
            ui->ellipseAngle2->bind(ell->Angle2);

            QSignalMapper* mapper = new QSignalMapper(this);
            connect(mapper, SIGNAL(mapped(QWidget*)), this, SLOT(onChangeEllipse(QWidget*)));
            connectSignalMapper(ui->ellipseMajorRadius, SIGNAL(valueChanged(double)), mapper);
            connectSignalMapper(ui->ellipseMinorRadius, SIGNAL(valueChanged(double)), mapper);
            connectSignalMapper(ui->ellipseAngle1, SIGNAL(valueChanged(double)), mapper);
            connectSignalMapper(ui->ellipseAngle2, SIGNAL(valueChanged(double)), mapper);
        }
        else if (type == Part::Vertex::getClassTypeId()) {
            Part::Vertex* v = static_cast<Part::Vertex*>(feature);
            ui->vertexX->setValue(v->X.getQuantityValue());
            ui->vertexX->bind(v->X);
            ui->vertexY->setValue(v->Y.getQuantityValue());
            ui->vertexY->bind(v->Y);
            ui->vertexZ->setValue(v->Z.getQuantityValue());
            ui->vertexZ->bind(v->Z);

            QSignalMapper* mapper = new QSignalMapper(this);
            connect(mapper, SIGNAL(mapped(QWidget*)), this, SLOT(onChangeVertex(QWidget*)));
            connectSignalMapper(ui->vertexX, SIGNAL(valueChanged(double)), mapper);
            connectSignalMapper(ui->vertexY, SIGNAL(valueChanged(double)), mapper);
            connectSignalMapper(ui->vertexZ, SIGNAL(valueChanged(double)), mapper);
        }
        else if (type == Part::Line::getClassTypeId()) {
            Part::Line* line = static_cast<Part::Line*>(feature);
            ui->edgeX1->setValue(line->X1.getQuantityValue());
            ui->edgeX1->bind(line->X1);
            ui->edgeY1->setValue(line->Y1.getQuantityValue());
            ui->edgeY1->bind(line->Y1);
            ui->edgeZ1->setValue(line->Z1.getQuantityValue());
            ui->edgeZ1->bind(line->Z1);
            ui->edgeX2->setValue(line->X2.getQuantityValue());
            ui->edgeX2->bind(line->X2);
            ui->edgeY2->setValue(line->Y2.getQuantityValue());
            ui->edgeY2->bind(line->Y2);
            ui->edgeZ2->setValue(line->Z2.getQuantityValue());
            ui->edgeZ2->bind(line->Z2);

            QSignalMapper* mapper = new QSignalMapper(this);
            connect(mapper, SIGNAL(mapped(QWidget*)), this, SLOT(onChangeLine(QWidget*)));
            connectSignalMapper(ui->edgeX1, SIGNAL(valueChanged(double)), mapper);
            connectSignalMapper(ui->edgeY1, SIGNAL(valueChanged(double)), mapper);
            connectSignalMapper(ui->edgeZ1, SIGNAL(valueChanged(double)), mapper);
            connectSignalMapper(ui->edgeX2, SIGNAL(valueChanged(double)), mapper);
            connectSignalMapper(ui->edgeY2, SIGNAL(valueChanged(double)), mapper);
            connectSignalMapper(ui->edgeZ2, SIGNAL(valueChanged(double)), mapper);
        }
        else if (type == Part::RegularPolygon::getClassTypeId()) {
            Part::RegularPolygon* poly = static_cast<Part::RegularPolygon*>(feature);
            ui->regularPolygonPolygon->setValue(poly->Polygon.getValue());
            ui->regularPolygonCircumradius->setValue(poly->Circumradius.getQuantityValue());
            ui->regularPolygonCircumradius->bind(poly->Circumradius);

            QSignalMapper* mapper = new QSignalMapper(this);
            connect(mapper, SIGNAL(mapped(QWidget*)), this, SLOT(onChangeRegularPolygon(QWidget*)));
            connectSignalMapper(ui->regularPolygonPolygon, SIGNAL(valueChanged(int)), mapper);
            connectSignalMapper(ui->regularPolygonCircumradius, SIGNAL(valueChanged(double)), mapper);
        }
    }
}

/*
 *  Destroys the object and frees any allocated resources
 */
DlgPrimitives::~DlgPrimitives()
{
}

void DlgPrimitives::connectSignalMapper(QWidget *sender, const char *signal, QSignalMapper* mapper)
{
    connect(sender, signal, mapper, SLOT(map()));
    mapper->setMapping(sender, sender);
}

void DlgPrimitives::pickCallback(void * ud, SoEventCallback * n)
{
    const SoMouseButtonEvent * mbe = static_cast<const SoMouseButtonEvent*>(n->getEvent());
    Picker* pick = static_cast<Picker*>(ud);
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
            int mode = 0;
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
                p->createPrimitive(this, ui->PrimitiveTypeCB->currentText(), doc);
            }
        }
    }
}

void DlgPrimitives::on_buttonCircleFromThreePoints_clicked()
{
    CircleFromThreePoints pp;
    executeCallback(&pp);
}

QString DlgPrimitives::createPlane(const QString& objectName, const QString& placement) const
{
    return QString::fromLatin1(
        "App.ActiveDocument.addObject(\"Part::Plane\",\"%1\")\n"
        "App.ActiveDocument.%1.Length=%2\n"
        "App.ActiveDocument.%1.Width=%3\n"
        "App.ActiveDocument.%1.Placement=%4\n"
        "App.ActiveDocument.%1.Label='%5'\n")
        .arg(objectName)
        .arg(Base::UnitsApi::toNumber(ui->planeLength->value()))
        .arg(Base::UnitsApi::toNumber(ui->planeWidth->value()))
        .arg(placement)
        .arg(tr("Plane"));
}

QString DlgPrimitives::createBox(const QString& objectName, const QString& placement) const
{
   return QString::fromLatin1(
        "App.ActiveDocument.addObject(\"Part::Box\",\"%1\")\n"
        "App.ActiveDocument.%1.Length=%2\n"
        "App.ActiveDocument.%1.Width=%3\n"
        "App.ActiveDocument.%1.Height=%4\n"
        "App.ActiveDocument.%1.Placement=%5\n"
        "App.ActiveDocument.%1.Label='%6'\n")
        .arg(objectName)
        .arg(Base::UnitsApi::toNumber(ui->boxLength->value()))
        .arg(Base::UnitsApi::toNumber(ui->boxWidth->value()))
        .arg(Base::UnitsApi::toNumber(ui->boxHeight->value()))
        .arg(placement)
        .arg(tr("Box"));
}

QString DlgPrimitives::createCylinder(const QString& objectName, const QString& placement) const
{
    return QString::fromLatin1(
        "App.ActiveDocument.addObject(\"Part::Cylinder\",\"%1\")\n"
        "App.ActiveDocument.%1.Radius=%2\n"
        "App.ActiveDocument.%1.Height=%3\n"
        "App.ActiveDocument.%1.Angle=%4\n"
        "App.ActiveDocument.%1.FirstAngle=%5\n"
        "App.ActiveDocument.%1.SecondAngle=%6\n"
        "App.ActiveDocument.%1.Placement=%7\n"
        "App.ActiveDocument.%1.Label='%8'\n")
        .arg(objectName)
        .arg(Base::UnitsApi::toNumber(ui->cylinderRadius->value()))
        .arg(Base::UnitsApi::toNumber(ui->cylinderHeight->value()))
        .arg(Base::UnitsApi::toNumber(ui->cylinderAngle->value()))
        .arg(Base::UnitsApi::toNumber(ui->cylinderXSkew->value()))
        .arg(Base::UnitsApi::toNumber(ui->cylinderYSkew->value()))
        .arg(placement)
        .arg(tr("Cylinder"));
}

QString DlgPrimitives::createCone(const QString& objectName, const QString& placement) const
{
    return QString::fromLatin1(
        "App.ActiveDocument.addObject(\"Part::Cone\",\"%1\")\n"
        "App.ActiveDocument.%1.Radius1=%2\n"
        "App.ActiveDocument.%1.Radius2=%3\n"
        "App.ActiveDocument.%1.Height=%4\n"
        "App.ActiveDocument.%1.Angle=%5\n"
        "App.ActiveDocument.%1.Placement=%6\n"
        "App.ActiveDocument.%1.Label='%7'\n")
        .arg(objectName)
        .arg(Base::UnitsApi::toNumber(ui->coneRadius1->value()))
        .arg(Base::UnitsApi::toNumber(ui->coneRadius2->value()))
        .arg(Base::UnitsApi::toNumber(ui->coneHeight->value()))
        .arg(Base::UnitsApi::toNumber(ui->coneAngle->value()))
        .arg(placement)
        .arg(tr("Cone"));
}

QString DlgPrimitives::createSphere(const QString& objectName, const QString& placement) const
{
    return QString::fromLatin1(
        "App.ActiveDocument.addObject(\"Part::Sphere\",\"%1\")\n"
        "App.ActiveDocument.%1.Radius=%2\n"
        "App.ActiveDocument.%1.Angle1=%3\n"
        "App.ActiveDocument.%1.Angle2=%4\n"
        "App.ActiveDocument.%1.Angle3=%5\n"
        "App.ActiveDocument.%1.Placement=%6\n"
        "App.ActiveDocument.%1.Label='%7'\n")
        .arg(objectName)
        .arg(Base::UnitsApi::toNumber(ui->sphereRadius->value()))
        .arg(Base::UnitsApi::toNumber(ui->sphereAngle1->value()))
        .arg(Base::UnitsApi::toNumber(ui->sphereAngle2->value()))
        .arg(Base::UnitsApi::toNumber(ui->sphereAngle3->value()))
        .arg(placement)
        .arg(tr("Sphere"));
}

QString DlgPrimitives::createEllipsoid(const QString& objectName, const QString& placement) const
{
    return QString::fromLatin1(
        "App.ActiveDocument.addObject(\"Part::Ellipsoid\",\"%1\")\n"
        "App.ActiveDocument.%1.Radius1=%2\n"
        "App.ActiveDocument.%1.Radius2=%3\n"
        "App.ActiveDocument.%1.Radius3=%4\n"
        "App.ActiveDocument.%1.Angle1=%5\n"
        "App.ActiveDocument.%1.Angle2=%6\n"
        "App.ActiveDocument.%1.Angle3=%7\n"
        "App.ActiveDocument.%1.Placement=%8\n"
        "App.ActiveDocument.%1.Label='%9'\n")
        .arg(objectName)
        .arg(Base::UnitsApi::toNumber(ui->ellipsoidRadius1->value()))
        .arg(Base::UnitsApi::toNumber(ui->ellipsoidRadius2->value()))
        .arg(Base::UnitsApi::toNumber(ui->ellipsoidRadius3->value()))
        .arg(Base::UnitsApi::toNumber(ui->ellipsoidAngle1->value()))
        .arg(Base::UnitsApi::toNumber(ui->ellipsoidAngle2->value()))
        .arg(Base::UnitsApi::toNumber(ui->ellipsoidAngle3->value()))
        .arg(placement)
        .arg(tr("Ellipsoid"));
}

QString DlgPrimitives::createTorus(const QString& objectName, const QString& placement) const
{
    return QString::fromLatin1(
        "App.ActiveDocument.addObject(\"Part::Torus\",\"%1\")\n"
        "App.ActiveDocument.%1.Radius1=%2\n"
        "App.ActiveDocument.%1.Radius2=%3\n"
        "App.ActiveDocument.%1.Angle1=%4\n"
        "App.ActiveDocument.%1.Angle2=%5\n"
        "App.ActiveDocument.%1.Angle3=%6\n"
        "App.ActiveDocument.%1.Placement=%7\n"
        "App.ActiveDocument.%1.Label='%8'\n")
        .arg(objectName)
        .arg(Base::UnitsApi::toNumber(ui->torusRadius1->value()))
        .arg(Base::UnitsApi::toNumber(ui->torusRadius2->value()))
        .arg(Base::UnitsApi::toNumber(ui->torusAngle1->value()))
        .arg(Base::UnitsApi::toNumber(ui->torusAngle2->value()))
        .arg(Base::UnitsApi::toNumber(ui->torusAngle3->value()))
        .arg(placement)
        .arg(tr("Torus"));
}

QString DlgPrimitives::createPrism(const QString& objectName, const QString& placement) const
{
    return QString::fromLatin1(
        "App.ActiveDocument.addObject(\"Part::Prism\",\"%1\")\n"
        "App.ActiveDocument.%1.Polygon=%2\n"
        "App.ActiveDocument.%1.Circumradius=%3\n"
        "App.ActiveDocument.%1.Height=%4\n"
        "App.ActiveDocument.%1.FirstAngle=%5\n"
        "App.ActiveDocument.%1.SecondAngle=%6\n"
        "App.ActiveDocument.%1.Placement=%7\n"
        "App.ActiveDocument.%1.Label='%8'\n")
        .arg(objectName)
        .arg(ui->prismPolygon->value())
        .arg(Base::UnitsApi::toNumber(ui->prismCircumradius->value()))
        .arg(Base::UnitsApi::toNumber(ui->prismHeight->value()))
        .arg(Base::UnitsApi::toNumber(ui->prismXSkew->value()))
        .arg(Base::UnitsApi::toNumber(ui->prismYSkew->value()))
        .arg(placement)
        .arg(tr("Prism"));
}

QString DlgPrimitives::createWedge(const QString& objectName, const QString& placement) const
{
    return QString::fromLatin1(
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
        .arg(objectName)
        .arg(Base::UnitsApi::toNumber(ui->wedgeXmin->value()))
        .arg(Base::UnitsApi::toNumber(ui->wedgeYmin->value()))
        .arg(Base::UnitsApi::toNumber(ui->wedgeZmin->value()))
        .arg(Base::UnitsApi::toNumber(ui->wedgeX2min->value()))
        .arg(Base::UnitsApi::toNumber(ui->wedgeZ2min->value()))
        .arg(Base::UnitsApi::toNumber(ui->wedgeXmax->value()))
        .arg(Base::UnitsApi::toNumber(ui->wedgeYmax->value()))
        .arg(Base::UnitsApi::toNumber(ui->wedgeZmax->value()))
        .arg(Base::UnitsApi::toNumber(ui->wedgeX2max->value()))
        .arg(Base::UnitsApi::toNumber(ui->wedgeZ2max->value()))
        .arg(placement)
        .arg(tr("Wedge"));
}

QString DlgPrimitives::createHelix(const QString& objectName, const QString& placement) const
{
    return QString::fromLatin1(
        "App.ActiveDocument.addObject(\"Part::Helix\",\"%1\")\n"
        "App.ActiveDocument.%1.Pitch=%2\n"
        "App.ActiveDocument.%1.Height=%3\n"
        "App.ActiveDocument.%1.Radius=%4\n"
        "App.ActiveDocument.%1.Angle=%5\n"
        "App.ActiveDocument.%1.LocalCoord=%6\n"
        "App.ActiveDocument.%1.Style=1\n"
        "App.ActiveDocument.%1.Placement=%7\n"
        "App.ActiveDocument.%1.Label='%8'\n")
        .arg(objectName)
        .arg(Base::UnitsApi::toNumber(ui->helixPitch->value()))
        .arg(Base::UnitsApi::toNumber(ui->helixHeight->value()))
        .arg(Base::UnitsApi::toNumber(ui->helixRadius->value()))
        .arg(Base::UnitsApi::toNumber(ui->helixAngle->value()))
        .arg(ui->helixLocalCS->currentIndex())
        .arg(placement)
        .arg(tr("Helix"));
}

QString DlgPrimitives::createSpiral(const QString& objectName, const QString& placement) const
{
    return QString::fromLatin1(
        "App.ActiveDocument.addObject(\"Part::Spiral\",\"%1\")\n"
        "App.ActiveDocument.%1.Growth=%2\n"
        "App.ActiveDocument.%1.Rotations=%3\n"
        "App.ActiveDocument.%1.Radius=%4\n"
        "App.ActiveDocument.%1.Placement=%5\n"
        "App.ActiveDocument.%1.Label='%6'\n")
        .arg(objectName)
        .arg(Base::UnitsApi::toNumber(ui->spiralGrowth->value()))
        .arg(Base::UnitsApi::toNumber(ui->spiralRotation->value()))
        .arg(Base::UnitsApi::toNumber(ui->spiralRadius->value()))
        .arg(placement)
        .arg(tr("Spiral"));
}

QString DlgPrimitives::createCircle(const QString& objectName, const QString& placement) const
{
    return QString::fromLatin1(
        "App.ActiveDocument.addObject(\"Part::Circle\",\"%1\")\n"
        "App.ActiveDocument.%1.Radius=%2\n"
        "App.ActiveDocument.%1.Angle1=%3\n"
        "App.ActiveDocument.%1.Angle2=%4\n"
        "App.ActiveDocument.%1.Placement=%5\n"
        "App.ActiveDocument.%1.Label='%6'\n")
        .arg(objectName)
        .arg(Base::UnitsApi::toNumber(ui->circleRadius->value()))
        .arg(Base::UnitsApi::toNumber(ui->circleAngle1->value()))
        .arg(Base::UnitsApi::toNumber(ui->circleAngle2->value()))
        .arg(placement)
        .arg(tr("Circle"));
}

QString DlgPrimitives::createEllipse(const QString& objectName, const QString& placement) const
{
    return QString::fromLatin1(
        "App.ActiveDocument.addObject(\"Part::Ellipse\",\"%1\")\n"
        "App.ActiveDocument.%1.MajorRadius=%2\n"
        "App.ActiveDocument.%1.MinorRadius=%3\n"
        "App.ActiveDocument.%1.Angle1=%4\n"
        "App.ActiveDocument.%1.Angle2=%5\n"
        "App.ActiveDocument.%1.Placement=%6\n"
        "App.ActiveDocument.%1.Label='%7'\n")
        .arg(objectName)
        .arg(Base::UnitsApi::toNumber(ui->ellipseMajorRadius->value()))
        .arg(Base::UnitsApi::toNumber(ui->ellipseMinorRadius->value()))
        .arg(Base::UnitsApi::toNumber(ui->ellipseAngle1->value()))
        .arg(Base::UnitsApi::toNumber(ui->ellipseAngle2->value()))
        .arg(placement)
        .arg(tr("Ellipse"));
}

QString DlgPrimitives::createVertex(const QString& objectName, const QString& placement) const
{
    return QString::fromLatin1(
        "App.ActiveDocument.addObject(\"Part::Vertex\",\"%1\")\n"
        "App.ActiveDocument.%1.X=%2\n"
        "App.ActiveDocument.%1.Y=%3\n"
        "App.ActiveDocument.%1.Z=%4\n"
        "App.ActiveDocument.%1.Placement=%5\n"
        "App.ActiveDocument.%1.Label='%6'\n")
        .arg(objectName)
        .arg(Base::UnitsApi::toNumber(ui->vertexX->value()))
        .arg(Base::UnitsApi::toNumber(ui->vertexY->value()))
        .arg(Base::UnitsApi::toNumber(ui->vertexZ->value()))
        .arg(placement)
        .arg(tr("Vertex"));
}

QString DlgPrimitives::createLine(const QString& objectName, const QString& placement) const
{
    return QString::fromLatin1(
        "App.ActiveDocument.addObject(\"Part::Line\",\"%1\")\n"
        "App.ActiveDocument.%1.X1=%2\n"
        "App.ActiveDocument.%1.Y1=%3\n"
        "App.ActiveDocument.%1.Z1=%4\n"
        "App.ActiveDocument.%1.X2=%5\n"
        "App.ActiveDocument.%1.Y2=%6\n"
        "App.ActiveDocument.%1.Z2=%7\n"
        "App.ActiveDocument.%1.Placement=%8\n"
        "App.ActiveDocument.%1.Label='%9'\n")
        .arg(objectName)
        .arg(Base::UnitsApi::toNumber(ui->edgeX1->value()))
        .arg(Base::UnitsApi::toNumber(ui->edgeY1->value()))
        .arg(Base::UnitsApi::toNumber(ui->edgeZ1->value()))
        .arg(Base::UnitsApi::toNumber(ui->edgeX2->value()))
        .arg(Base::UnitsApi::toNumber(ui->edgeY2->value()))
        .arg(Base::UnitsApi::toNumber(ui->edgeZ2->value()))
        .arg(placement)
        .arg(tr("Line"));
}

QString DlgPrimitives::createRegularPolygon(const QString& objectName, const QString& placement) const
{
    return QString::fromLatin1(
        "App.ActiveDocument.addObject(\"Part::RegularPolygon\",\"%1\")\n"
        "App.ActiveDocument.%1.Polygon=%2\n"
        "App.ActiveDocument.%1.Circumradius=%3\n"
        "App.ActiveDocument.%1.Placement=%4\n"
        "App.ActiveDocument.%1.Label='%5'\n")
        .arg(objectName)
        .arg(ui->regularPolygonPolygon->value())
        .arg(Base::UnitsApi::toNumber(ui->regularPolygonCircumradius->value()))
        .arg(placement)
        .arg(tr("Regular polygon"));
}

void DlgPrimitives::createPrimitive(const QString& placement)
{
    try {
        QString cmd; QString name;
        App::Document* doc = App::GetApplication().getActiveDocument();
        if (!doc) {
            QMessageBox::warning(this, tr("Create %1")
                .arg(ui->PrimitiveTypeCB->currentText()), tr("No active document"));
            return;
        }
        if (ui->PrimitiveTypeCB->currentIndex() == 0) {         // plane
            name = QString::fromLatin1(doc->getUniqueObjectName("Plane").c_str());
            cmd = createPlane(name, placement);
        }
        else if (ui->PrimitiveTypeCB->currentIndex() == 1) {         // box
            name = QString::fromLatin1(doc->getUniqueObjectName("Box").c_str());
            cmd = createBox(name, placement);
        }
        else if (ui->PrimitiveTypeCB->currentIndex() == 2) {  // cylinder
            name = QString::fromLatin1(doc->getUniqueObjectName("Cylinder").c_str());
            cmd = createCylinder(name, placement);
        }
        else if (ui->PrimitiveTypeCB->currentIndex() == 3) {  // cone
            name = QString::fromLatin1(doc->getUniqueObjectName("Cone").c_str());
            cmd = createCone(name, placement);
        }
        else if (ui->PrimitiveTypeCB->currentIndex() == 4) {  // sphere
            name = QString::fromLatin1(doc->getUniqueObjectName("Sphere").c_str());
            cmd = createSphere(name, placement);
        }
        else if (ui->PrimitiveTypeCB->currentIndex() == 5) {  // ellipsoid
            name = QString::fromLatin1(doc->getUniqueObjectName("Ellipsoid").c_str());
            cmd = createEllipsoid(name, placement);
        }
        else if (ui->PrimitiveTypeCB->currentIndex() == 6) {  // torus
            name = QString::fromLatin1(doc->getUniqueObjectName("Torus").c_str());
            cmd = createTorus(name, placement);
        }
        else if (ui->PrimitiveTypeCB->currentIndex() == 7) {  // prism
            name = QString::fromLatin1(doc->getUniqueObjectName("Prism").c_str());
            cmd = createPrism(name, placement);
        }
        else if (ui->PrimitiveTypeCB->currentIndex() == 8) {  // wedge
            name = QString::fromLatin1(doc->getUniqueObjectName("Wedge").c_str());
            cmd = createWedge(name, placement);
        }
        else if (ui->PrimitiveTypeCB->currentIndex() == 9) {  // helix
            name = QString::fromLatin1(doc->getUniqueObjectName("Helix").c_str());
            cmd = createHelix(name, placement);
        }
        else if (ui->PrimitiveTypeCB->currentIndex() == 10) {  // spiral
            name = QString::fromLatin1(doc->getUniqueObjectName("Spiral").c_str());
            cmd = createSpiral(name, placement);
        }
        else if (ui->PrimitiveTypeCB->currentIndex() == 11) {  // circle
            name = QString::fromLatin1(doc->getUniqueObjectName("Circle").c_str());
            cmd = createCircle(name, placement);
        }
        else if (ui->PrimitiveTypeCB->currentIndex() == 12) {  // ellipse
            name = QString::fromLatin1(doc->getUniqueObjectName("Ellipse").c_str());
            cmd = createEllipse(name, placement);
        }
        else if (ui->PrimitiveTypeCB->currentIndex() == 13) {  // vertex
            name = QString::fromLatin1(doc->getUniqueObjectName("Vertex").c_str());
            cmd = createVertex(name, placement);
        }
        else if (ui->PrimitiveTypeCB->currentIndex() == 14) {  // line
            name = QString::fromLatin1(doc->getUniqueObjectName("Line").c_str());
            cmd = createLine(name, placement);
        }
        else if (ui->PrimitiveTypeCB->currentIndex() == 15) {  // RegularPolygon
            name = QString::fromLatin1(doc->getUniqueObjectName("RegularPolygon").c_str());
            cmd = createRegularPolygon(name, placement);
        }

        // Execute the Python block
        QString prim = tr("Create %1").arg(ui->PrimitiveTypeCB->currentText());
        Gui::Application::Instance->activeDocument()->openCommand(prim.toUtf8());
        Gui::Command::runCommand(Gui::Command::Doc, cmd.toUtf8());
        Gui::Command::runCommand(Gui::Command::Doc, getAutoGroupCommandStr(name).toUtf8());
        Gui::Application::Instance->activeDocument()->commitCommand();
        Gui::Command::runCommand(Gui::Command::Doc, "App.ActiveDocument.recompute()");
        Gui::Command::runCommand(Gui::Command::Gui, "Gui.SendMsgToActiveView(\"ViewFit\")");
    }
    catch (const Base::PyException& e) {
        QMessageBox::warning(this, tr("Create %1")
            .arg(ui->PrimitiveTypeCB->currentText()), QString::fromLatin1(e.what()));
    }
}

QString DlgPrimitives::changePlane(const QString& objectName, const QString& placement) const
{
    return QString::fromLatin1(
        "%1.Length=%2\n"
        "%1.Width=%3\n"
        "%1.Placement=%4\n")
        .arg(objectName)
        .arg(Base::UnitsApi::toNumber(ui->planeLength->value()))
        .arg(Base::UnitsApi::toNumber(ui->planeWidth->value()))
        .arg(placement);
}

QString DlgPrimitives::changeBox(const QString& objectName, const QString& placement) const
{
    return QString::fromLatin1(
        "%1.Length=%2\n"
        "%1.Width=%3\n"
        "%1.Height=%4\n"
        "%1.Placement=%5\n")
        .arg(objectName)
        .arg(Base::UnitsApi::toNumber(ui->boxLength->value()))
        .arg(Base::UnitsApi::toNumber(ui->boxWidth->value()))
        .arg(Base::UnitsApi::toNumber(ui->boxHeight->value()))
        .arg(placement);
}

QString DlgPrimitives::changeCylinder(const QString& objectName, const QString& placement) const
{
    return QString::fromLatin1(
        "%1.Radius=%2\n"
        "%1.Height=%3\n"
        "%1.Angle=%4\n"
        "%1.Placement=%5\n")
        .arg(objectName)
        .arg(Base::UnitsApi::toNumber(ui->cylinderRadius->value()))
        .arg(Base::UnitsApi::toNumber(ui->cylinderHeight->value()))
        .arg(Base::UnitsApi::toNumber(ui->cylinderAngle->value()))
        .arg(placement);
}

QString DlgPrimitives::changeCone(const QString& objectName, const QString& placement) const
{
    return QString::fromLatin1(
        "%1.Radius1=%2\n"
        "%1.Radius2=%3\n"
        "%1.Height=%4\n"
        "%1.Angle=%5\n"
        "%1.Placement=%6\n")
        .arg(objectName)
        .arg(Base::UnitsApi::toNumber(ui->coneRadius1->value()))
        .arg(Base::UnitsApi::toNumber(ui->coneRadius2->value()))
        .arg(Base::UnitsApi::toNumber(ui->coneHeight->value()))
        .arg(Base::UnitsApi::toNumber(ui->coneAngle->value()))
        .arg(placement);
}

QString DlgPrimitives::changeSphere(const QString& objectName, const QString& placement) const
{
    return QString::fromLatin1(
        "%1.Radius=%2\n"
        "%1.Angle1=%3\n"
        "%1.Angle2=%4\n"
        "%1.Angle3=%5\n"
        "%1.Placement=%6\n")
        .arg(objectName)
        .arg(Base::UnitsApi::toNumber(ui->sphereRadius->value()))
        .arg(Base::UnitsApi::toNumber(ui->sphereAngle1->value()))
        .arg(Base::UnitsApi::toNumber(ui->sphereAngle2->value()))
        .arg(Base::UnitsApi::toNumber(ui->sphereAngle3->value()))
        .arg(placement);
}

QString DlgPrimitives::changeEllipsoid(const QString& objectName, const QString& placement) const
{
    return QString::fromLatin1(
        "%1.Radius1=%2\n"
        "%1.Radius2=%3\n"
        "%1.Radius3=%4\n"
        "%1.Angle1=%5\n"
        "%1.Angle2=%6\n"
        "%1.Angle3=%7\n"
        "%1.Placement=%8\n")
        .arg(objectName)
        .arg(Base::UnitsApi::toNumber(ui->ellipsoidRadius1->value()))
        .arg(Base::UnitsApi::toNumber(ui->ellipsoidRadius2->value()))
        .arg(Base::UnitsApi::toNumber(ui->ellipsoidRadius3->value()))
        .arg(Base::UnitsApi::toNumber(ui->ellipsoidAngle1->value()))
        .arg(Base::UnitsApi::toNumber(ui->ellipsoidAngle2->value()))
        .arg(Base::UnitsApi::toNumber(ui->ellipsoidAngle3->value()))
        .arg(placement);
}

QString DlgPrimitives::changeTorus(const QString& objectName, const QString& placement) const
{
    return QString::fromLatin1(
        "%1.Radius1=%2\n"
        "%1.Radius2=%3\n"
        "%1.Angle1=%4\n"
        "%1.Angle2=%5\n"
        "%1.Angle3=%6\n"
        "%1.Placement=%7\n")
        .arg(objectName)
        .arg(Base::UnitsApi::toNumber(ui->torusRadius1->value()))
        .arg(Base::UnitsApi::toNumber(ui->torusRadius2->value()))
        .arg(Base::UnitsApi::toNumber(ui->torusAngle1->value()))
        .arg(Base::UnitsApi::toNumber(ui->torusAngle2->value()))
        .arg(Base::UnitsApi::toNumber(ui->torusAngle3->value()))
        .arg(placement);
}

QString DlgPrimitives::changePrism(const QString& objectName, const QString& placement) const
{
    return QString::fromLatin1(
        "%1.Polygon=%2\n"
        "%1.Circumradius=%3\n"
        "%1.Height=%4\n"
        "%1.FirstAngle=%5\n"
        "%1.SecondAngle=%6\n"
        "%1.Placement=%7\n")
        .arg(objectName)
        .arg(ui->prismPolygon->value())
        .arg(Base::UnitsApi::toNumber(ui->prismCircumradius->value()))
        .arg(Base::UnitsApi::toNumber(ui->prismHeight->value()))
        .arg(Base::UnitsApi::toNumber(ui->prismXSkew->value()))
        .arg(Base::UnitsApi::toNumber(ui->prismYSkew->value()))
        .arg(placement);
}

QString DlgPrimitives::changeWedge(const QString& objectName, const QString& placement) const
{
    return QString::fromLatin1(
        "%1.Xmin=%2\n"
        "%1.Ymin=%3\n"
        "%1.Zmin=%4\n"
        "%1.X2min=%5\n"
        "%1.Z2min=%6\n"
        "%1.Xmax=%7\n"
        "%1.Ymax=%8\n"
        "%1.Zmax=%9\n"
        "%1.X2max=%10\n"
        "%1.Z2max=%11\n"
        "%1.Placement=%12\n")
        .arg(objectName)
        .arg(Base::UnitsApi::toNumber(ui->wedgeXmin->value()))
        .arg(Base::UnitsApi::toNumber(ui->wedgeYmin->value()))
        .arg(Base::UnitsApi::toNumber(ui->wedgeZmin->value()))
        .arg(Base::UnitsApi::toNumber(ui->wedgeX2min->value()))
        .arg(Base::UnitsApi::toNumber(ui->wedgeZ2min->value()))
        .arg(Base::UnitsApi::toNumber(ui->wedgeXmax->value()))
        .arg(Base::UnitsApi::toNumber(ui->wedgeYmax->value()))
        .arg(Base::UnitsApi::toNumber(ui->wedgeZmax->value()))
        .arg(Base::UnitsApi::toNumber(ui->wedgeX2max->value()))
        .arg(Base::UnitsApi::toNumber(ui->wedgeZ2max->value()))
        .arg(placement);
}

QString DlgPrimitives::changeHelix(const QString& objectName, const QString& placement) const
{
    return QString::fromLatin1(
        "%1.Pitch=%2\n"
        "%1.Height=%3\n"
        "%1.Radius=%4\n"
        "%1.Angle=%5\n"
        "%1.LocalCoord=%6\n"
        "%1.Placement=%7\n")
        .arg(objectName)
        .arg(Base::UnitsApi::toNumber(ui->helixPitch->value()))
        .arg(Base::UnitsApi::toNumber(ui->helixHeight->value()))
        .arg(Base::UnitsApi::toNumber(ui->helixRadius->value()))
        .arg(Base::UnitsApi::toNumber(ui->helixAngle->value()))
        .arg(ui->helixLocalCS->currentIndex())
        .arg(placement);
}

QString DlgPrimitives::changeSpiral(const QString& objectName, const QString& placement) const
{
    return QString::fromLatin1(
        "%1.Growth=%2\n"
        "%1.Rotations=%3\n"
        "%1.Radius=%4\n"
        "%1.Placement=%5\n")
        .arg(objectName)
        .arg(Base::UnitsApi::toNumber(ui->spiralGrowth->value()))
        .arg(Base::UnitsApi::toNumber(ui->spiralRotation->value()))
        .arg(Base::UnitsApi::toNumber(ui->spiralRadius->value()))
        .arg(placement);
}

QString DlgPrimitives::changeCircle(const QString& objectName, const QString& placement) const
{
    return QString::fromLatin1(
        "%1.Radius=%2\n"
        "%1.Angle1=%3\n"
        "%1.Angle2=%4\n"
        "%1.Placement=%5\n")
        .arg(objectName)
        .arg(Base::UnitsApi::toNumber(ui->circleRadius->value()))
        .arg(Base::UnitsApi::toNumber(ui->circleAngle1->value()))
        .arg(Base::UnitsApi::toNumber(ui->circleAngle2->value()))
        .arg(placement);
}

QString DlgPrimitives::changeEllipse(const QString& objectName, const QString& placement) const
{
    return QString::fromLatin1(
        "%1.MajorRadius=%2\n"
        "%1.MinorRadius=%3\n"
        "%1.Angle1=%4\n"
        "%1.Angle2=%5\n"
        "%1.Placement=%6\n")
        .arg(objectName)
        .arg(Base::UnitsApi::toNumber(ui->ellipseMajorRadius->value()))
        .arg(Base::UnitsApi::toNumber(ui->ellipseMinorRadius->value()))
        .arg(Base::UnitsApi::toNumber(ui->ellipseAngle1->value()))
        .arg(Base::UnitsApi::toNumber(ui->ellipseAngle2->value()))
        .arg(placement);
}

QString DlgPrimitives::changeVertex(const QString& objectName, const QString& placement) const
{
    return QString::fromLatin1(
        "%1.X=%2\n"
        "%1.Y=%3\n"
        "%1.Z=%4\n"
        "%1.Placement=%5\n")
        .arg(objectName)
        .arg(Base::UnitsApi::toNumber(ui->vertexX->value()))
        .arg(Base::UnitsApi::toNumber(ui->vertexY->value()))
        .arg(Base::UnitsApi::toNumber(ui->vertexZ->value()))
        .arg(placement);
}

QString DlgPrimitives::changeLine(const QString& objectName, const QString& placement) const
{
    return QString::fromLatin1(
        "%1.X1=%2\n"
        "%1.Y1=%3\n"
        "%1.Z1=%4\n"
        "%1.X2=%5\n"
        "%1.Y2=%6\n"
        "%1.Z2=%7\n"
        "%1.Placement=%8\n")
        .arg(objectName)
        .arg(Base::UnitsApi::toNumber(ui->edgeX1->value()))
        .arg(Base::UnitsApi::toNumber(ui->edgeY1->value()))
        .arg(Base::UnitsApi::toNumber(ui->edgeZ1->value()))
        .arg(Base::UnitsApi::toNumber(ui->edgeX2->value()))
        .arg(Base::UnitsApi::toNumber(ui->edgeY2->value()))
        .arg(Base::UnitsApi::toNumber(ui->edgeZ2->value()))
        .arg(placement);
}

QString DlgPrimitives::changeRegularPolygon(const QString& objectName, const QString& placement) const
{
    return QString::fromLatin1(
        "%1.Polygon=%2\n"
        "%1.Circumradius=%3\n"
        "%1.Placement=%4\n")
        .arg(objectName)
        .arg(ui->regularPolygonPolygon->value())
        .arg(Base::UnitsApi::toNumber(ui->regularPolygonCircumradius->value()))
        .arg(placement);
}

void DlgPrimitives::accept(const QString& placement)
{
    if (featurePtr.expired())
        return;
    QString command;
    App::Document* doc = featurePtr->getDocument();
    Base::Type type = featurePtr->getTypeId();
    QString objectName = QString::fromLatin1("App.getDocument(\"%1\").%2")
                         .arg(QString::fromLatin1(doc->getName()))
                         .arg(QString::fromLatin1(featurePtr->getNameInDocument()));

    // read values from the properties
    if (type == Part::Plane::getClassTypeId()) {
        command = changePlane(objectName, placement);
    }
    else if (type == Part::Box::getClassTypeId()) {
        command = changeBox(objectName, placement);
    }
    else if (type == Part::Cylinder::getClassTypeId()) {
        command = changeCylinder(objectName, placement);
    }
    else if (type == Part::Cone::getClassTypeId()) {
        command = changeCone(objectName, placement);
    }
    else if (type == Part::Sphere::getClassTypeId()) {
        command = changeSphere(objectName, placement);
    }
    else if (type == Part::Ellipsoid::getClassTypeId()) {
        command = changeEllipsoid(objectName, placement);
    }
    else if (type == Part::Torus::getClassTypeId()) {
        command = changeTorus(objectName, placement);
    }
    else if (type == Part::Prism::getClassTypeId()) {
        command = changePrism(objectName, placement);
    }
    else if (type == Part::Wedge::getClassTypeId()) {
        command = changeWedge(objectName, placement);
    }
    else if (type == Part::Helix::getClassTypeId()) {
        command = changeHelix(objectName, placement);
    }
    else if (type == Part::Spiral::getClassTypeId()) {
        command = changeSpiral(objectName, placement);
    }
    else if (type == Part::Circle::getClassTypeId()) {
        command = changeCircle(objectName, placement);
    }
    else if (type == Part::Ellipse::getClassTypeId()) {
        command = changeEllipse(objectName, placement);
    }
    else if (type == Part::Vertex::getClassTypeId()) {
        command = changeVertex(objectName, placement);
    }
    else if (type == Part::Line::getClassTypeId()) {
        command = changeLine(objectName, placement);
    }
    else if (type == Part::RegularPolygon::getClassTypeId()) {
        command = changeRegularPolygon(objectName, placement);
    }

    // execute command, a transaction is already opened
    Gui::Command::runCommand(Gui::Command::App, command.toLatin1());
    doc->recompute();
    // commit undo command
    doc->commitTransaction();
}

void DlgPrimitives::reject()
{
    if (featurePtr.expired())
        return;
    App::Document* doc = featurePtr->getDocument();
    doc->abortTransaction();
}

void DlgPrimitives::onChangePlane(QWidget* widget)
{
    if (featurePtr.expired())
        return;
    Part::Plane* plane = featurePtr.get<Part::Plane>();
    if (widget == ui->planeLength) {
        plane->Length.setValue(ui->planeLength->value().getValue());
    }
    else if (widget == ui->planeWidth) {
        plane->Width.setValue(ui->planeWidth->value().getValue());
    }

    plane->recomputeFeature();
}

void DlgPrimitives::onChangeBox(QWidget* widget)
{
    if (featurePtr.expired())
        return;
    Part::Box* box = featurePtr.get<Part::Box>();
    if (widget == ui->boxLength) {
        box->Length.setValue(ui->boxLength->value().getValue());
    }
    else if (widget == ui->boxWidth) {
        box->Width.setValue(ui->boxWidth->value().getValue());
    }
    else if (widget == ui->boxHeight) {
        box->Height.setValue(ui->boxHeight->value().getValue());
    }

    box->recomputeFeature();
}

void DlgPrimitives::onChangeCylinder(QWidget* widget)
{
    if (featurePtr.expired())
        return;
    Part::Cylinder* cyl = featurePtr.get<Part::Cylinder>();
    if (widget == ui->cylinderRadius) {
        cyl->Radius.setValue(ui->cylinderRadius->value().getValue());
    }
    else if (widget == ui->cylinderHeight) {
        cyl->Height.setValue(ui->cylinderHeight->value().getValue());
    }
    else if (widget == ui->cylinderAngle) {
        cyl->Angle.setValue(ui->cylinderAngle->value().getValue());
    }
    else if (widget == ui->cylinderXSkew) {
        cyl->FirstAngle.setValue(ui->cylinderXSkew->value().getValue());
    }
    else if (widget == ui->cylinderYSkew) {
        cyl->SecondAngle.setValue(ui->cylinderYSkew->value().getValue());
    }

    cyl->recomputeFeature();
}

void DlgPrimitives::onChangeCone(QWidget* widget)
{
    if (featurePtr.expired())
        return;
    Part::Cone* cone = featurePtr.get<Part::Cone>();
    if (widget == ui->coneRadius1) {
        cone->Radius1.setValue(ui->coneRadius1->value().getValue());
    }
    else if (widget == ui->coneRadius2) {
        cone->Radius2.setValue(ui->coneRadius2->value().getValue());
    }
    else if (widget == ui->coneHeight) {
        cone->Height.setValue(ui->coneHeight->value().getValue());
    }
    else if (widget == ui->coneAngle) {
        cone->Angle.setValue(ui->coneAngle->value().getValue());
    }

    cone->recomputeFeature();
}

void DlgPrimitives::onChangeSphere(QWidget* widget)
{
    if (featurePtr.expired())
        return;
    Part::Sphere* sphere = featurePtr.get<Part::Sphere>();
    if (widget == ui->sphereRadius) {
        sphere->Radius.setValue(ui->sphereRadius->value().getValue());
    }
    else if (widget == ui->sphereAngle1) {
        sphere->Angle1.setValue(ui->sphereAngle1->value().getValue());
    }
    else if (widget == ui->sphereAngle2) {
        sphere->Angle2.setValue(ui->sphereAngle2->value().getValue());
    }
    else if (widget == ui->sphereAngle3) {
        sphere->Angle3.setValue(ui->sphereAngle3->value().getValue());
    }

    sphere->recomputeFeature();
}

void DlgPrimitives::onChangeEllipsoid(QWidget* widget)
{
    if (featurePtr.expired())
        return;
    Part::Ellipsoid* ell = featurePtr.get<Part::Ellipsoid>();
    if (widget == ui->ellipsoidRadius1) {
        ell->Radius1.setValue(ui->ellipsoidRadius1->value().getValue());
    }
    else if (widget == ui->ellipsoidRadius2) {
        ell->Radius2.setValue(ui->ellipsoidRadius2->value().getValue());
    }
    else if (widget == ui->ellipsoidRadius3) {
        ell->Radius3.setValue(ui->ellipsoidRadius3->value().getValue());
    }
    else if (widget == ui->ellipsoidAngle1) {
        ell->Angle1.setValue(ui->ellipsoidAngle1->value().getValue());
    }
    else if (widget == ui->ellipsoidAngle2) {
        ell->Angle2.setValue(ui->ellipsoidAngle2->value().getValue());
    }
    else if (widget == ui->ellipsoidAngle3) {
        ell->Angle3.setValue(ui->ellipsoidAngle3->value().getValue());
    }

    ell->recomputeFeature();
}

void DlgPrimitives::onChangeTorus(QWidget* widget)
{
    if (featurePtr.expired())
        return;
    Part::Torus* torus = featurePtr.get<Part::Torus>();
    if (widget == ui->torusRadius1) {
        torus->Radius1.setValue(ui->torusRadius1->value().getValue());
    }
    else if (widget == ui->torusRadius2) {
        torus->Radius2.setValue(ui->torusRadius2->value().getValue());
    }
    else if (widget == ui->torusAngle1) {
        torus->Angle1.setValue(ui->torusAngle1->value().getValue());
    }
    else if (widget == ui->torusAngle2) {
        torus->Angle2.setValue(ui->torusAngle2->value().getValue());
    }
    else if (widget == ui->torusAngle3) {
        torus->Angle3.setValue(ui->torusAngle3->value().getValue());
    }

    torus->recomputeFeature();
}

void DlgPrimitives::onChangePrism(QWidget* widget)
{
    if (featurePtr.expired())
        return;
    Part::Prism* prism = featurePtr.get<Part::Prism>();
    if (widget == ui->prismPolygon) {
        prism->Polygon.setValue(ui->prismPolygon->value());
    }
    else if (widget == ui->prismCircumradius) {
        prism->Circumradius.setValue(ui->prismCircumradius->value().getValue());
    }
    else if (widget == ui->prismHeight) {
        prism->Height.setValue(ui->prismHeight->value().getValue());
    }
    else if (widget == ui->prismXSkew) {
        prism->FirstAngle.setValue(ui->prismXSkew->value().getValue());
    }
    else if (widget == ui->prismYSkew) {
        prism->SecondAngle.setValue(ui->prismYSkew->value().getValue());
    }

    prism->recomputeFeature();
}

void DlgPrimitives::onChangeWedge(QWidget* widget)
{
    if (featurePtr.expired())
        return;
    Part::Wedge* wedge = featurePtr.get<Part::Wedge>();
    if (widget == ui->wedgeXmin) {
        wedge->Xmin.setValue(ui->wedgeXmin->value().getValue());
    }
    else if (widget == ui->wedgeYmin) {
        wedge->Ymin.setValue(ui->wedgeYmin->value().getValue());
    }
    else if (widget == ui->wedgeZmin) {
        wedge->Zmin.setValue(ui->wedgeZmin->value().getValue());
    }
    else if (widget == ui->wedgeX2min) {
        wedge->X2min.setValue(ui->wedgeX2min->value().getValue());
    }
    else if (widget == ui->wedgeZ2min) {
        wedge->Z2min.setValue(ui->wedgeZ2min->value().getValue());
    }
    else if (widget == ui->wedgeXmax) {
        wedge->Xmax.setValue(ui->wedgeXmax->value().getValue());
    }
    else if (widget == ui->wedgeYmax) {
        wedge->Ymax.setValue(ui->wedgeYmax->value().getValue());
    }
    else if (widget == ui->wedgeZmax) {
        wedge->Zmax.setValue(ui->wedgeZmax->value().getValue());
    }
    else if (widget == ui->wedgeX2max) {
        wedge->X2max.setValue(ui->wedgeX2max->value().getValue());
    }
    else if (widget == ui->wedgeZ2max) {
        wedge->Z2max.setValue(ui->wedgeZ2max->value().getValue());
    }

    wedge->recomputeFeature();
}

void DlgPrimitives::onChangeHelix(QWidget* widget)
{
    if (featurePtr.expired())
        return;
    Part::Helix* helix = featurePtr.get<Part::Helix>();
    if (widget == ui->helixPitch) {
        helix->Pitch.setValue(ui->helixPitch->value().getValue());
    }
    else if (widget == ui->helixHeight) {
        helix->Height.setValue(ui->helixHeight->value().getValue());
    }
    else if (widget == ui->helixRadius) {
        helix->Radius.setValue(ui->helixRadius->value().getValue());
    }
    else if (widget == ui->helixAngle) {
        helix->Angle.setValue(ui->helixAngle->value().getValue());
    }
    else if (widget == ui->helixLocalCS) {
        helix->LocalCoord.setValue(ui->helixLocalCS->currentIndex());
    }

    helix->recomputeFeature();
}

void DlgPrimitives::onChangeSpiral(QWidget* widget)
{
    if (featurePtr.expired())
        return;
    Part::Spiral* spiral = featurePtr.get<Part::Spiral>();
    if (widget == ui->spiralGrowth) {
        spiral->Growth.setValue(ui->spiralGrowth->value().getValue());
    }
    else if (widget == ui->spiralRotation) {
        spiral->Rotations.setValue(ui->spiralRotation->value());
    }
    else if (widget == ui->spiralRadius) {
        spiral->Radius.setValue(ui->spiralRadius->value().getValue());
    }

    spiral->recomputeFeature();
}

void DlgPrimitives::onChangeCircle(QWidget* widget)
{
    if (featurePtr.expired())
        return;
    Part::Circle* circle = featurePtr.get<Part::Circle>();
    if (widget == ui->circleRadius) {
        circle->Radius.setValue(ui->circleRadius->value().getValue());
    }
    else if (widget == ui->circleAngle1) {
        circle->Angle1.setValue(ui->circleAngle1->value().getValue());
    }
    else if (widget == ui->circleAngle2) {
        circle->Angle2.setValue(ui->circleAngle2->value().getValue());
    }

    circle->recomputeFeature();
}

void DlgPrimitives::onChangeEllipse(QWidget* widget)
{
    if (featurePtr.expired())
        return;
    Part::Ellipse* ell = featurePtr.get<Part::Ellipse>();
    if (widget == ui->ellipseMajorRadius) {
        ell->MajorRadius.setValue(ui->ellipseMajorRadius->value().getValue());
    }
    else if (widget == ui->ellipseMinorRadius) {
        ell->MinorRadius.setValue(ui->ellipseMinorRadius->value().getValue());
    }
    else if (widget == ui->ellipseAngle1) {
        ell->Angle1.setValue(ui->ellipseAngle1->value().getValue());
    }
    else if (widget == ui->ellipseAngle2) {
        ell->Angle2.setValue(ui->ellipseAngle2->value().getValue());
    }

    ell->recomputeFeature();
}

void DlgPrimitives::onChangeVertex(QWidget* widget)
{
    if (featurePtr.expired())
        return;
    Part::Vertex* v = featurePtr.get<Part::Vertex>();
    if (widget == ui->vertexX) {
        v->X.setValue(ui->vertexX->value().getValue());
    }
    else if (widget == ui->vertexY) {
        v->Y.setValue(ui->vertexY->value().getValue());
    }
    else if (widget == ui->vertexZ) {
        v->Z.setValue(ui->vertexZ->value().getValue());
    }

    v->recomputeFeature();
}

void DlgPrimitives::onChangeLine(QWidget* widget)
{
    if (featurePtr.expired())
        return;
    Part::Line* line = featurePtr.get<Part::Line>();
    if (widget == ui->edgeX1) {
        line->X1.setValue(ui->edgeX1->value().getValue());
    }
    else if (widget == ui->edgeY1) {
        line->Y1.setValue(ui->edgeY1->value().getValue());
    }
    else if (widget == ui->edgeZ1) {
        line->Z1.setValue(ui->edgeZ1->value().getValue());
    }
    else if (widget == ui->edgeX2) {
        line->X2.setValue(ui->edgeX2->value().getValue());
    }
    else if (widget == ui->edgeY2) {
        line->Y2.setValue(ui->edgeY2->value().getValue());
    }
    else if (widget == ui->edgeZ2) {
        line->Z2.setValue(ui->edgeZ2->value().getValue());
    }

    line->recomputeFeature();
}

void DlgPrimitives::onChangeRegularPolygon(QWidget* widget)
{
    if (featurePtr.expired())
        return;
    Part::RegularPolygon* poly = featurePtr.get<Part::RegularPolygon>();
    if (widget == ui->regularPolygonPolygon) {
        poly->Polygon.setValue(ui->regularPolygonPolygon->value());
    }
    else if (widget == ui->regularPolygonCircumradius) {
        poly->Circumradius.setValue(ui->regularPolygonCircumradius->value().getValue());
    }

    poly->recomputeFeature();
}

// ----------------------------------------------

/* TRANSLATOR PartGui::Location */

Location::Location(QWidget* parent, Part::Feature* feature)
    : QWidget(parent)
    , ui(new Ui_Location)
    , featurePtr(feature)
{
    mode = 0;
    ui->setupUi(this);

    ui->XPositionQSB->setUnit(Base::Unit::Length);
    ui->YPositionQSB->setUnit(Base::Unit::Length);
    ui->ZPositionQSB->setUnit(Base::Unit::Length);
    ui->AngleQSB->setUnit(Base::Unit::Angle);

    // fill location widget if object already exists
    if (feature) {
        // get the placement values
        auto placement = feature->Placement.getValue();

        auto position = placement.getPosition();
        ui->XPositionQSB->setValue(position.x);
        ui->YPositionQSB->setValue(position.y);
        ui->ZPositionQSB->setValue(position.z);

        double rotationAngle;
        Base::Vector3d rotationAxes;
        auto rotation = placement.getRotation();
        rotation.getRawValue(rotationAxes, rotationAngle);
        ui->XDirectionEdit->setValue(rotationAxes.x);
        ui->YDirectionEdit->setValue(rotationAxes.y);
        ui->ZDirectionEdit->setValue(rotationAxes.z);
        // the angle is rad, transform it for display to degrees
        ui->AngleQSB->setValue(Base::toDegrees<double>(rotationAngle));

        ui->XPositionQSB->bind(App::ObjectIdentifier::parse(feature, std::string("Placement.Base.x")));
        ui->YPositionQSB->bind(App::ObjectIdentifier::parse(feature, std::string("Placement.Base.y")));
        ui->ZPositionQSB->bind(App::ObjectIdentifier::parse(feature, std::string("Placement.Base.z")));
        ui->XDirectionEdit->bind(App::ObjectIdentifier::parse(feature, std::string("Placement.Rotation.Axis.x")));
        ui->YDirectionEdit->bind(App::ObjectIdentifier::parse(feature, std::string("Placement.Rotation.Axis.y")));
        ui->ZDirectionEdit->bind(App::ObjectIdentifier::parse(feature, std::string("Placement.Rotation.Axis.z")));
        ui->AngleQSB->bind(App::ObjectIdentifier::parse(feature, std::string("Placement.Rotation.Angle")));

        //connect signals
        connect(ui->XPositionQSB, SIGNAL(valueChanged(double)), this,  SLOT(onChangePosRot()));
        connect(ui->YPositionQSB, SIGNAL(valueChanged(double)), this,  SLOT(onChangePosRot()));
        connect(ui->ZPositionQSB, SIGNAL(valueChanged(double)), this,  SLOT(onChangePosRot()));
        connect(ui->AngleQSB, SIGNAL(valueChanged(double)), this,  SLOT(onChangePosRot()));
        connect(ui->XDirectionEdit, SIGNAL(valueChanged(double)), this,  SLOT(onChangePosRot()));
        connect(ui->YDirectionEdit, SIGNAL(valueChanged(double)), this,  SLOT(onChangePosRot()));
        connect(ui->ZDirectionEdit, SIGNAL(valueChanged(double)), this,  SLOT(onChangePosRot()));
    }
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

void Location::onChangePosRot()
{
    App::GeoFeature* geom = featurePtr.get<App::GeoFeature>();
    if (!geom)
        return;

    // read dialog values
    Base::Vector3d loc;
    loc.x = ui->XPositionQSB->rawValue();
    loc.y = ui->YPositionQSB->rawValue();
    loc.z = ui->ZPositionQSB->rawValue();
    double angle = ui->AngleQSB->rawValue();
    // the angle is displayed in degrees, transform it to rad
    angle = Base::toRadians<double>(angle);
    Base::Vector3d rot;
    rot.x = ui->XDirectionEdit->value();
    rot.y = ui->YDirectionEdit->value();
    rot.z = ui->ZDirectionEdit->value();

    // set placement and rotation
    Base::Placement placement;
    Base::Rotation rotation(rot, angle);
    placement.setPosition(loc);
    placement.setRotation(rotation);

    // apply new placement to the feature
    geom->Placement.setValue(placement);
    geom->recomputeFeature();
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
    Gui::View3DInventorViewer* view  = static_cast<Gui::View3DInventorViewer*>(n->getUserData());

    // Mark all incoming mouse button events as handled, especially, to deactivate the selection node
    n->getAction()->setHandled();
    if (mbe->getButton() == SoMouseButtonEvent::BUTTON1) {
        if (mbe->getState() == SoButtonEvent::DOWN) {
            const SoPickedPoint * point = n->getPickedPoint();
            if (point) {
                SbVec3f pnt = point->getPoint();
                SbVec3f nor = point->getNormal();
                Location* dlg = static_cast<Location*>(ud);
                dlg->ui->XPositionQSB->setValue(pnt[0]);
                dlg->ui->YPositionQSB->setValue(pnt[1]);
                dlg->ui->ZPositionQSB->setValue(pnt[2]);
                dlg->ui->XDirectionEdit->setValue(nor[0]);
                dlg->ui->YDirectionEdit->setValue(nor[1]);
                dlg->ui->ZDirectionEdit->setValue(nor[2]);
                n->setHandled();
            }
        }
    }
    else if (mbe->getButton() == SoMouseButtonEvent::BUTTON2) {
        if (mbe->getState() == SoButtonEvent::UP) {
            n->setHandled();
            view->setEditing(false);
            view->setRedirectToSceneGraph(false);
            Location* dlg = static_cast<Location*>(ud);
            dlg->activeView = nullptr;
            view->removeEventCallback(SoMouseButtonEvent::getClassTypeId(), pickCallback,ud);
            SoNode* root = view->getSceneGraph();
            if (root && root->getTypeId().isDerivedFrom(Gui::SoFCUnifiedSelection::getClassTypeId()))
                static_cast<Gui::SoFCUnifiedSelection*>(root)->selectionMode.setValue(static_cast<Location*>(ud)->mode);
        }
    }
}

QString Location::toPlacement() const
{
    // create a command to set the position and angle of the primitive object

    Base::Vector3d rot;
    rot.x = ui->XDirectionEdit->value();
    rot.y = ui->YDirectionEdit->value();
    rot.z = ui->ZDirectionEdit->value();

    double angle = ui->AngleQSB->rawValue();

    Base::Vector3d loc;
    loc.x = ui->XPositionQSB->rawValue();
    loc.y = ui->YPositionQSB->rawValue();
    loc.z = ui->ZPositionQSB->rawValue();

    return QString::fromLatin1("App.Placement(App.Vector(%1,%2,%3),App.Rotation(App.Vector(%4,%5,%6),%7))")
        .arg(loc.x, 0, 'f', Base::UnitsApi::getDecimals())
        .arg(loc.y, 0, 'f', Base::UnitsApi::getDecimals())
        .arg(loc.z, 0, 'f', Base::UnitsApi::getDecimals())
        .arg(rot.x, 0, 'f', Base::UnitsApi::getDecimals())
        .arg(rot.y, 0, 'f', Base::UnitsApi::getDecimals())
        .arg(rot.z, 0, 'f', Base::UnitsApi::getDecimals())
        .arg(angle, 0, 'f', Base::UnitsApi::getDecimals());
}

// ----------------------------------------------

/* TRANSLATOR PartGui::TaskPrimitives */

TaskPrimitives::TaskPrimitives()
{
    Gui::TaskView::TaskBox* taskbox;
    widget = new DlgPrimitives();
    taskbox = new Gui::TaskView::TaskBox(QPixmap(), widget->windowTitle(), true, nullptr);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);

    location = new Location();
    taskbox = new Gui::TaskView::TaskBox(QPixmap(), location->windowTitle() ,true, nullptr);
    taskbox->groupLayout()->addWidget(location);
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

// ----------------------------------------------

/* TRANSLATOR PartGui::TaskPrimitivesEdit */

TaskPrimitivesEdit::TaskPrimitivesEdit(Part::Primitive* feature)
{
    // create and show dialog for the primitives
    Gui::TaskView::TaskBox* taskbox;
    widget = new DlgPrimitives(nullptr, feature);
    taskbox = new Gui::TaskView::TaskBox(QPixmap(), widget->windowTitle(), true, nullptr);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);

    // create and show dialog for the location
    location = new Location(nullptr, feature);
    taskbox = new Gui::TaskView::TaskBox(QPixmap(), location->windowTitle(), true, nullptr);
    taskbox->groupLayout()->addWidget(location);
    Content.push_back(taskbox);
}

TaskPrimitivesEdit::~TaskPrimitivesEdit()
{
    // automatically deleted in the sub-class
}

QDialogButtonBox::StandardButtons TaskPrimitivesEdit::getStandardButtons() const
{
    return QDialogButtonBox::Cancel |
        QDialogButtonBox::Ok;
}

bool TaskPrimitivesEdit::accept()
{
    widget->accept(location->toPlacement());
    std::string document = getDocumentName(); // needed because resetEdit() deletes this instance
    Gui::Command::doCommand(Gui::Command::Gui, "Gui.getDocument('%s').resetEdit()", document.c_str());
    return true;
}

bool TaskPrimitivesEdit::reject()
{
    widget->reject();
    std::string document = getDocumentName(); // needed because resetEdit() deletes this instance
    Gui::Command::doCommand(Gui::Command::Gui, "Gui.getDocument('%s').resetEdit()", document.c_str());
    return true;
}

#include "moc_DlgPrimitives.cpp"
