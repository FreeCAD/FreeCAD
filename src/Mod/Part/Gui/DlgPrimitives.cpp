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
#include <GC_MakeArcOfCircle.hxx>
#include <Geom_Circle.hxx>
#include <Geom_TrimmedCurve.hxx>
#include <gp_Ax3.hxx>
#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>
#include <QMessageBox>
#include <QSignalMapper>
#include <Inventor/SoPickedPoint.h>
#include <Inventor/events/SoMouseButtonEvent.h>
#endif
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
#include <App/Application.h>
#include <App/Part.h>
#include <App/Document.h>
#include <Base/Rotation.h>
#include <Base/Tools.h>
#include <Base/UnitsApi.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/Command.h>
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
                .arg(activeObjectName, objectName);
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
        QMessageBox::warning(widget, descr, QCoreApplication::translate("Exception", e.what()));
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
    bool pickedPoint(const SoPickedPoint * point) override
    {
        SbVec3f pnt = point->getPoint();
        points.emplace_back(pnt[0],pnt[1],pnt[2]);
        return points.size() == 3;
    }
    QString command(App::Document* doc) const override
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

// ----------------------------------------------------------------------------

AbstractPrimitive::AbstractPrimitive(Part::Primitive* feature)
    : featurePtr(feature)
{
}

bool AbstractPrimitive::hasValidPrimitive() const
{
    return (!featurePtr.expired());
}

void AbstractPrimitive::connectSignalMapper(QSignalMapper* mapper)
{
#if QT_VERSION < QT_VERSION_CHECK(5,15,0)
        connect(mapper, qOverload<QObject*>(&QSignalMapper::mapped), this, &AbstractPrimitive::changeValue);
#else
        connect(mapper, &QSignalMapper::mappedObject, this, &AbstractPrimitive::changeValue);
#endif
}

namespace PartGui {

void mapSignalMapper(QObject *sender, QSignalMapper* mapper)
{
    mapper->setMapping(sender, sender);
}

template <typename Function>
void connectMapSignalMapper(typename QtPrivate::FunctionPointer<Function>::Object *sender, Function func, QSignalMapper* mapper)
{
    QObject::connect(sender, func, mapper, qOverload<>(&QSignalMapper::map));
    mapSignalMapper(sender, mapper);
}
}

// ----------------------------------------------------------------------------

PlanePrimitive::PlanePrimitive(std::shared_ptr<Ui_DlgPrimitives> ui, Part::Plane* feature)
    : AbstractPrimitive(feature)
    , ui(ui)
{
    ui->planeLength->setRange(0, INT_MAX);
    ui->planeWidth->setRange(0, INT_MAX);

    if (feature) {
        ui->planeLength->setValue(feature->Length.getQuantityValue());
        ui->planeLength->bind(feature->Length);
        ui->planeWidth->setValue(feature->Width.getQuantityValue());
        ui->planeWidth->bind(feature->Width);

        QSignalMapper* mapper = new QSignalMapper(this);
        connectSignalMapper(mapper);
        connectMapSignalMapper(ui->planeLength, qOverload<double>(&Gui::QuantitySpinBox::valueChanged), mapper);
        connectMapSignalMapper(ui->planeWidth, qOverload<double>(&Gui::QuantitySpinBox::valueChanged), mapper);
    }
}

const char* PlanePrimitive::getDefaultName() const
{
    return "Plane";
}

QString PlanePrimitive::create(const QString& objectName, const QString& placement) const
{
    return QString::fromLatin1(
        "App.ActiveDocument.addObject(\"Part::Plane\",\"%1\")\n"
        "App.ActiveDocument.%1.Length='%2'\n"
        "App.ActiveDocument.%1.Width='%3'\n"
        "App.ActiveDocument.%1.Placement=%4\n"
        "App.ActiveDocument.%1.Label='%5'\n")
        .arg(objectName,
             ui->planeLength->value().getSafeUserString(),
             ui->planeWidth->value().getSafeUserString(),
             placement,
             DlgPrimitives::tr("Plane"));
}

QString PlanePrimitive::change(const QString& objectName, const QString& placement) const
{
    return QString::fromLatin1(
        "%1.Length='%2'\n"
        "%1.Width='%3'\n"
        "%1.Placement=%4\n")
        .arg(objectName,
             ui->planeLength->value().getSafeUserString(),
             ui->planeWidth->value().getSafeUserString(),
             placement);
}

void PlanePrimitive::changeValue(QObject* widget)
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

// ----------------------------------------------------------------------------

BoxPrimitive::BoxPrimitive(std::shared_ptr<Ui_DlgPrimitives> ui, Part::Box* feature)
    : AbstractPrimitive(feature)
    , ui(ui)
{
    ui->boxLength->setRange(0, INT_MAX);
    ui->boxWidth->setRange(0, INT_MAX);
    ui->boxHeight->setRange(0, INT_MAX);

    if (feature) {
        ui->boxLength->setValue(feature->Length.getQuantityValue());
        ui->boxLength->bind(feature->Length);
        ui->boxWidth->setValue(feature->Width.getQuantityValue());
        ui->boxWidth->bind(feature->Width);
        ui->boxHeight->setValue(feature->Height.getQuantityValue());
        ui->boxHeight->bind(feature->Height);

        QSignalMapper* mapper = new QSignalMapper(this);
        connectSignalMapper(mapper);
        connectMapSignalMapper(ui->boxLength, qOverload<double>(&Gui::QuantitySpinBox::valueChanged), mapper);
        connectMapSignalMapper(ui->boxWidth, qOverload<double>(&Gui::QuantitySpinBox::valueChanged), mapper);
        connectMapSignalMapper(ui->boxHeight, qOverload<double>(&Gui::QuantitySpinBox::valueChanged), mapper);
    }
}

const char* BoxPrimitive::getDefaultName() const
{
    return "Box";
}

QString BoxPrimitive::create(const QString& objectName, const QString& placement) const
{
    return QString::fromLatin1(
        "App.ActiveDocument.addObject(\"Part::Box\",\"%1\")\n"
        "App.ActiveDocument.%1.Length='%2'\n"
        "App.ActiveDocument.%1.Width='%3'\n"
        "App.ActiveDocument.%1.Height='%4'\n"
        "App.ActiveDocument.%1.Placement=%5\n"
        "App.ActiveDocument.%1.Label='%6'\n")
        .arg(objectName,
             ui->boxLength->value().getSafeUserString(),
             ui->boxWidth->value().getSafeUserString(),
             ui->boxHeight->value().getSafeUserString(),
             placement,
             DlgPrimitives::tr("Box"));
}

QString BoxPrimitive::change(const QString& objectName, const QString& placement) const
{
    return QString::fromLatin1(
        "%1.Length='%2'\n"
        "%1.Width='%3'\n"
        "%1.Height='%4'\n"
        "%1.Placement=%5\n")
        .arg(objectName,
             ui->boxLength->value().getSafeUserString(),
             ui->boxWidth->value().getSafeUserString(),
             ui->boxHeight->value().getSafeUserString(),
             placement);
}

void BoxPrimitive::changeValue(QObject* widget)
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

// ----------------------------------------------------------------------------

CylinderPrimitive::CylinderPrimitive(std::shared_ptr<Ui_DlgPrimitives> ui, Part::Cylinder* feature)
    : AbstractPrimitive(feature)
    , ui(ui)
{
    ui->cylinderRadius->setRange(0, INT_MAX);
    ui->cylinderHeight->setRange(0, INT_MAX);
    ui->cylinderAngle->setRange(0, 360);

    if (feature) {
        ui->cylinderRadius->setValue(feature->Radius.getQuantityValue());
        ui->cylinderRadius->bind(feature->Radius);
        ui->cylinderHeight->setValue(feature->Height.getQuantityValue());
        ui->cylinderHeight->bind(feature->Height);
        ui->cylinderXSkew->setValue(feature->FirstAngle.getQuantityValue());
        ui->cylinderXSkew->bind(feature->FirstAngle);
        ui->cylinderYSkew->setValue(feature->SecondAngle.getQuantityValue());
        ui->cylinderYSkew->bind(feature->SecondAngle);
        ui->cylinderAngle->setValue(feature->Angle.getQuantityValue());
        ui->cylinderAngle->bind(feature->Angle);

        QSignalMapper* mapper = new QSignalMapper(this);
        connectSignalMapper(mapper);
        connectMapSignalMapper(ui->cylinderRadius, qOverload<double>(&Gui::QuantitySpinBox::valueChanged), mapper);
        connectMapSignalMapper(ui->cylinderHeight, qOverload<double>(&Gui::QuantitySpinBox::valueChanged), mapper);
        connectMapSignalMapper(ui->cylinderXSkew, qOverload<double>(&Gui::QuantitySpinBox::valueChanged), mapper);
        connectMapSignalMapper(ui->cylinderYSkew, qOverload<double>(&Gui::QuantitySpinBox::valueChanged), mapper);
        connectMapSignalMapper(ui->cylinderAngle, qOverload<double>(&Gui::QuantitySpinBox::valueChanged), mapper);
    }
}

