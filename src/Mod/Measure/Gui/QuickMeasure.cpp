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

#include "PreCompiled.h"

#ifndef _PreComp_
#include <cmath>
#include <vector>
#endif

#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/Link.h>
#include <App/Part.h>
#include <Base/UnitsApi.h>
#include <Gui/Application.h>
#include <Gui/MainWindow.h>
#include <Gui/Selection.h>

#include <Mod/Part/App/PartFeature.h>
#include <Mod/Part/App/TopoShape.h>

#include <Mod/Measure/App/Measurement.h>

#include "QuickMeasure.h"

using namespace Measure;
using namespace MeasureGui;

QuickMeasure::QuickMeasure(QObject* parent)
    : QObject(parent)
    , measurement{new Measure::Measurement()}
{
}

QuickMeasure::~QuickMeasure()
{
    delete measurement;
}

void QuickMeasure::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    try {
        tryMeasureSelection(msg);
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
        e.ReportException();
    }
}

void QuickMeasure::tryMeasureSelection(const Gui::SelectionChanges& msg)
{
    if (canMeasureSelection(msg)) {
        measurement->clear();
        addSelectionToMeasurement();
        printResult();
    }
}

bool QuickMeasure::canMeasureSelection(const Gui::SelectionChanges& msg) const
{
    if (msg.Type == Gui::SelectionChanges::SetPreselect ||
        msg.Type == Gui::SelectionChanges::RmvPreselect) {
        return false;
    }

    Gui::Document* doc = Gui::Application::Instance->activeDocument();
    return doc != nullptr;
}

void QuickMeasure::addSelectionToMeasurement()
{
    for (auto& selObj : Gui::Selection().getSelectionEx()) {
        App::DocumentObject* obj = selObj.getObject();
        const std::vector<std::string> subNames = selObj.getSubNames();
        if (subNames.empty()) {
            measurement->addReference3D(obj, "");
        }
        else {
            for (auto& subName : subNames) {
                measurement->addReference3D(obj, subName);
            }
        }
    }
}

void QuickMeasure::printResult()
{
    MeasureType mtype = measurement->getType();
    if (mtype == MeasureType::Surfaces) {
        Base::Quantity area(measurement->area(), Base::Unit::Area);
        print(tr("Total area: %1").arg(area.getUserString()));
    }
    /* deactivated because computing the volumes/area of solids makes a significant
    slow down in selection of complex solids.
    else if (mtype == MeasureType::Volumes) {
        Base::Quantity area(measurement->area(), Base::Unit::Area);
        Base::Quantity vol(measurement->volume(), Base::Unit::Volume);
        print(tr("Volume: %1, Area: %2").arg(vol.getSafeUserString()).arg(area.getSafeUserString()));
    }*/
    else if (mtype == MeasureType::TwoPlanes) {
        Base::Quantity dist(measurement->planePlaneDistance(), Base::Unit::Length);
        print(tr("Nominal distance: %1").arg(dist.getSafeUserString()));
    }
    else if (mtype == MeasureType::Cone || mtype == MeasureType::Plane) {
        Base::Quantity area(measurement->area(), Base::Unit::Area);
        print(tr("Area: %1").arg(area.getUserString()));
    }
    else if (mtype == MeasureType::Cylinder || mtype == MeasureType::Sphere || mtype == MeasureType::Torus) {
        Base::Quantity area(measurement->area(), Base::Unit::Area);
        Base::Quantity rad(measurement->radius(), Base::Unit::Length);
        print(tr("Area: %1, Radius: %2").arg(area.getSafeUserString(), rad.getSafeUserString()));
    }
    else if (mtype == MeasureType::Edges) {
        Base::Quantity dist(measurement->length(), Base::Unit::Length);
        print(tr("Total length: %1").arg(dist.getSafeUserString()));
    }
    else if (mtype == MeasureType::TwoParallelLines) {
        Base::Quantity dist(measurement->lineLineDistance(), Base::Unit::Length);
        print(tr("Nominal distance: %1").arg(dist.getSafeUserString()));
    }
    else if (mtype == MeasureType::TwoLines) {
        Base::Quantity angle(measurement->angle(), Base::Unit::Length);
        Base::Quantity dist(measurement->length(), Base::Unit::Length);
        print(tr("Angle: %1, Total length: %2").arg(angle.getSafeUserString(), dist.getSafeUserString()));
    }
    else if (mtype == MeasureType::Line) {
        Base::Quantity dist(measurement->length(), Base::Unit::Length);
        print(tr("Length: %1").arg(dist.getSafeUserString()));
    }
    else if (mtype == MeasureType::Circle) {
        Base::Quantity dist(measurement->radius(), Base::Unit::Length);
        print(tr("Radius: %1").arg(dist.getSafeUserString()));
    }
    else if (mtype == MeasureType::PointToPoint) {
        Base::Quantity dist(measurement->length(), Base::Unit::Length);
        print(tr("Distance: %1").arg(dist.getSafeUserString()));
    }
    else if (mtype == MeasureType::PointToEdge || mtype == MeasureType::PointToSurface) {
        Base::Quantity dist(measurement->length(), Base::Unit::Length);
        print(tr("Minimum distance: %1").arg(dist.getSafeUserString()));
    }
    else {
        print(QLatin1String(""));
    }
}

void QuickMeasure::print(const QString& message)
{
    Gui::getMainWindow()->setRightSideMessage(message);
}


#include "moc_QuickMeasure.cpp"
