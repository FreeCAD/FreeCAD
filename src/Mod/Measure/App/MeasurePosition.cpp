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

#include <App/PropertyContainer.h>
#include <App/Application.h>
#include <App/MeasureManager.h>
#include <App/Document.h>

#include "MeasurePosition.h"


using namespace Measure;

PROPERTY_SOURCE(Measure::MeasurePosition, Measure::MeasureBase)



MeasurePosition::MeasurePosition()
{
    ADD_PROPERTY_TYPE(Element,(nullptr), "Measurement", App::Prop_None, "Element to get the position from");
    Element.setScope(App::LinkScope::Global);
    Element.setAllowExternal(true);


    ADD_PROPERTY_TYPE(Position,(0.0, 0.0, 0.0), "Measurement", App::PropertyType(App::Prop_ReadOnly|App::Prop_Output),
                                            "The absolute position");

}

MeasurePosition::~MeasurePosition() = default;


bool MeasurePosition::isValidSelection(const App::MeasureSelection& selection){

    if (selection.empty() || selection.size() > 1) {
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

        if (type != App::MeasureElementType::POINT) {
            return false;
        }
    }
    return true;
}

void MeasurePosition::parseSelection(const App::MeasureSelection& selection) {
    // Set properties from selection, method is only invoked when isValid Selection returns true

    for (auto element : selection) {
        auto objT = element.object;

        std::vector<std::string> subElements {objT.getSubName()};
        Element.setValue(objT.getObject(), subElements);
        break;
    }
}


App::DocumentObjectExecReturn *MeasurePosition::execute()
{
    recalculatePosition();
    return DocumentObject::StdReturn;
}

void MeasurePosition::recalculatePosition()
{
    const App::DocumentObject* object = Element.getValue();
    const std::vector<std::string>& subElements = Element.getSubValues();

    // Get the position of the first point
    std::string subElement = subElements.front();

    // Get the Geometry handler based on the module
    const char* className = object->getSubObject(subElement.c_str())->getTypeId().getName();
    const std::string& mod = Base::Type::getModuleName(className);
    auto handler = getGeometryHandler(mod);
    if (!handler) {
        throw Base::RuntimeError("No geometry handler available for submitted element type");
    }

    App::SubObjectT subject{object, subElement.c_str()};
    auto info = handler(subject);
    auto positionInfo = std::dynamic_pointer_cast<Part::MeasurePositionInfo>(info);
    Position.setValue(positionInfo->position);
}

void MeasurePosition::onChanged(const App::Property* prop)
{
    if (isRestoring() || isRemoving()) {
        return;
    }

    if (prop == &Element) {
        recalculatePosition();
    }
    DocumentObject::onChanged(prop);
}


QString MeasurePosition::getResultString() {
    App::Property* prop = this->getResultProp();
    if (prop == nullptr) {
        return {};
    }

    Base::Vector3d value = Position.getValue();
    QString unit = Position.getUnit().getString();
    int precision = 2;
    QString text;
    #if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
    QTextStream(&text)
        << "X: " << QString::number(value.x, 'f', precision) << " " << unit << endl
        << "Y: " << QString::number(value.y, 'f', precision) << " " << unit << endl
        << "Z: " << QString::number(value.z, 'f', precision) << " " << unit;
    #else
    QTextStream(&text)
        << "X: " << QString::number(value.x, 'f', precision) << " " << unit << Qt::endl
        << "Y: " << QString::number(value.y, 'f', precision) << " " << unit << Qt::endl
        << "Z: " << QString::number(value.z, 'f', precision) << " " << unit;
    #endif
    return text;
}


Base::Placement MeasurePosition::getPlacement() {
    Base::Placement placement;
    placement.setPosition(Position.getValue());
    return placement;
}


//! Return the object we are measuring
//! used by the viewprovider in determining visibility
std::vector<App::DocumentObject*> MeasurePosition::getSubject() const
{
    return {Element.getValue()};
}
