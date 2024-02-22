/***************************************************************************
 *   Copyright (c) 2023 David Friedli <david[at]friedli-be.ch>             *
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

#include <App/PropertyContainer.h>
#include <App/Application.h>
#include <App/Document.h>
#include <tuple>

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

    App::Document* doc = App::GetApplication().getActiveDocument();

    for (const std::tuple<std::string, std::string>& element : selection) {
        const std::string& obName = get<0>(element);
        App::DocumentObject* ob = doc->getObject(obName.c_str());
        
        const std::string& subName = get<1>(element);
        const char* className = ob->getSubObject(subName.c_str())->getTypeId().getName();
        std::string mod = ob->getClassTypeId().getModuleName(className);

        if (!hasGeometryHandler(mod)) {
            return false;
        }

        App::MeasureHandler handler = App::GetApplication().getMeasureHandler(mod.c_str());
        App::MeasureElementType type = handler.typeCb(obName.c_str(), subName.c_str());

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

    App::Document* doc = App::GetApplication().getActiveDocument();

    std::vector<App::DocumentObject*> objects;
    std::vector<const char*> subElements;

    for (const std::tuple<std::string, std::string>& element : selection) {
       App::DocumentObject* ob = doc->getObject(get<0>(element).c_str());

        auto sub = std::vector<std::string>();
        sub.push_back(get<1>(element));

        Element.setValue(ob, sub);
        break;
    }
    initialize();
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
    const std::string& mod = object->getClassTypeId().getModuleName(className);
    auto handler = getGeometryHandlerCB(mod);
    if (!handler) {
        throw Base::RuntimeError("No geometry handler available for submitted element type");
    }

    std::string obName = object->getNameInDocument();
    auto info = handler(&obName, &subElement);
    auto positionInfo = static_cast<MeasurePositionInfo*>(info);
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

namespace Measure {
// explicit template instantiation
template class MeasureExport MeasureBaseExtendable<Base::Vector3d>;
}