const char* CylinderPrimitive::getDefaultName() const
{
    return "Cylinder";
}

QString CylinderPrimitive::create(const QString& objectName, const QString& placement) const
{
    return QString::fromLatin1(
        "App.ActiveDocument.addObject(\"Part::Cylinder\",\"%1\")\n"
        "App.ActiveDocument.%1.Radius='%2'\n"
        "App.ActiveDocument.%1.Height='%3'\n"
        "App.ActiveDocument.%1.Angle='%4'\n"
        "App.ActiveDocument.%1.FirstAngle='%5'\n"
        "App.ActiveDocument.%1.SecondAngle='%6'\n"
        "App.ActiveDocument.%1.Placement=%7\n"
        "App.ActiveDocument.%1.Label='%8'\n")
        .arg(objectName,
             ui->cylinderRadius->value().getSafeUserString(),
             ui->cylinderHeight->value().getSafeUserString(),
             ui->cylinderAngle->value().getSafeUserString(),
             ui->cylinderXSkew->value().getSafeUserString(),
             ui->cylinderYSkew->value().getSafeUserString(),
             placement,
             DlgPrimitives::tr("Cylinder"));
}

QString CylinderPrimitive::change(const QString& objectName, const QString& placement) const
{
    return QString::fromLatin1(
        "%1.Radius='%2'\n"
        "%1.Height='%3'\n"
        "%1.Angle='%4'\n"
        "%1.FirstAngle='%5'\n"
        "%1.SecondAngle='%6'\n"
        "%1.Placement=%7\n")
        .arg(objectName,
             ui->cylinderRadius->value().getSafeUserString(),
             ui->cylinderHeight->value().getSafeUserString(),
             ui->cylinderAngle->value().getSafeUserString(),
             ui->cylinderXSkew->value().getSafeUserString(),
             ui->cylinderYSkew->value().getSafeUserString(),
             placement);
}

void CylinderPrimitive::changeValue(QObject* widget)
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

// ----------------------------------------------------------------------------

ConePrimitive::ConePrimitive(std::shared_ptr<Ui_DlgPrimitives> ui, Part::Cone* feature)
    : AbstractPrimitive(feature)
    , ui(ui)
{
    ui->coneRadius1->setRange(0, INT_MAX);
    ui->coneRadius2->setRange(0, INT_MAX);
    ui->coneHeight->setRange(0, INT_MAX);
    ui->coneAngle->setRange(0, 360);

    if (feature) {
        ui->coneRadius1->setValue(feature->Radius1.getQuantityValue());
        ui->coneRadius1->bind(feature->Radius1);
        ui->coneRadius2->setValue(feature->Radius2.getQuantityValue());
        ui->coneRadius2->bind(feature->Radius2);
        ui->coneHeight->setValue(feature->Height.getQuantityValue());
        ui->coneHeight->bind(feature->Height);
        ui->coneAngle->setValue(feature->Angle.getQuantityValue());
        ui->coneAngle->bind(feature->Angle);

        QSignalMapper* mapper = new QSignalMapper(this);
        connectSignalMapper(mapper);
        connectMapSignalMapper(ui->coneRadius1, qOverload<double>(&Gui::QuantitySpinBox::valueChanged), mapper);
        connectMapSignalMapper(ui->coneRadius2, qOverload<double>(&Gui::QuantitySpinBox::valueChanged), mapper);
        connectMapSignalMapper(ui->coneHeight, qOverload<double>(&Gui::QuantitySpinBox::valueChanged), mapper);
        connectMapSignalMapper(ui->coneAngle, qOverload<double>(&Gui::QuantitySpinBox::valueChanged), mapper);
    }
}

const char* ConePrimitive::getDefaultName() const
{
    return "Cone";
}

QString ConePrimitive::create(const QString& objectName, const QString& placement) const
{
    return QString::fromLatin1(
        "App.ActiveDocument.addObject(\"Part::Cone\",\"%1\")\n"
        "App.ActiveDocument.%1.Radius1='%2'\n"
        "App.ActiveDocument.%1.Radius2='%3'\n"
        "App.ActiveDocument.%1.Height='%4'\n"
        "App.ActiveDocument.%1.Angle='%5'\n"
        "App.ActiveDocument.%1.Placement=%6\n"
        "App.ActiveDocument.%1.Label='%7'\n")
        .arg(objectName,
             ui->coneRadius1->value().getSafeUserString(),
             ui->coneRadius2->value().getSafeUserString(),
             ui->coneHeight->value().getSafeUserString(),
             ui->coneAngle->value().getSafeUserString(),
             placement,
             DlgPrimitives::tr("Cone"));
}

QString ConePrimitive::change(const QString& objectName, const QString& placement) const
{
    return QString::fromLatin1(
        "%1.Radius1='%2'\n"
        "%1.Radius2='%3'\n"
        "%1.Height='%4'\n"
        "%1.Angle='%5'\n"
        "%1.Placement=%6\n")
        .arg(objectName,
             ui->coneRadius1->value().getSafeUserString(),
             ui->coneRadius2->value().getSafeUserString(),
             ui->coneHeight->value().getSafeUserString(),
             ui->coneAngle->value().getSafeUserString(),
             placement);
}

void ConePrimitive::changeValue(QObject* widget)
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

// ----------------------------------------------------------------------------

SpherePrimitive::SpherePrimitive(std::shared_ptr<Ui_DlgPrimitives> ui, Part::Sphere* feature)
    : AbstractPrimitive(feature)
    , ui(ui)
{
    ui->sphereRadius->setRange(0, INT_MAX);
    ui->sphereAngle1->setRange(-90, 90);
    ui->sphereAngle2->setRange(-90, 90);
    ui->sphereAngle3->setRange(0, 360);

    if (feature) {
        ui->sphereRadius->setValue(feature->Radius.getQuantityValue());
        ui->sphereRadius->bind(feature->Radius);
        ui->sphereAngle1->setValue(feature->Angle1.getQuantityValue());
        ui->sphereAngle1->bind(feature->Angle1);
        ui->sphereAngle2->setValue(feature->Angle2.getQuantityValue());
        ui->sphereAngle2->bind(feature->Angle2);
        ui->sphereAngle3->setValue(feature->Angle3.getQuantityValue());
        ui->sphereAngle3->bind(feature->Angle3);

        QSignalMapper* mapper = new QSignalMapper(this);
        connectSignalMapper(mapper);
        connectMapSignalMapper(ui->sphereRadius, qOverload<double>(&Gui::QuantitySpinBox::valueChanged), mapper);
        connectMapSignalMapper(ui->sphereAngle1, qOverload<double>(&Gui::QuantitySpinBox::valueChanged), mapper);
        connectMapSignalMapper(ui->sphereAngle2, qOverload<double>(&Gui::QuantitySpinBox::valueChanged), mapper);
        connectMapSignalMapper(ui->sphereAngle3, qOverload<double>(&Gui::QuantitySpinBox::valueChanged), mapper);
    }
}

const char* SpherePrimitive::getDefaultName() const
{
    return "Sphere";
}

QString SpherePrimitive::create(const QString& objectName, const QString& placement) const
{
    return QString::fromLatin1(
        "App.ActiveDocument.addObject(\"Part::Sphere\",\"%1\")\n"
        "App.ActiveDocument.%1.Radius='%2'\n"
        "App.ActiveDocument.%1.Angle1='%3'\n"
        "App.ActiveDocument.%1.Angle2='%4'\n"
        "App.ActiveDocument.%1.Angle3='%5'\n"
        "App.ActiveDocument.%1.Placement=%6\n"
        "App.ActiveDocument.%1.Label='%7'\n")
        .arg(objectName,
             ui->sphereRadius->value().getSafeUserString(),
             ui->sphereAngle1->value().getSafeUserString(),
             ui->sphereAngle2->value().getSafeUserString(),
             ui->sphereAngle3->value().getSafeUserString(),
             placement,
             DlgPrimitives::tr("Sphere"));
}

