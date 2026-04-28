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
#include <App/Document.h>
#include <App/MeasureManager.h>

#include <Mod/Part/App/PartFeature.h>

#include "MeasureLength.h"


using namespace Measure;

PROPERTY_SOURCE(Measure::MeasureLength, Measure::MeasureBase)


MeasureLength::MeasureLength()
{
    ADD_PROPERTY_TYPE(Elements, (nullptr), "Measurement", App::Prop_None, "Elements to get the length from");
    Elements.setScope(App::LinkScope::Global);
    Elements.setAllowExternal(true);

    ADD_PROPERTY_TYPE(
        Length,
        (0.0),
        "Measurement",
        App::PropertyType(App::Prop_ReadOnly | App::Prop_Output),
        "Length of selection"
    );
}

MeasureLength::~MeasureLength() = default;


bool MeasureLength::isValidSelection(const App::MeasureSelection& selection)
{

    if (selection.empty()) {
        return false;
    }

    for (auto element : selection) {
        auto type = App::MeasureManager::getMeasureElementType(element);

        if (type == App::MeasureElementType::INVALID) {
            return false;
        }

        if ((type != App::MeasureElementType::LINESEGMENT && type != App::MeasureElementType::CIRCLE
             && type != App::MeasureElementType::ARC && type != App::MeasureElementType::CURVE)) {
            return false;
        }
    }
    return true;
}

void MeasureLength::parseSelection(const App::MeasureSelection& selection)
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


App::DocumentObjectExecReturn* MeasureLength::execute()
{
    const std::vector<App::DocumentObject*>& objects = Elements.getValues();
    const std::vector<std::string>& subElements = Elements.getSubValues();

    double result(0.0);

    // Loop through Elements and call the valid geometry handler
    for (std::vector<App::DocumentObject*>::size_type i = 0; i < objects.size(); i++) {

        App::SubObjectT subject {objects.at(i), subElements.at(i).c_str()};
        auto info = getMeasureInfo(subject);
        if (!info || !info->valid) {
            return new App::DocumentObjectExecReturn("Cannot calculate length");
        }

        auto lengthInfo = std::dynamic_pointer_cast<Part::MeasureLengthInfo>(info);
        result += lengthInfo->length;
    }

    Length.setValue(result);
    return DocumentObject::StdReturn;
}


void MeasureLength::onChanged(const App::Property* prop)
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


Base::Placement MeasureLength::getPlacement() const
{
    const std::vector<App::DocumentObject*>& objects = Elements.getValues();
    const std::vector<std::string>& subElements = Elements.getSubValues();

    if (!objects.size() || !subElements.size()) {
        return Base::Placement();
    }

    App::SubObjectT subject {objects.front(), subElements.front().c_str()};
    auto info = getMeasureInfo(subject);

    if (!info || !info->valid) {
        return {};
    }
    auto lengthInfo = std::dynamic_pointer_cast<Part::MeasureLengthInfo>(info);
    return lengthInfo->placement;
}


//! Return the object we are measuring
//! used by the viewprovider in determining visibility
std::vector<App::DocumentObject*> MeasureLength::getSubject() const
{
    return Elements.getValues();
}
