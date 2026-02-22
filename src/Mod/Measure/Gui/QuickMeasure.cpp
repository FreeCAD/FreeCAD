// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2023 Pierre-Louis Boyer <development@Ondsel.com>        *
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


#include <cmath>
#include <vector>
#include <QTimer>


#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/Link.h>
#include <App/Part.h>
#include <Base/UnitsApi.h>
#include <Gui/Application.h>
#include <Gui/MainWindow.h>
#include <Gui/Selection/Selection.h>
#include <Gui/Control.h>

#include <Mod/Part/App/PartFeature.h>
#include <Mod/Part/App/TopoShape.h>
#include <Mod/Part/App/DatumFeature.h>

#include <Mod/Measure/App/Measurement.h>

#include "QuickMeasure.h"

using namespace Measure;
using namespace MeasureGui;

FC_LOG_LEVEL_INIT("QuickMeasure", true, true)

QuickMeasure::QuickMeasure(QObject* parent)
    : QObject(parent)
    , measurement {new Measure::Measurement()}
{
    selectionTimer = new QTimer(this);
    pendingProcessing = false;
    connect(selectionTimer, &QTimer::timeout, this, &QuickMeasure::processSelection);
}

QuickMeasure::~QuickMeasure()
{
    delete selectionTimer;
    delete measurement;
}

void QuickMeasure::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (shouldMeasure(msg)) {
        if (!pendingProcessing) {
            selectionTimer->start(100);
        }
        pendingProcessing = true;
    }
}

void QuickMeasure::processSelection()
{
    if (pendingProcessing) {
        pendingProcessing = false;
        try {
            tryMeasureSelection();
        }
        catch (const Base::IndexError&) {
            // ignore this exception because it can be caused by trying to access a non-existing
            // sub-element e.g. when selecting a construction geometry in sketcher
        }
        catch (const Base::ValueError&) {
            // ignore this exception because it can be caused by trying to access a non-existing
            // sub-element e.g. when selecting a constraint in sketcher
        }
        catch (const Base::Exception& e) {
            e.reportException();
        }
        catch (const Standard_Failure& e) {
            FC_ERR(e);
        }
        catch (...) {
            FC_ERR("Unhandled unknown exception");
        }
    }
}

void QuickMeasure::tryMeasureSelection()
{
    Gui::Document* doc = Gui::Application::Instance->activeDocument();
    measurement->clear();
    if (doc && Gui::Control().activeDialog(nullptr) == nullptr) {
        // we (still) have a doc and are not in a tool dialog where the user needs to click on stuff
        addSelectionToMeasurement();
    }
    printResult();
}

bool QuickMeasure::shouldMeasure(const Gui::SelectionChanges& msg) const
{
    if (!Gui::getMainWindow()->isRightSideMessageVisible()) {
        // don't measure if there's no where to show the result
        return false;
    }

    Gui::Document* doc = Gui::Application::Instance->activeDocument();
    if (!doc) {
        // no active document
        return false;
    }

    if (msg.Type == Gui::SelectionChanges::AddSelection
        || msg.Type == Gui::SelectionChanges::RmvSelection
        || msg.Type == Gui::SelectionChanges::SetSelection
        || msg.Type == Gui::SelectionChanges::ClrSelection) {
        // the event is about a change in selected objects
        return true;
    }
    return false;
}

bool QuickMeasure::isObjAcceptable(App::DocumentObject* obj)
{
    // only measure shapes. Exclude datums that derive from Part::Feature
    if (obj && obj->isDerivedFrom<Part::Feature>() && !obj->isDerivedFrom<Part::Datum>()) {
        return true;
    }

    return false;
}

void QuickMeasure::addSelectionToMeasurement()
{
    int count = 0;
    int limit = 100;

    auto selObjs = Gui::Selection().getSelectionEx(
        nullptr,
        App::DocumentObject::getClassTypeId(),
        Gui::ResolveMode::NoResolve
    );

    for (auto& selObj : selObjs) {
        App::DocumentObject* rootObj = selObj.getObject();
        const std::vector<std::string> subNames = selObj.getSubNames();

        // Check that there's not too many selection
        count += subNames.empty() ? 1 : subNames.size();
        if (count > limit) {
            measurement->clear();
            return;
        }

        if (subNames.empty()) {
            if (isObjAcceptable(rootObj)) {
                measurement->addReference3D(rootObj, "");
            }
            continue;
        }

        for (auto& subName : subNames) {
            App::DocumentObject* obj = rootObj->getSubObject(subName.c_str());

            if (!isObjAcceptable(obj)) {
                continue;
            }
            measurement->addReference3D(rootObj, subName);
        }
    }
}

static QString areaStr(double value)
{
    Base::Quantity area(value, Base::Unit::Area);
    return QString::fromStdString(area.getUserString());
}

static QString lengthStr(double value)
{
    Base::Quantity dist(value, Base::Unit::Length);
    return QString::fromStdString(dist.getUserString());
}

static QString angleStr(double value)
{
    Base::Quantity dist(value, Base::Unit::Angle);
    return QString::fromStdString(dist.getUserString());
}