QString SpherePrimitive::change(const QString& objectName, const QString& placement) const
{
    return QString::fromLatin1(
        "%1.Radius='%2'\n"
        "%1.Angle1='%3'\n"
        "%1.Angle2='%4'\n"
        "%1.Angle3='%5'\n"
        "%1.Placement=%6\n")
        .arg(objectName,
             ui->sphereRadius->value().getSafeUserString(),
             ui->sphereAngle1->value().getSafeUserString(),
             ui->sphereAngle2->value().getSafeUserString(),
             ui->sphereAngle3->value().getSafeUserString(),
             placement);
}

void SpherePrimitive::changeValue(QObject* widget)
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

// ----------------------------------------------------------------------------

EllipsoidPrimitive::EllipsoidPrimitive(std::shared_ptr<Ui_DlgPrimitives> ui, Part::Ellipsoid* feature)
    : AbstractPrimitive(feature)
    , ui(ui)
{
    ui->ellipsoidRadius1->setRange(0, INT_MAX);
    ui->ellipsoidRadius2->setRange(0, INT_MAX);
    ui->ellipsoidRadius3->setRange(0, INT_MAX);
    ui->ellipsoidAngle1->setRange(-90, 90);
    ui->ellipsoidAngle2->setRange(-90, 90);
    ui->ellipsoidAngle3->setRange(0, 360);

    if (feature) {
        ui->ellipsoidRadius1->setValue(feature->Radius1.getQuantityValue());
        ui->ellipsoidRadius1->bind(feature->Radius1);
        ui->ellipsoidRadius2->setValue(feature->Radius2.getQuantityValue());
        ui->ellipsoidRadius2->bind(feature->Radius2);
        ui->ellipsoidRadius3->setValue(feature->Radius3.getQuantityValue());
        ui->ellipsoidRadius3->bind(feature->Radius3);
        ui->ellipsoidAngle1->setValue(feature->Angle1.getQuantityValue());
        ui->ellipsoidAngle1->bind(feature->Angle1);
        ui->ellipsoidAngle2->setValue(feature->Angle2.getQuantityValue());
        ui->ellipsoidAngle2->bind(feature->Angle2);
        ui->ellipsoidAngle3->setValue(feature->Angle3.getQuantityValue());
        ui->ellipsoidAngle3->bind(feature->Angle3);

        QSignalMapper* mapper = new QSignalMapper(this);
        connectSignalMapper(mapper);
        connectMapSignalMapper(ui->ellipsoidRadius1, qOverload<double>(&Gui::QuantitySpinBox::valueChanged), mapper);
        connectMapSignalMapper(ui->ellipsoidRadius2, qOverload<double>(&Gui::QuantitySpinBox::valueChanged), mapper);
        connectMapSignalMapper(ui->ellipsoidRadius3, qOverload<double>(&Gui::QuantitySpinBox::valueChanged), mapper);
        connectMapSignalMapper(ui->ellipsoidAngle1, qOverload<double>(&Gui::QuantitySpinBox::valueChanged), mapper);
        connectMapSignalMapper(ui->ellipsoidAngle2, qOverload<double>(&Gui::QuantitySpinBox::valueChanged), mapper);
        connectMapSignalMapper(ui->ellipsoidAngle3, qOverload<double>(&Gui::QuantitySpinBox::valueChanged), mapper);

    }
}

const char* EllipsoidPrimitive::getDefaultName() const
{
    return "Ellipsoid";
}

QString EllipsoidPrimitive::create(const QString& objectName, const QString& placement) const
{
    return QString::fromLatin1(
        "App.ActiveDocument.addObject(\"Part::Ellipsoid\",\"%1\")\n"
        "App.ActiveDocument.%1.Radius1='%2'\n"
        "App.ActiveDocument.%1.Radius2='%3'\n"
        "App.ActiveDocument.%1.Radius3='%4'\n"
        "App.ActiveDocument.%1.Angle1='%5'\n"
        "App.ActiveDocument.%1.Angle2='%6'\n"
        "App.ActiveDocument.%1.Angle3='%7'\n"
        "App.ActiveDocument.%1.Placement=%8\n"
        "App.ActiveDocument.%1.Label='%9'\n")
        .arg(objectName,
             ui->ellipsoidRadius1->value().getSafeUserString(),
             ui->ellipsoidRadius2->value().getSafeUserString(),
             ui->ellipsoidRadius3->value().getSafeUserString(),
             ui->ellipsoidAngle1->value().getSafeUserString(),
             ui->ellipsoidAngle2->value().getSafeUserString(),
             ui->ellipsoidAngle3->value().getSafeUserString(),
             placement,
             DlgPrimitives::tr("Ellipsoid"));
}

QString EllipsoidPrimitive::change(const QString& objectName, const QString& placement) const
{
    return QString::fromLatin1(
        "%1.Radius1='%2'\n"
        "%1.Radius2='%3'\n"
        "%1.Radius3='%4'\n"
        "%1.Angle1='%5'\n"
        "%1.Angle2='%6'\n"
        "%1.Angle3='%7'\n"
        "%1.Placement=%8\n")
        .arg(objectName,
             ui->ellipsoidRadius1->value().getSafeUserString(),
             ui->ellipsoidRadius2->value().getSafeUserString(),
             ui->ellipsoidRadius3->value().getSafeUserString(),
             ui->ellipsoidAngle1->value().getSafeUserString(),
             ui->ellipsoidAngle2->value().getSafeUserString(),
             ui->ellipsoidAngle3->value().getSafeUserString(),
             placement);
}

void EllipsoidPrimitive::changeValue(QObject* widget)
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

// ----------------------------------------------------------------------------

TorusPrimitive::TorusPrimitive(std::shared_ptr<Ui_DlgPrimitives> ui, Part::Torus* feature)
    : AbstractPrimitive(feature)
    , ui(ui)
{
    ui->torusRadius1->setRange(0, INT_MAX);
    ui->torusRadius2->setRange(0, INT_MAX);
    ui->torusAngle1->setRange(-180, 180);
    ui->torusAngle2->setRange(-180, 180);
    ui->torusAngle3->setRange(0, 360);

    if (feature) {
        ui->torusRadius1->setValue(feature->Radius1.getQuantityValue());
        ui->torusRadius1->bind(feature->Radius1);
        ui->torusRadius2->setValue(feature->Radius2.getQuantityValue());
        ui->torusRadius2->bind(feature->Radius2);
        ui->torusAngle1->setValue(feature->Angle1.getQuantityValue());
        ui->torusAngle1->bind(feature->Angle1);
        ui->torusAngle2->setValue(feature->Angle2.getQuantityValue());
        ui->torusAngle2->bind(feature->Angle2);
        ui->torusAngle3->setValue(feature->Angle3.getQuantityValue());
        ui->torusAngle3->bind(feature->Angle3);

        QSignalMapper* mapper = new QSignalMapper(this);
        connectSignalMapper(mapper);
        connectMapSignalMapper(ui->torusRadius1, qOverload<double>(&Gui::QuantitySpinBox::valueChanged), mapper);
        connectMapSignalMapper(ui->torusRadius2, qOverload<double>(&Gui::QuantitySpinBox::valueChanged), mapper);
        connectMapSignalMapper(ui->torusAngle1, qOverload<double>(&Gui::QuantitySpinBox::valueChanged), mapper);
        connectMapSignalMapper(ui->torusAngle2, qOverload<double>(&Gui::QuantitySpinBox::valueChanged), mapper);
        connectMapSignalMapper(ui->torusAngle3, qOverload<double>(&Gui::QuantitySpinBox::valueChanged), mapper);
    }
}

const char* TorusPrimitive::getDefaultName() const
{
    return "Torus";
}

QString TorusPrimitive::create(const QString& objectName, const QString& placement) const
{
    return QString::fromLatin1(
        "App.ActiveDocument.addObject(\"Part::Torus\",\"%1\")\n"
        "App.ActiveDocument.%1.Radius1='%2'\n"
        "App.ActiveDocument.%1.Radius2='%3'\n"
        "App.ActiveDocument.%1.Angle1='%4'\n"
        "App.ActiveDocument.%1.Angle2='%5'\n"
        "App.ActiveDocument.%1.Angle3='%6'\n"
        "App.ActiveDocument.%1.Placement=%7\n"
        "App.ActiveDocument.%1.Label='%8'\n")
        .arg(objectName,
             ui->torusRadius1->value().getSafeUserString(),
             ui->torusRadius2->value().getSafeUserString(),
             ui->torusAngle1->value().getSafeUserString(),
             ui->torusAngle2->value().getSafeUserString(),
             ui->torusAngle3->value().getSafeUserString(),
             placement,
             DlgPrimitives::tr("Torus"));
}

