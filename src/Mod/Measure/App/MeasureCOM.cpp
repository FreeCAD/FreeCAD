// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2023 David Friedli <david[at]friedli-be.ch>             *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/

#include <QTextStream>

#include <App/Application.h>
#include <App/Document.h>

#include <Mod/Part/App/PartFeature.h>
#include <Mod/Part/App/TopoShape.h>

#include "MeasureCOM.h"


using namespace Measure;

PROPERTY_SOURCE(Measure::MeasureCOM, Measure::MeasureBase)


MeasureCOM::MeasureCOM()
{
    ADD_PROPERTY_TYPE(Element, (nullptr), "Measurement", App::Prop_None, "Element to measure COM");
    Element.setScope(App::LinkScope::Global);
    Element.setAllowExternal(true);

    ADD_PROPERTY_TYPE(
        CenterOfMass,
        (0.0, 0.0, 0.0),
        "Measurement",
        App::PropertyType(App::Prop_ReadOnly | App::Prop_Output),
        "Center of mass of element"
    );
}

MeasureCOM::~MeasureCOM() = default;


bool MeasureCOM::isValidSelection(const App::MeasureSelection& selection)
{
    if (selection.size() != 1) {
        return false;
    }

    const auto& item = selection.front();
    App::DocumentObject* obj = item.object.getObject();
    if (!obj) {
        return false;
    }

    const std::string& subName = item.object.getSubName();
    Part::TopoShape topoShape = Part::Feature::getTopoShape(
        obj,
        Part::ShapeOption::NeedSubElement | Part::ShapeOption::ResolveLink
            | Part::ShapeOption::Transform,
        subName.c_str()
    );

    // In toposhape centerOfGravity = centerOfMass
    return topoShape.centerOfGravity().has_value();
}


void MeasureCOM::parseSelection(const App::MeasureSelection& selection)
{
    const auto& item = selection.front();
    App::DocumentObject* obj = item.object.getObject();
    std::vector<std::string> subElements {item.object.getSubName()};
    Element.setValue(obj, subElements);
}


App::DocumentObjectExecReturn* MeasureCOM::execute()
{
    App::DocumentObject* obj = Element.getValue();
    if (!obj) {
        return DocumentObject::StdReturn;
    }

    const std::vector<std::string>& subElements = Element.getSubValues();
    const char* subName = subElements.empty() ? nullptr : subElements.front().c_str();

    Part::TopoShape topoShape = Part::Feature::getTopoShape(
        obj,
        Part::ShapeOption::NeedSubElement | Part::ShapeOption::ResolveLink
            | Part::ShapeOption::Transform,
        subName
    );

    // In toposhape centerOfGravity = centerOfMass
    auto com = topoShape.centerOfGravity();
    if (!com) {
        return new App::DocumentObjectExecReturn("Cannot calculate center of mass");
    }

    CenterOfMass.setValue(*com);

    return DocumentObject::StdReturn;
}


void MeasureCOM::onChanged(const App::Property* prop)
{
    if (isRestoring() || isRemoving()) {
        DocumentObject::onChanged(prop);
        return;
    }

    if (prop == &Element) {
        auto ret = recompute();
        delete ret;
    }
    DocumentObject::onChanged(prop);
}


QString MeasureCOM::getResultString()
{
    Base::Vector3d value = CenterOfMass.getValue();
    QString unit = QString::fromStdString(CenterOfMass.getUnit().getString());
    const int precision = 2;
    QString text;
    QTextStream(&text) << "COM" << Qt::endl
                       << "X: " << QString::number(value.x, 'f', precision) << " " << unit << Qt::endl
                       << "Y: " << QString::number(value.y, 'f', precision) << " " << unit << Qt::endl
                       << "Z: " << QString::number(value.z, 'f', precision) << " " << unit;
    return text;
}


Base::Placement MeasureCOM::getPlacement() const
{
    Base::Placement placement;
    placement.setPosition(CenterOfMass.getValue());
    return placement;
}


std::vector<App::DocumentObject*> MeasureCOM::getSubject() const
{
    return {Element.getValue()};
}
