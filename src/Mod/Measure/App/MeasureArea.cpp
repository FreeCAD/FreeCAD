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
#include <App/MeasureManager.h>
#include <App/Document.h>

#include "MeasureArea.h"


using namespace Measure;

PROPERTY_SOURCE(Measure::MeasureArea, Measure::MeasureBase)



MeasureArea::MeasureArea()
{
    ADD_PROPERTY_TYPE(Elements,(nullptr), "Measurement", App::Prop_None, "Element to get the area from");
    Elements.setScope(App::LinkScope::Global);
    Elements.setAllowExternal(true);

    ADD_PROPERTY_TYPE(Area,(0.0), "Measurement", App::PropertyType(App::Prop_ReadOnly|App::Prop_Output),
                                            "Area of element");

}

MeasureArea::~MeasureArea() = default;


bool MeasureArea::isValidSelection(const App::MeasureSelection& selection){

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

        // TODO: Also support Cylinder & Volume?
        if ((type != App::MeasureElementType::PLANE && type != App::MeasureElementType::CYLINDER)) {
            return false;
        }
    }
    return true;
}

void MeasureArea::parseSelection(const App::MeasureSelection& selection) {
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


App::DocumentObjectExecReturn *MeasureArea::execute()
{
    recalculateArea();
    return DocumentObject::StdReturn;
}

void MeasureArea::recalculateArea()
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
        auto areaInfo = std::dynamic_pointer_cast<Part::MeasureAreaInfo>(info);
        result += areaInfo->area;
    }

    Area.setValue(result);
}

void MeasureArea::onChanged(const App::Property* prop)
{
    if (isRestoring() || isRemoving()) {
        return;
    }

    if (prop == &Elements) {
        recalculateArea();
    }
    
    MeasureBase::onChanged(prop);
}


Base::Placement MeasureArea::getPlacement() {
    const std::vector<App::DocumentObject*>& objects = Elements.getValues();
    const std::vector<std::string>& subElements = Elements.getSubValues();

    if (objects.empty() || subElements.empty()) {
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
    auto areaInfo = std::dynamic_pointer_cast<Part::MeasureAreaInfo>(info);
    return areaInfo->placement;
}


//! Return the object we are measuring
//! used by the viewprovider in determining visibility
std::vector<App::DocumentObject*> MeasureArea::getSubject() const
{
    return Elements.getValues();
}