QString TorusPrimitive::change(const QString& objectName, const QString& placement) const
{
    return QString::fromLatin1(
        "%1.Radius1='%2'\n"
        "%1.Radius2='%3'\n"
        "%1.Angle1='%4'\n"
        "%1.Angle2='%5'\n"
        "%1.Angle3='%6'\n"
        "%1.Placement=%7\n")
        .arg(objectName,
             ui->torusRadius1->value().getSafeUserString(),
             ui->torusRadius2->value().getSafeUserString(),
             ui->torusAngle1->value().getSafeUserString(),
             ui->torusAngle2->value().getSafeUserString(),
             ui->torusAngle3->value().getSafeUserString(),
             placement);
}

void TorusPrimitive::changeValue(QObject* widget)
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

// ----------------------------------------------------------------------------

PrismPrimitive::PrismPrimitive(std::shared_ptr<Ui_DlgPrimitives> ui, Part::Prism* feature)
    : AbstractPrimitive(feature)
    , ui(ui)
{
    ui->prismCircumradius->setRange(0, INT_MAX);
    ui->prismHeight->setRange(0, INT_MAX);

    if (feature) {
        ui->prismPolygon->setValue(feature->Polygon.getValue());
        ui->prismCircumradius->setValue(feature->Circumradius.getQuantityValue());
        ui->prismCircumradius->bind(feature->Circumradius);
        ui->prismHeight->setValue(feature->Height.getQuantityValue());
        ui->prismHeight->bind(feature->Height);
        ui->prismXSkew->setValue(feature->FirstAngle.getQuantityValue());
        ui->prismXSkew->bind(feature->FirstAngle);
        ui->prismYSkew->setValue(feature->SecondAngle.getQuantityValue());
        ui->prismYSkew->bind(feature->SecondAngle);

        QSignalMapper* mapper = new QSignalMapper(this);
        connectSignalMapper(mapper);
        connectMapSignalMapper(ui->prismPolygon, qOverload<int>(&QSpinBox::valueChanged), mapper);
        connectMapSignalMapper(ui->prismCircumradius, qOverload<double>(&Gui::QuantitySpinBox::valueChanged), mapper);
        connectMapSignalMapper(ui->prismHeight, qOverload<double>(&Gui::QuantitySpinBox::valueChanged), mapper);
        connectMapSignalMapper(ui->prismXSkew, qOverload<double>(&Gui::QuantitySpinBox::valueChanged), mapper);
        connectMapSignalMapper(ui->prismYSkew, qOverload<double>(&Gui::QuantitySpinBox::valueChanged), mapper);
    }
}

const char* PrismPrimitive::getDefaultName() const
{
    return "Prism";
}

QString PrismPrimitive::create(const QString& objectName, const QString& placement) const
{
    return QString::fromLatin1(
        "App.ActiveDocument.addObject(\"Part::Prism\",\"%1\")\n"
        "App.ActiveDocument.%1.Polygon=%2\n"
        "App.ActiveDocument.%1.Circumradius='%3'\n"
        "App.ActiveDocument.%1.Height='%4'\n"
        "App.ActiveDocument.%1.FirstAngle='%5'\n"
        "App.ActiveDocument.%1.SecondAngle='%6'\n"
        "App.ActiveDocument.%1.Placement=%7\n"
        "App.ActiveDocument.%1.Label='%8'\n")
        .arg(objectName,
             QString::number(ui->prismPolygon->value()),
             ui->prismCircumradius->value().getSafeUserString(),
             ui->prismHeight->value().getSafeUserString(),
             ui->prismXSkew->value().getSafeUserString(),
             ui->prismYSkew->value().getSafeUserString(),
             placement,
             DlgPrimitives::tr("Prism"));
}

QString PrismPrimitive::change(const QString& objectName, const QString& placement) const
{
    return QString::fromLatin1(
        "%1.Polygon=%2\n"
        "%1.Circumradius='%3'\n"
        "%1.Height='%4'\n"
        "%1.FirstAngle='%5'\n"
        "%1.SecondAngle='%6'\n"
        "%1.Placement=%7\n")
        .arg(objectName,
             QString::number(ui->prismPolygon->value()),
             ui->prismCircumradius->value().getSafeUserString(),
             ui->prismHeight->value().getSafeUserString(),
             ui->prismXSkew->value().getSafeUserString(),
             ui->prismYSkew->value().getSafeUserString(),
             placement);
}

void PrismPrimitive::changeValue(QObject* widget)
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

// ----------------------------------------------------------------------------

WedgePrimitive::WedgePrimitive(std::shared_ptr<Ui_DlgPrimitives> ui, Part::Wedge* feature)
    : AbstractPrimitive(feature)
    , ui(ui)
{
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

    if (feature) {
        ui->wedgeXmin->setValue(feature->Xmin.getQuantityValue());
        ui->wedgeXmin->bind(feature->Xmin);
        ui->wedgeYmin->setValue(feature->Ymin.getQuantityValue());
        ui->wedgeYmin->bind(feature->Ymin);
        ui->wedgeZmin->setValue(feature->Zmin.getQuantityValue());
        ui->wedgeZmin->bind(feature->Zmin);
        ui->wedgeX2min->setValue(feature->X2min.getQuantityValue());
        ui->wedgeX2min->bind(feature->X2min);
        ui->wedgeZ2min->setValue(feature->Z2min.getQuantityValue());
        ui->wedgeZ2min->bind(feature->Z2min);
        ui->wedgeXmax->setValue(feature->Xmax.getQuantityValue());
        ui->wedgeXmax->bind(feature->Xmax);
        ui->wedgeYmax->setValue(feature->Ymax.getQuantityValue());
        ui->wedgeYmax->bind(feature->Ymax);
        ui->wedgeZmax->setValue(feature->Zmax.getQuantityValue());
        ui->wedgeZmax->bind(feature->Zmax);
        ui->wedgeX2max->setValue(feature->X2max.getQuantityValue());
        ui->wedgeX2max->bind(feature->X2max);
        ui->wedgeZ2max->setValue(feature->Z2max.getQuantityValue());
        ui->wedgeZ2max->bind(feature->Z2max);

        QSignalMapper* mapper = new QSignalMapper(this);
        connectSignalMapper(mapper);
        connectMapSignalMapper(ui->wedgeXmin, qOverload<double>(&Gui::QuantitySpinBox::valueChanged), mapper);
        connectMapSignalMapper(ui->wedgeYmin, qOverload<double>(&Gui::QuantitySpinBox::valueChanged), mapper);
        connectMapSignalMapper(ui->wedgeZmin, qOverload<double>(&Gui::QuantitySpinBox::valueChanged), mapper);
        connectMapSignalMapper(ui->wedgeX2min, qOverload<double>(&Gui::QuantitySpinBox::valueChanged), mapper);
        connectMapSignalMapper(ui->wedgeZ2min, qOverload<double>(&Gui::QuantitySpinBox::valueChanged), mapper);
        connectMapSignalMapper(ui->wedgeXmax, qOverload<double>(&Gui::QuantitySpinBox::valueChanged), mapper);
        connectMapSignalMapper(ui->wedgeYmax, qOverload<double>(&Gui::QuantitySpinBox::valueChanged), mapper);
        connectMapSignalMapper(ui->wedgeZmax, qOverload<double>(&Gui::QuantitySpinBox::valueChanged), mapper);
        connectMapSignalMapper(ui->wedgeX2max, qOverload<double>(&Gui::QuantitySpinBox::valueChanged), mapper);
        connectMapSignalMapper(ui->wedgeZ2max, qOverload<double>(&Gui::QuantitySpinBox::valueChanged), mapper);
    }
}

const char* WedgePrimitive::getDefaultName() const
{
    return "Wedge";
}

