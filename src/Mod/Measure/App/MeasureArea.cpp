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


#include <App/Application.h>
#include <App/MeasureManager.h>
#include <App/Document.h>

#include "MeasureArea.h"


using namespace Measure;

PROPERTY_SOURCE(Measure::MeasureArea, Measure::MeasureBase)


MeasureArea::MeasureArea()
{
    ADD_PROPERTY_TYPE(Elements, (nullptr), "Measurement", App::Prop_None, "Element to get the area from");
    Elements.setScope(App::LinkScope::Global);
    Elements.setAllowExternal(true);

    ADD_PROPERTY_TYPE(
        Area,
        (0.0),
        "Measurement",
        App::PropertyType(App::Prop_ReadOnly | App::Prop_Output),
        "Area of element"
    );
}

MeasureArea::~MeasureArea() = default;

bool MeasureArea::isSupported(App::MeasureElementType type)
{
    // clang-format off
    return (type == App::MeasureElementType::PLANE) ||
           (type == App::MeasureElementType::CYLINDER) ||
           (type == App::MeasureElementType::SURFACE) ||
           (type == App::MeasureElementType::VOLUME) ||
           (type == App::MeasureElementType::DISC);
    // clang-format on
}

bool MeasureArea::isValidSelection(const App::MeasureSelection& selection)
{

    if (selection.empty()) {
        return false;
    }

    for (auto element : selection) {
        auto type = App::MeasureManager::getMeasureElementType(element);

        if (type == App::MeasureElementType::INVALID) {
            return false;
        }

        if (!isSupported(type)) {
            return false;
        }
    }
    return true;
}

void MeasureArea::parseSelection(const App::MeasureSelection& selection)
{
    // Set properties from selection, method is only invoked when isValid Selection returns true

    std::vector<App::DocumentObject*> objects;
    std::vector<std::string> subElements;

    for (auto element : selection) {
        auto objT = element.object;

        objects.push_back(objT.getObject());
        subElements.push_back(objT.getSubName());
    }

    Elements.setValues(objects, subElements);
}


App::DocumentObjectExecReturn* MeasureArea::execute()
{
    const std::vector<App::DocumentObject*>& objects = Elements.getValues();
    const std::vector<std::string>& subElements = Elements.getSubValues();

    double result(0.0);

    // Loop through Elements and call the valid geometry handler
    for (std::vector<App::DocumentObject*>::size_type i = 0; i < objects.size(); i++) {
        App::SubObjectT subject {objects.at(i), subElements.at(i).c_str()};

        auto info = getMeasureInfo(subject);
        if (!info || !info->valid) {
            return new App::DocumentObjectExecReturn("Cannot calculate area");
        }
        auto areaInfo = std::dynamic_pointer_cast<Part::MeasureAreaInfo>(info);
        result += areaInfo->area;
    }

    Area.setValue(result);
    return DocumentObject::StdReturn;
}


void MeasureArea::onChanged(const App::Property* prop)
{
    if (isRestoring() || isRemoving()) {
        return;
    }

    if (prop == &Elements) {
        auto ret = recompute();
        delete ret;
    }

    MeasureBase::onChanged(prop);
}


Base::Placement MeasureArea::getPlacement() const
{
    const std::vector<App::DocumentObject*>& objects = Elements.getValues();
    const std::vector<std::string>& subElements = Elements.getSubValues();

    if (objects.empty() || subElements.empty()) {
        return Base::Placement();
    }

    App::SubObjectT subject {objects.front(), subElements.front().c_str()};

    auto info = getMeasureInfo(subject);
    if (!info) {
        return {};
    }
    auto areaInfo = std::dynamic_pointer_cast<Part::MeasureAreaInfo>(info);
    return areaInfo->placement;
}


//! Return the object we are measuring
//! used by the viewprovider in determining visibility
std::vector<App::DocumentObject*> MeasureArea::getSubject() const
{
    return Elements.getValues();
}