void QuickMeasure::printResult()
{
    MeasureType mtype = measurement->getType();
    if (mtype == MeasureType::Surfaces) {
        print(tr("Total area: %1").arg(areaStr(measurement->area())));
    }
    /* deactivated because computing the volumes/area of solids makes a significant
    slow down in selection of complex solids.
    else if (mtype == MeasureType::Volumes) {
        Base::Quantity area(measurement->area(), Base::Unit::Area);
        Base::Quantity vol(measurement->volume(), Base::Unit::Volume);
        print(tr("Volume: %1, Area:
    %2").arg(vol.getSafeUserString()).arg(area.getSafeUserString()));
    }*/
    else if (mtype == MeasureType::TwoPlanes) {
        print(tr("Nominal distance: %1").arg(lengthStr(measurement->planePlaneDistance())));
    }
    else if (mtype == MeasureType::Cone || mtype == MeasureType::Plane) {
        print(tr("Area: %1").arg(areaStr(measurement->area())));
    }
    else if (mtype == MeasureType::CylinderSection || mtype == MeasureType::Sphere
             || mtype == MeasureType::Torus) {
        print(tr("Area: %1, Radius: %2")
                  .arg(areaStr(measurement->area()), lengthStr(measurement->radius())));
    }
    else if (mtype == MeasureType::Cylinder) {
        print(tr("Area: %1, Diameter: %2")
                  .arg(areaStr(measurement->area()), lengthStr(measurement->diameter())));
    }
    else if (mtype == MeasureType::TwoCylinders) {

        double angle = measurement->angle();

        if (angle <= Precision::Confusion()) {
            print(
                tr("Total area: %1, Axis distance: %2")
                    .arg(areaStr(measurement->area()), lengthStr(measurement->cylinderAxisDistance()))
            );
        }
        else {
            print(tr("Total area: %1, Axis distance: %2, Axis angle: %3")
                      .arg(
                          areaStr(measurement->area()),
                          lengthStr(measurement->cylinderAxisDistance()),
                          angleStr(angle)
                      ));
        }
    }
    else if (mtype == MeasureType::Edges) {
        print(tr("Total length: %1").arg(lengthStr(measurement->length())));
    }
    else if (mtype == MeasureType::TwoParallelLines) {
        print(tr("Nominal distance: %1").arg(lengthStr(measurement->lineLineDistance())));
    }
    else if (mtype == MeasureType::TwoLines) {
        print(tr("Angle: %1, Total length: %2")
                  .arg(angleStr(measurement->angle()), lengthStr(measurement->length())));
    }
    else if (mtype == MeasureType::Line) {
        print(tr("Length: %1").arg(lengthStr(measurement->length())));
    }
    else if (mtype == MeasureType::CircleArc) {
        print(tr("Radius: %1").arg(lengthStr(measurement->radius())));
    }
    else if (mtype == MeasureType::Circle) {
        print(tr("Diameter: %1").arg(lengthStr(measurement->diameter())));
    }
    else if (mtype == MeasureType::PointToPoint) {
        print(tr("Distance: %1").arg(lengthStr(measurement->length())));
    }
    else if (mtype == MeasureType::PointToEdge || mtype == MeasureType::PointToSurface) {
        print(tr("Minimum distance: %1").arg(lengthStr(measurement->length())));
    }
    else if (mtype == MeasureType::PointToCylinder) {
        print(
            tr("Minimum distance: %1, Axis distance: %2")
                .arg(lengthStr(measurement->length()), lengthStr(measurement->cylinderAxisDistance()))
        );
    }
    else if (mtype == MeasureType::PointToCircle) {
        print(
            tr("Minimum distance: %1, Center distance: %2")
                .arg(lengthStr(measurement->length()), lengthStr(measurement->circleCenterDistance()))
        );
    }
    else if (mtype == MeasureType::TwoCircles) {
        double angle = measurement->angle();
        if (angle <= Precision::Confusion()) {
            print(tr("Total length: %1, Center distance: %2")
                      .arg(
                          lengthStr(measurement->length()),
                          lengthStr(measurement->circleCenterDistance())
                      ));
        }
        else {
            print(tr("Total length: %1, Center distance: %2, Axis angle: %3")
                      .arg(
                          lengthStr(measurement->length()),
                          lengthStr(measurement->circleCenterDistance()),
                          angleStr(angle)
                      ));
        }
    }
    else if (mtype == MeasureType::CircleToEdge) {
        print(
            tr("Total length: %1, Center distance: %2")
                .arg(lengthStr(measurement->length()), lengthStr(measurement->circleCenterDistance()))
        );
    }
    else if (mtype == MeasureType::CircleToSurface) {
        print(tr("Center surface distance: %1").arg(lengthStr(measurement->circleCenterDistance())));
    }
    else if (mtype == MeasureType::CircleToCylinder) {
        double angle = measurement->angle();
        if (angle <= Precision::Confusion()) {
            print(tr("Center axis distance: %1").arg(lengthStr(measurement->cylinderAxisDistance())));
        }
        else {
            print(tr("Center axis distance: %1, Axis angle: %2")
                      .arg(lengthStr(measurement->cylinderAxisDistance()), angleStr(angle)));
        }
    }
    else {
        print(QStringLiteral(""));
    }
}

void QuickMeasure::print(const QString& message)
{
    Gui::getMainWindow()->setRightSideMessage(message);
}


#include "moc_QuickMeasure.cpp"