QString WedgePrimitive::create(const QString& objectName, const QString& placement) const
{
    return QString::fromLatin1(
        "App.ActiveDocument.addObject(\"Part::Wedge\",\"%1\")\n"
        "App.ActiveDocument.%1.Xmin='%2'\n"
        "App.ActiveDocument.%1.Ymin='%3'\n"
        "App.ActiveDocument.%1.Zmin='%4'\n"
        "App.ActiveDocument.%1.X2min='%5'\n"
        "App.ActiveDocument.%1.Z2min='%6'\n"
        "App.ActiveDocument.%1.Xmax='%7'\n"
        "App.ActiveDocument.%1.Ymax='%8'\n"
        "App.ActiveDocument.%1.Zmax='%9'\n"
        "App.ActiveDocument.%1.X2max='%10'\n"
        "App.ActiveDocument.%1.Z2max='%11'\n"
        "App.ActiveDocument.%1.Placement=%12\n"
        "App.ActiveDocument.%1.Label='%13'\n")
        .arg(objectName,
             ui->wedgeXmin->value().getSafeUserString(),
             ui->wedgeYmin->value().getSafeUserString(),
             ui->wedgeZmin->value().getSafeUserString(),
             ui->wedgeX2min->value().getSafeUserString(),
             ui->wedgeZ2min->value().getSafeUserString(),
             ui->wedgeXmax->value().getSafeUserString(),
             ui->wedgeYmax->value().getSafeUserString())
        .arg(ui->wedgeZmax->value().getSafeUserString(),
             ui->wedgeX2max->value().getSafeUserString(),
             ui->wedgeZ2max->value().getSafeUserString(),
             placement,
             DlgPrimitives::tr("Wedge"));
}

QString WedgePrimitive::change(const QString& objectName, const QString& placement) const
{
    return QString::fromLatin1(
        "%1.Xmin='%2'\n"
        "%1.Ymin='%3'\n"
        "%1.Zmin='%4'\n"
        "%1.X2min='%5'\n"
        "%1.Z2min='%6'\n"
        "%1.Xmax='%7'\n"
        "%1.Ymax='%8'\n"
        "%1.Zmax='%9'\n"
        "%1.X2max='%10'\n"
        "%1.Z2max='%11'\n"
        "%1.Placement=%12\n")
        .arg(objectName,
             ui->wedgeXmin->value().getSafeUserString(),
             ui->wedgeYmin->value().getSafeUserString(),
             ui->wedgeZmin->value().getSafeUserString(),
             ui->wedgeX2min->value().getSafeUserString(),
             ui->wedgeZ2min->value().getSafeUserString(),
             ui->wedgeXmax->value().getSafeUserString(),
             ui->wedgeYmax->value().getSafeUserString(),
             ui->wedgeZmax->value().getSafeUserString())
        .arg(ui->wedgeX2max->value().getSafeUserString(),
             ui->wedgeZ2max->value().getSafeUserString(),
             placement);
}

void WedgePrimitive::changeValue(QObject* widget)
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

// ----------------------------------------------------------------------------

HelixPrimitive::HelixPrimitive(std::shared_ptr<Ui_DlgPrimitives> ui, Part::Helix* feature)
    : AbstractPrimitive(feature)
    , ui(ui)
{
    ui->helixPitch->setRange(0, INT_MAX);
    ui->helixHeight->setRange(0, INT_MAX);
    ui->helixRadius->setRange(0, INT_MAX);
    ui->helixAngle->setRange(-90, 90);

    if (feature) {
        ui->helixPitch->setValue(feature->Pitch.getQuantityValue());
        ui->helixPitch->bind(feature->Pitch);
        ui->helixHeight->setValue(feature->Height.getQuantityValue());
        ui->helixHeight->bind(feature->Height);
        ui->helixRadius->setValue(feature->Radius.getQuantityValue());
        ui->helixRadius->bind(feature->Radius);
        ui->helixAngle->setValue(feature->Angle.getQuantityValue());
        ui->helixAngle->bind(feature->Angle);
        ui->helixLocalCS->setCurrentIndex(feature->LocalCoord.getValue());

        QSignalMapper* mapper = new QSignalMapper(this);
        connectSignalMapper(mapper);
        connectMapSignalMapper(ui->helixPitch, qOverload<double>(&Gui::QuantitySpinBox::valueChanged), mapper);
        connectMapSignalMapper(ui->helixHeight, qOverload<double>(&Gui::QuantitySpinBox::valueChanged), mapper);
        connectMapSignalMapper(ui->helixRadius, qOverload<double>(&Gui::QuantitySpinBox::valueChanged), mapper);
        connectMapSignalMapper(ui->helixAngle, qOverload<double>(&Gui::QuantitySpinBox::valueChanged), mapper);
        connectMapSignalMapper(ui->helixLocalCS, qOverload<int>(&QComboBox::currentIndexChanged), mapper);
    }
}

const char* HelixPrimitive::getDefaultName() const
{
    return "Helix";
}

QString HelixPrimitive::create(const QString& objectName, const QString& placement) const
{
    return QString::fromLatin1(
        "App.ActiveDocument.addObject(\"Part::Helix\",\"%1\")\n"
        "App.ActiveDocument.%1.Pitch='%2'\n"
        "App.ActiveDocument.%1.Height='%3'\n"
        "App.ActiveDocument.%1.Radius='%4'\n"
        "App.ActiveDocument.%1.Angle='%5'\n"
        "App.ActiveDocument.%1.LocalCoord=%6\n"
        "App.ActiveDocument.%1.Style=1\n"
        "App.ActiveDocument.%1.Placement=%7\n"
        "App.ActiveDocument.%1.Label='%8'\n")
        .arg(objectName,
             ui->helixPitch->value().getSafeUserString(),
             ui->helixHeight->value().getSafeUserString(),
             ui->helixRadius->value().getSafeUserString(),
             ui->helixAngle->value().getSafeUserString(),
             QString::number(ui->helixLocalCS->currentIndex()),
             placement,
             DlgPrimitives::tr("Helix"));
}

QString HelixPrimitive::change(const QString& objectName, const QString& placement) const
{
    return QString::fromLatin1(
        "%1.Pitch='%2'\n"
        "%1.Height='%3'\n"
        "%1.Radius='%4'\n"
        "%1.Angle='%5'\n"
        "%1.LocalCoord=%6\n"
        "%1.Placement=%7\n")
        .arg(objectName,
             ui->helixPitch->value().getSafeUserString(),
             ui->helixHeight->value().getSafeUserString(),
             ui->helixRadius->value().getSafeUserString(),
             ui->helixAngle->value().getSafeUserString(),
             QString::number(ui->helixLocalCS->currentIndex()),
             placement);
}

void HelixPrimitive::changeValue(QObject* widget)
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

// ----------------------------------------------------------------------------

SpiralPrimitive::SpiralPrimitive(std::shared_ptr<Ui_DlgPrimitives> ui, Part::Spiral* feature)
    : AbstractPrimitive(feature)
    , ui(ui)
{
    ui->spiralGrowth->setRange(0, INT_MAX);
    ui->spiralRotation->setRange(0, INT_MAX);
    ui->spiralRadius->setRange(0, INT_MAX);

    if (feature) {
        ui->spiralGrowth->setValue(feature->Growth.getQuantityValue());
        ui->spiralGrowth->bind(feature->Growth);
        ui->spiralRotation->setValue(feature->Rotations.getQuantityValue().getValue());
        ui->spiralRadius->setValue(feature->Radius.getQuantityValue());
        ui->spiralRadius->bind(feature->Radius);

        QSignalMapper* mapper = new QSignalMapper(this);
        connectSignalMapper(mapper);
        connectMapSignalMapper(ui->spiralGrowth, qOverload<double>(&Gui::QuantitySpinBox::valueChanged), mapper);
        connectMapSignalMapper(ui->spiralRotation, qOverload<double>(&QDoubleSpinBox::valueChanged), mapper);
        connectMapSignalMapper(ui->spiralRadius, qOverload<double>(&Gui::QuantitySpinBox::valueChanged), mapper);
    }
}

const char* SpiralPrimitive::getDefaultName() const
{
    return "Spiral";
}

QString SpiralPrimitive::create(const QString& objectName, const QString& placement) const
{
    return QString::fromLatin1(
        "App.ActiveDocument.addObject(\"Part::Spiral\",\"%1\")\n"
        "App.ActiveDocument.%1.Growth='%2'\n"
        "App.ActiveDocument.%1.Rotations=%3\n"
        "App.ActiveDocument.%1.Radius='%4'\n"
        "App.ActiveDocument.%1.Placement=%5\n"
        "App.ActiveDocument.%1.Label='%6'\n")
        .arg(objectName,
             ui->spiralGrowth->value().getSafeUserString(),
             QString::number(ui->spiralRotation->value()),
             ui->spiralRadius->value().getSafeUserString(),
             placement,
             DlgPrimitives::tr("Spiral"));
}

