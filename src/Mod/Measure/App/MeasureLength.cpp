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


#include "PreCompiled.h"

#include <App/Application.h>
#include <App/Document.h>
#include <App/MeasureManager.h>

#include <Mod/Part/App/PartFeature.h>

#include "MeasureLength.h"


using namespace Measure;

PROPERTY_SOURCE(Measure::MeasureLength, Measure::MeasureBase)



MeasureLength::MeasureLength()
{
    ADD_PROPERTY_TYPE(Elements,(nullptr), "Measurement", App::Prop_None, "Elements to get the length from");
    Elements.setScope(App::LinkScope::Global);
    Elements.setAllowExternal(true);

    ADD_PROPERTY_TYPE(Length,(0.0)       ,"Measurement",App::PropertyType(App::Prop_ReadOnly|App::Prop_Output),
                                            "Length of selection");

}

MeasureLength::~MeasureLength() = default;


bool MeasureLength::isValidSelection(const App::MeasureSelection& selection){

    if (selection.empty()) {
        return false;
    }

    for (auto element : selection) {
        auto objT = element.object;
        
        App::DocumentObject* ob = objT.getObject();
        const std::string& subName = objT.getSubName();
        const char* className = objT.getSubObject()->getTypeId().getName();
        std::string mod = Base::Type::getModuleName(className);

        if (!hasGeometryHandler(mod)) {
            return false;
        }

        App::MeasureHandler handler = App::MeasureManager::getMeasureHandler(mod.c_str());
        App::MeasureElementType type = handler.typeCb(ob, subName.c_str());

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

void MeasureLength::parseSelection(const App::MeasureSelection& selection) {
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


App::DocumentObjectExecReturn *MeasureLength::execute()
{
    recalculateLength();
    return DocumentObject::StdReturn;
}

void MeasureLength::recalculateLength()
{
    const std::vector<App::DocumentObject*>& objects = Elements.getValues();
    const std::vector<std::string>& subElements = Elements.getSubValues();

    double result(0.0);

    // Loop through Elements and call the valid geometry handler
    for (std::vector<App::DocumentObject*>::size_type i=0; i<objects.size(); i++) {
        App::DocumentObject *object = objects.at(i);
        std::string subElement = subElements.at(i);

        // Get the Geometry handler based on the module
        const char* className = object->getSubObject(subElement.c_str())->getTypeId().getName();
        const std::string& mod = Base::Type::getModuleName(className);
        auto handler = getGeometryHandler(mod);
        if (!handler) {
            throw Base::RuntimeError("No geometry handler available for submitted element type");
        }

        App::SubObjectT subject{object, subElement.c_str()};
        auto info = handler(subject);
        auto lengthInfo = std::dynamic_pointer_cast<Part::MeasureLengthInfo>(info);
        result += lengthInfo->length;
    }

    Length.setValue(result);
}

void MeasureLength::onChanged(const App::Property* prop)
{
    if (isRestoring() || isRemoving()) {
        return;
    }

    if (prop == &Elements) {
        recalculateLength();
    }
    
    MeasureBase::onChanged(prop);
}


Base::Placement MeasureLength::getPlacement() {
    const std::vector<App::DocumentObject*>& objects = Elements.getValues();
    const std::vector<std::string>& subElements = Elements.getSubValues();

    if (!objects.size() || !subElements.size()) {
        return Base::Placement();
    }

    App::DocumentObject* object = objects.front();
    std::string subElement = subElements.front();
    const char* className = object->getSubObject(subElement.c_str())->getTypeId().getName();
    const std::string& mod = Base::Type::getModuleName(className);

    auto handler = getGeometryHandler(mod);
    if (!handler) {
        throw Base::RuntimeError("No geometry handler available for submitted element type");
    }

    App::SubObjectT subject{object, subElement.c_str()};
    auto info = handler(subject);
    auto lengthInfo = std::dynamic_pointer_cast<Part::MeasureLengthInfo>(info);
    return lengthInfo->placement;
}


//! Return the object we are measuring
//! used by the viewprovider in determining visibility
std::vector<App::DocumentObject*> MeasureLength::getSubject() const
{
    return Elements.getValues();
}