QString SpiralPrimitive::change(const QString& objectName, const QString& placement) const
{
    return QString::fromLatin1(
        "%1.Growth='%2'\n"
        "%1.Rotations=%3\n"
        "%1.Radius='%4'\n"
        "%1.Placement=%5\n")
        .arg(objectName,
             ui->spiralGrowth->value().getSafeUserString(),
             QString::number(ui->spiralRotation->value()),
             ui->spiralRadius->value().getSafeUserString(),
             placement);
}

void SpiralPrimitive::changeValue(QObject* widget)
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

// ----------------------------------------------------------------------------

CirclePrimitive::CirclePrimitive(std::shared_ptr<Ui_DlgPrimitives> ui, Part::Circle* feature)
    : AbstractPrimitive(feature)
    , ui(ui)
{
    ui->circleRadius->setRange(0, INT_MAX);
    ui->circleAngle1->setRange(0, 360);
    ui->circleAngle2->setRange(0, 360);

    if (feature) {
        ui->circleRadius->setValue(feature->Radius.getQuantityValue());
        ui->circleRadius->bind(feature->Radius);
        ui->circleAngle1->setValue(feature->Angle1.getQuantityValue());
        ui->circleAngle1->bind(feature->Angle1);
        ui->circleAngle2->setValue(feature->Angle2.getQuantityValue());
        ui->circleAngle2->bind(feature->Angle2);

        QSignalMapper* mapper = new QSignalMapper(this);
        connectSignalMapper(mapper);
        connectMapSignalMapper(ui->circleRadius, qOverload<double>(&Gui::QuantitySpinBox::valueChanged), mapper);
        connectMapSignalMapper(ui->circleAngle1, qOverload<double>(&Gui::QuantitySpinBox::valueChanged), mapper);
        connectMapSignalMapper(ui->circleAngle2, qOverload<double>(&Gui::QuantitySpinBox::valueChanged), mapper);
    }
}

const char* CirclePrimitive::getDefaultName() const
{
    return "Circle";
}

QString CirclePrimitive::create(const QString& objectName, const QString& placement) const
{
    return QString::fromLatin1(
        "App.ActiveDocument.addObject(\"Part::Circle\",\"%1\")\n"
        "App.ActiveDocument.%1.Radius='%2'\n"
        "App.ActiveDocument.%1.Angle1='%3'\n"
        "App.ActiveDocument.%1.Angle2='%4'\n"
        "App.ActiveDocument.%1.Placement=%5\n"
        "App.ActiveDocument.%1.Label='%6'\n")
        .arg(objectName,
             ui->circleRadius->value().getSafeUserString(),
             ui->circleAngle1->value().getSafeUserString(),
             ui->circleAngle2->value().getSafeUserString(),
             placement,
             DlgPrimitives::tr("Circle"));
}

QString CirclePrimitive::change(const QString& objectName, const QString& placement) const
{
    return QString::fromLatin1(
        "%1.Radius='%2'\n"
        "%1.Angle1='%3'\n"
        "%1.Angle2='%4'\n"
        "%1.Placement=%5\n")
        .arg(objectName,
             ui->circleRadius->value().getSafeUserString(),
             ui->circleAngle1->value().getSafeUserString(),
             ui->circleAngle2->value().getSafeUserString(),
             placement);
}

void CirclePrimitive::changeValue(QObject* widget)
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

// ----------------------------------------------------------------------------

EllipsePrimitive::EllipsePrimitive(std::shared_ptr<Ui_DlgPrimitives> ui, Part::Ellipse* feature)
    : AbstractPrimitive(feature)
    , ui(ui)
{
    ui->ellipseMajorRadius->setRange(0, INT_MAX);
    ui->ellipseMinorRadius->setRange(0, INT_MAX);
    ui->ellipseAngle1->setRange(0, 360);
    ui->ellipseAngle2->setRange(0, 360);

    if (feature) {
        ui->ellipseMajorRadius->setValue(feature->MajorRadius.getQuantityValue());
        ui->ellipseMajorRadius->bind(feature->MajorRadius);
        ui->ellipseMinorRadius->setValue(feature->MinorRadius.getQuantityValue());
        ui->ellipseMinorRadius->bind(feature->MinorRadius);
        ui->ellipseAngle1->setValue(feature->Angle1.getQuantityValue());
        ui->ellipseAngle1->bind(feature->Angle1);
        ui->ellipseAngle2->setValue(feature->Angle2.getQuantityValue());
        ui->ellipseAngle2->bind(feature->Angle2);

        QSignalMapper* mapper = new QSignalMapper(this);
        connectSignalMapper(mapper);
        connectMapSignalMapper(ui->ellipseMajorRadius, qOverload<double>(&Gui::QuantitySpinBox::valueChanged), mapper);
        connectMapSignalMapper(ui->ellipseMinorRadius, qOverload<double>(&Gui::QuantitySpinBox::valueChanged), mapper);
        connectMapSignalMapper(ui->ellipseAngle1, qOverload<double>(&Gui::QuantitySpinBox::valueChanged), mapper);
        connectMapSignalMapper(ui->ellipseAngle2, qOverload<double>(&Gui::QuantitySpinBox::valueChanged), mapper);
    }
}

const char* EllipsePrimitive::getDefaultName() const
{
    return "Ellipse";
}

QString EllipsePrimitive::create(const QString& objectName, const QString& placement) const
{
    return QString::fromLatin1(
        "App.ActiveDocument.addObject(\"Part::Ellipse\",\"%1\")\n"
        "App.ActiveDocument.%1.MajorRadius='%2'\n"
        "App.ActiveDocument.%1.MinorRadius='%3'\n"
        "App.ActiveDocument.%1.Angle1='%4'\n"
        "App.ActiveDocument.%1.Angle2='%5'\n"
        "App.ActiveDocument.%1.Placement=%6\n"
        "App.ActiveDocument.%1.Label='%7'\n")
        .arg(objectName,
             ui->ellipseMajorRadius->value().getSafeUserString(),
             ui->ellipseMinorRadius->value().getSafeUserString(),
             ui->ellipseAngle1->value().getSafeUserString(),
             ui->ellipseAngle2->value().getSafeUserString(),
             placement,
             DlgPrimitives::tr("Ellipse"));
}

QString EllipsePrimitive::change(const QString& objectName, const QString& placement) const
{
    return QString::fromLatin1(
        "%1.MajorRadius='%2'\n"
        "%1.MinorRadius='%3'\n"
        "%1.Angle1='%4'\n"
        "%1.Angle2='%5'\n"
        "%1.Placement=%6\n")
        .arg(objectName,
             ui->ellipseMajorRadius->value().getSafeUserString(),
             ui->ellipseMinorRadius->value().getSafeUserString(),
             ui->ellipseAngle1->value().getSafeUserString(),
             ui->ellipseAngle2->value().getSafeUserString(),
             placement);
}

void EllipsePrimitive::changeValue(QObject* widget)
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

// ----------------------------------------------------------------------------

PolygonPrimitive::PolygonPrimitive(std::shared_ptr<Ui_DlgPrimitives> ui, Part::RegularPolygon* feature)
    : AbstractPrimitive(feature)
    , ui(ui)
{
    ui->regularPolygonCircumradius->setRange(0, INT_MAX);

    if (feature) {
        ui->regularPolygonPolygon->setValue(feature->Polygon.getValue());
        ui->regularPolygonCircumradius->setValue(feature->Circumradius.getQuantityValue());
        ui->regularPolygonCircumradius->bind(feature->Circumradius);

        QSignalMapper* mapper = new QSignalMapper(this);
        connectSignalMapper(mapper);
        connectMapSignalMapper(ui->regularPolygonPolygon, qOverload<int>(&QSpinBox::valueChanged), mapper);
        connectMapSignalMapper(ui->regularPolygonCircumradius, qOverload<double>(&Gui::QuantitySpinBox::valueChanged), mapper);
    }
}

const char* PolygonPrimitive::getDefaultName() const
{
    return "RegularPolygon";
}

QString PolygonPrimitive::create(const QString& objectName, const QString& placement) const
{
    return QString::fromLatin1(
        "App.ActiveDocument.addObject(\"Part::RegularPolygon\",\"%1\")\n"
        "App.ActiveDocument.%1.Polygon=%2\n"
        "App.ActiveDocument.%1.Circumradius='%3'\n"
        "App.ActiveDocument.%1.Placement=%4\n"
        "App.ActiveDocument.%1.Label='%5'\n")
        .arg(objectName,
             QString::number(ui->regularPolygonPolygon->value()),
             ui->regularPolygonCircumradius->value().getSafeUserString(),
             placement,
             DlgPrimitives::tr("Regular polygon"));
}

QString PolygonPrimitive::change(const QString& objectName, const QString& placement) const
{
    return QString::fromLatin1(
        "%1.Polygon=%2\n"
        "%1.Circumradius='%3'\n"
        "%1.Placement=%4\n")
        .arg(objectName,
             QString::number(ui->regularPolygonPolygon->value()),
             ui->regularPolygonCircumradius->value().getSafeUserString(),
             placement);
}

void PolygonPrimitive::changeValue(QObject* widget)
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

// ----------------------------------------------------------------------------

LinePrimitive::LinePrimitive(std::shared_ptr<Ui_DlgPrimitives> ui, Part::Line* feature)
    : AbstractPrimitive(feature)
    , ui(ui)
{
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

    if (feature) {
        ui->edgeX1->setValue(feature->X1.getQuantityValue());
        ui->edgeX1->bind(feature->X1);
        ui->edgeY1->setValue(feature->Y1.getQuantityValue());
        ui->edgeY1->bind(feature->Y1);
        ui->edgeZ1->setValue(feature->Z1.getQuantityValue());
        ui->edgeZ1->bind(feature->Z1);
        ui->edgeX2->setValue(feature->X2.getQuantityValue());
        ui->edgeX2->bind(feature->X2);
        ui->edgeY2->setValue(feature->Y2.getQuantityValue());
        ui->edgeY2->bind(feature->Y2);
        ui->edgeZ2->setValue(feature->Z2.getQuantityValue());
        ui->edgeZ2->bind(feature->Z2);

        QSignalMapper* mapper = new QSignalMapper(this);
        connectSignalMapper(mapper);
        connectMapSignalMapper(ui->edgeX1, qOverload<double>(&Gui::QuantitySpinBox::valueChanged), mapper);
        connectMapSignalMapper(ui->edgeY1, qOverload<double>(&Gui::QuantitySpinBox::valueChanged), mapper);
        connectMapSignalMapper(ui->edgeZ1, qOverload<double>(&Gui::QuantitySpinBox::valueChanged), mapper);
        connectMapSignalMapper(ui->edgeX2, qOverload<double>(&Gui::QuantitySpinBox::valueChanged), mapper);
        connectMapSignalMapper(ui->edgeY2, qOverload<double>(&Gui::QuantitySpinBox::valueChanged), mapper);
        connectMapSignalMapper(ui->edgeZ2, qOverload<double>(&Gui::QuantitySpinBox::valueChanged), mapper);
    }
}

const char* LinePrimitive::getDefaultName() const
{
    return "Line";
}

QString LinePrimitive::create(const QString& objectName, const QString& placement) const
{
    return QString::fromLatin1(
        "App.ActiveDocument.addObject(\"Part::Line\",\"%1\")\n"
        "App.ActiveDocument.%1.X1='%2'\n"
        "App.ActiveDocument.%1.Y1='%3'\n"
        "App.ActiveDocument.%1.Z1='%4'\n"
        "App.ActiveDocument.%1.X2='%5'\n"
        "App.ActiveDocument.%1.Y2='%6'\n"
        "App.ActiveDocument.%1.Z2='%7'\n"
        "App.ActiveDocument.%1.Placement=%8\n"
        "App.ActiveDocument.%1.Label='%9'\n")
        .arg(objectName,
             ui->edgeX1->value().getSafeUserString(),
             ui->edgeY1->value().getSafeUserString(),
             ui->edgeZ1->value().getSafeUserString(),
             ui->edgeX2->value().getSafeUserString(),
             ui->edgeY2->value().getSafeUserString(),
             ui->edgeZ2->value().getSafeUserString(),
             placement,
             DlgPrimitives::tr("Line"));
}

QString LinePrimitive::change(const QString& objectName, const QString& placement) const
{
    return QString::fromLatin1(
        "%1.X1='%2'\n"
        "%1.Y1='%3'\n"
        "%1.Z1='%4'\n"
        "%1.X2='%5'\n"
        "%1.Y2='%6'\n"
        "%1.Z2='%7'\n"
        "%1.Placement=%8\n")
        .arg(objectName,
             ui->edgeX1->value().getSafeUserString(),
             ui->edgeY1->value().getSafeUserString(),
             ui->edgeZ1->value().getSafeUserString(),
             ui->edgeX2->value().getSafeUserString(),
             ui->edgeY2->value().getSafeUserString(),
             ui->edgeZ2->value().getSafeUserString(),
             placement);
}

void LinePrimitive::changeValue(QObject* widget)
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

// ----------------------------------------------------------------------------

VertexPrimitive::VertexPrimitive(std::shared_ptr<Ui_DlgPrimitives> ui, Part::Vertex* feature)
    : AbstractPrimitive(feature)
    , ui(ui)
{
    ui->vertexX->setMaximum(INT_MAX);
    ui->vertexY->setMaximum(INT_MAX);
    ui->vertexZ->setMaximum(INT_MAX);
    ui->vertexX->setMinimum(INT_MIN);
    ui->vertexY->setMinimum(INT_MIN);
    ui->vertexZ->setMinimum(INT_MIN);

    if (feature) {
        ui->vertexX->setValue(feature->X.getQuantityValue());
        ui->vertexX->bind(feature->X);
        ui->vertexY->setValue(feature->Y.getQuantityValue());
        ui->vertexY->bind(feature->Y);
        ui->vertexZ->setValue(feature->Z.getQuantityValue());
        ui->vertexZ->bind(feature->Z);

        QSignalMapper* mapper = new QSignalMapper(this);
        connectSignalMapper(mapper);
        connectMapSignalMapper(ui->vertexX, qOverload<double>(&Gui::QuantitySpinBox::valueChanged), mapper);
        connectMapSignalMapper(ui->vertexY, qOverload<double>(&Gui::QuantitySpinBox::valueChanged), mapper);
        connectMapSignalMapper(ui->vertexZ, qOverload<double>(&Gui::QuantitySpinBox::valueChanged), mapper);
    }
}

const char* VertexPrimitive::getDefaultName() const
{
    return "Vertex";
}

QString VertexPrimitive::create(const QString& objectName, const QString& placement) const
{
    return QString::fromLatin1(
        "App.ActiveDocument.addObject(\"Part::Vertex\",\"%1\")\n"
        "App.ActiveDocument.%1.X='%2'\n"
        "App.ActiveDocument.%1.Y='%3'\n"
        "App.ActiveDocument.%1.Z='%4'\n"
        "App.ActiveDocument.%1.Placement=%5\n"
        "App.ActiveDocument.%1.Label='%6'\n")
        .arg(objectName,
             ui->vertexX->value().getSafeUserString(),
             ui->vertexY->value().getSafeUserString(),
             ui->vertexZ->value().getSafeUserString(),
             placement,
             DlgPrimitives::tr("Vertex"));
}

QString VertexPrimitive::change(const QString& objectName, const QString& placement) const
{
    return QString::fromLatin1(
        "%1.X='%2'\n"
        "%1.Y='%3'\n"
        "%1.Z='%4'\n"
        "%1.Placement=%5\n")
        .arg(objectName,
             ui->vertexX->value().getSafeUserString(),
             ui->vertexY->value().getSafeUserString(),
             ui->vertexZ->value().getSafeUserString(),
             placement);
}

void VertexPrimitive::changeValue(QObject* widget)
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

// ----------------------------------------------------------------------------

/* TRANSLATOR PartGui::DlgPrimitives */

DlgPrimitives::DlgPrimitives(QWidget* parent, Part::Primitive* feature)
  : QWidget(parent)
  , ui(new Ui_DlgPrimitives)
  , featurePtr(feature)
{
    ui->setupUi(this);
    connect(ui->buttonCircleFromThreePoints, &QPushButton::clicked,
            this, &DlgPrimitives::buttonCircleFromThreePoints);
    Gui::Command::doCommand(Gui::Command::Doc, "from FreeCAD import Base");
    Gui::Command::doCommand(Gui::Command::Doc, "import Part,PartGui");

    // must be in the same order as of the stacked widget
    addPrimitive(std::make_shared<PlanePrimitive>(ui, dynamic_cast<Part::Plane*>(feature)));
    addPrimitive(std::make_shared<BoxPrimitive>(ui, dynamic_cast<Part::Box*>(feature)));
    addPrimitive(std::make_shared<CylinderPrimitive>(ui, dynamic_cast<Part::Cylinder*>(feature)));
    addPrimitive(std::make_shared<ConePrimitive>(ui, dynamic_cast<Part::Cone*>(feature)));
    addPrimitive(std::make_shared<SpherePrimitive>(ui, dynamic_cast<Part::Sphere*>(feature)));
    addPrimitive(std::make_shared<EllipsoidPrimitive>(ui, dynamic_cast<Part::Ellipsoid*>(feature)));
    addPrimitive(std::make_shared<TorusPrimitive>(ui, dynamic_cast<Part::Torus*>(feature)));
    addPrimitive(std::make_shared<PrismPrimitive>(ui, dynamic_cast<Part::Prism*>(feature)));
    addPrimitive(std::make_shared<WedgePrimitive>(ui, dynamic_cast<Part::Wedge*>(feature)));
    addPrimitive(std::make_shared<HelixPrimitive>(ui, dynamic_cast<Part::Helix*>(feature)));
    addPrimitive(std::make_shared<SpiralPrimitive>(ui, dynamic_cast<Part::Spiral*>(feature)));
    addPrimitive(std::make_shared<CirclePrimitive>(ui, dynamic_cast<Part::Circle*>(feature)));
    addPrimitive(std::make_shared<EllipsePrimitive>(ui, dynamic_cast<Part::Ellipse*>(feature)));
    addPrimitive(std::make_shared<VertexPrimitive>(ui, dynamic_cast<Part::Vertex*>(feature)));
    addPrimitive(std::make_shared<LinePrimitive>(ui, dynamic_cast<Part::Line*>(feature)));
    addPrimitive(std::make_shared<PolygonPrimitive>(ui, dynamic_cast<Part::RegularPolygon*>(feature)));

    if (feature) {
        activatePage();
    }
}

/*
 *  Destroys the object and frees any allocated resources
 */
DlgPrimitives::~DlgPrimitives() = default;

void DlgPrimitives::activatePage()
{
    int index = findIndexOfValidPrimitive();
    ui->PrimitiveTypeCB->setCurrentIndex(index);
    ui->widgetStack2->setCurrentIndex(index);
    ui->PrimitiveTypeCB->setDisabled(true);
}

void DlgPrimitives::addPrimitive(std::shared_ptr<AbstractPrimitive> prim)
{
    primitive.push_back(prim);
}

std::shared_ptr<AbstractPrimitive> DlgPrimitives::getPrimitive(int index) const
{
    return primitive.at(index);
}

int DlgPrimitives::findIndexOfValidPrimitive() const
{
    return std::distance(primitive.begin(), std::find_if(primitive.begin(), primitive.end(),
                         [](std::shared_ptr<AbstractPrimitive> prim) {
        return prim->hasValidPrimitive();
    }));
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

void DlgPrimitives::buttonCircleFromThreePoints()
{
    CircleFromThreePoints pp;
    executeCallback(&pp);
}

void DlgPrimitives::tryCreatePrimitive(const QString& placement)
{
    QString cmd;
    QString name;
    App::Document* doc = App::GetApplication().getActiveDocument();
    if (!doc) {
        QMessageBox::warning(this, tr("Create %1")
            .arg(ui->PrimitiveTypeCB->currentText()), tr("No active document"));
        return;
    }

    std::shared_ptr<AbstractPrimitive> primitive = getPrimitive(ui->PrimitiveTypeCB->currentIndex());
    name = QString::fromLatin1(doc->getUniqueObjectName(primitive->getDefaultName()).c_str());
    cmd = primitive->create(name, placement);

    // Execute the Python block
    QString prim = tr("Create %1").arg(ui->PrimitiveTypeCB->currentText());
    Gui::Application::Instance->activeDocument()->openCommand(prim.toUtf8());
    Gui::Command::runCommand(Gui::Command::Doc, cmd.toUtf8());
    Gui::Command::runCommand(Gui::Command::Doc, getAutoGroupCommandStr(name).toUtf8());
    Gui::Application::Instance->activeDocument()->commitCommand();
    Gui::Command::runCommand(Gui::Command::Doc, "App.ActiveDocument.recompute()");
    Gui::Command::runCommand(Gui::Command::Gui, "Gui.SendMsgToActiveView(\"ViewFit\")");
}

void DlgPrimitives::createPrimitive(const QString& placement)
{
    try {
        tryCreatePrimitive(placement);
    }
    catch (const std::exception& e) {
        QMessageBox::warning(this, tr("Create %1")
            .arg(ui->PrimitiveTypeCB->currentText()), QCoreApplication::translate("Exception", e.what()));
    }
    catch (const Base::PyException& e) {
        QMessageBox::warning(this, tr("Create %1")
            .arg(ui->PrimitiveTypeCB->currentText()), QCoreApplication::translate("Exception", e.what()));
    }
}

void DlgPrimitives::acceptChanges(const QString& placement)
{
    App::Document* doc = featurePtr->getDocument();
    QString objectName = QString::fromLatin1("App.getDocument(\"%1\").%2")
                         .arg(QString::fromLatin1(doc->getName()),
                              QString::fromLatin1(featurePtr->getNameInDocument()));

    // read values from the properties
    std::shared_ptr<AbstractPrimitive> primitive = getPrimitive(ui->PrimitiveTypeCB->currentIndex());
    QString command = primitive->change(objectName, placement);

    // execute command, a transaction is already opened
    Gui::Command::runCommand(Gui::Command::App, command.toUtf8());
}

void DlgPrimitives::accept(const QString& placement)
{
    if (featurePtr.expired())
        return;
    App::Document* doc = featurePtr->getDocument();
    acceptChanges(placement);
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

// ----------------------------------------------

/* TRANSLATOR PartGui::Location */

Location::Location(QWidget* parent, Part::Feature* feature)
    : QWidget(parent)
    , ui(new Ui_Location)
    , featurePtr(feature)
{
    mode = 0;
    ui->setupUi(this);
    connect(ui->viewPositionButton, &QPushButton::clicked,
            this, &Location::onViewPositionButton);

    ui->XPositionQSB->setUnit(Base::Unit::Length);
    ui->YPositionQSB->setUnit(Base::Unit::Length);
    ui->ZPositionQSB->setUnit(Base::Unit::Length);
    ui->AngleQSB->setUnit(Base::Unit::Angle);

    // fill location widget if object already exists
    if (feature) {
        setPlacement(feature);
        bindExpressions(feature);
        connectSignals();
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

void Location::setPlacement(Part::Feature* feature)
{
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
}

void Location::bindExpressions(Part::Feature* feature)
{
    ui->XPositionQSB->bind(App::ObjectIdentifier::parse(feature, std::string("Placement.Base.x")));
    ui->YPositionQSB->bind(App::ObjectIdentifier::parse(feature, std::string("Placement.Base.y")));
    ui->ZPositionQSB->bind(App::ObjectIdentifier::parse(feature, std::string("Placement.Base.z")));
    ui->XDirectionEdit->bind(App::ObjectIdentifier::parse(feature, std::string("Placement.Rotation.Axis.x")));
    ui->YDirectionEdit->bind(App::ObjectIdentifier::parse(feature, std::string("Placement.Rotation.Axis.y")));
    ui->ZDirectionEdit->bind(App::ObjectIdentifier::parse(feature, std::string("Placement.Rotation.Axis.z")));
    ui->AngleQSB->bind(App::ObjectIdentifier::parse(feature, std::string("Placement.Rotation.Angle")));
}

void Location::connectSignals()
{
    connect(ui->XPositionQSB, qOverload<double>(&Gui::QuantitySpinBox::valueChanged), this, &Location::onPlacementChanged);
    connect(ui->YPositionQSB, qOverload<double>(&Gui::QuantitySpinBox::valueChanged), this, &Location::onPlacementChanged);
    connect(ui->ZPositionQSB, qOverload<double>(&Gui::QuantitySpinBox::valueChanged), this, &Location::onPlacementChanged);
    connect(ui->AngleQSB,     qOverload<double>(&Gui::QuantitySpinBox::valueChanged), this, &Location::onPlacementChanged);
    connect(ui->XDirectionEdit, qOverload<double>(&Gui::DoubleSpinBox::valueChanged), this, &Location::onPlacementChanged);
    connect(ui->YDirectionEdit, qOverload<double>(&Gui::DoubleSpinBox::valueChanged), this, &Location::onPlacementChanged);
    connect(ui->ZDirectionEdit, qOverload<double>(&Gui::DoubleSpinBox::valueChanged), this, &Location::onPlacementChanged);
}

void Location::onPlacementChanged()
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

void Location::onViewPositionButton()
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
