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
#include <Base/Tools.h>
#include <Base/Precision.h>

#include "MeasureAngle.h"

using namespace Measure;

PROPERTY_SOURCE(Measure::MeasureAngle, Measure::MeasureBase)


MeasureAngle::MeasureAngle()
{
    ADD_PROPERTY_TYPE(Element1,(nullptr), "Measurement", App::Prop_None, "First element of the measurement");
    Element1.setScope(App::LinkScope::Global);
    Element1.setAllowExternal(true);

    ADD_PROPERTY_TYPE(Element2,(nullptr), "Measurement", App::Prop_None, "Second element of the measurement");
    Element2.setScope(App::LinkScope::Global);
    Element2.setAllowExternal(true);

    ADD_PROPERTY_TYPE(Angle,(0.0)       ,"Measurement",App::PropertyType(App::Prop_ReadOnly|App::Prop_Output),
                                            "Angle between the two elements");
    Angle.setUnit(Base::Unit::Angle);
}

MeasureAngle::~MeasureAngle() = default;


bool MeasureAngle::isValidSelection(const App::MeasureSelection& selection){

    if (selection.size() != 2) {
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

        if (!(type == App::MeasureElementType::LINE || type == App::MeasureElementType::PLANE)) {
            return false;
        }
    }
    return true;
}

bool MeasureAngle::isPrioritizedSelection(const App::MeasureSelection& selection) {
    if (selection.size() != 2) {
        return false;
    }
    
    // Check if the two elements are parallel
    App::Document* doc = App::GetApplication().getActiveDocument();

    App::DocumentObject* ob1 = doc->getObject(get<0>(selection.at(0)).c_str());
    std::string sub1 = get<1>(selection.at(0));
    Base::Vector3d vec1;
    getVec(*ob1, sub1, vec1);


    App::DocumentObject* ob2 = doc->getObject(get<0>(selection.at(1)).c_str());
    std::string sub2 = get<1>(selection.at(1));
    Base::Vector3d vec2;
    getVec(*ob2, sub2, vec2);

    
    double angle = std::fmod(vec1.GetAngle(vec2), D_PI);
    return angle > Base::Precision::Angular();
}


void MeasureAngle::parseSelection(const App::MeasureSelection& selection) {

    assert(selection.size() >= 2);

    App::Document* doc = App::GetApplication().getActiveDocument();

    App::DocumentObject* ob1 = doc->getObject(get<0>(selection.at(0)).c_str());
    const std::vector<std::string> elems1 = {get<1>(selection.at(0))};
    Element1.setValue(ob1, elems1);

    App::DocumentObject* ob2 = doc->getObject(get<0>(selection.at(1)).c_str());
    const std::vector<std::string> elems2 = {get<1>(selection.at(1))};
    Element2.setValue(ob2, elems2);

    initialize();
}


bool MeasureAngle::getVec(App::DocumentObject& ob, std::string& subName, Base::Vector3d& vecOut) {
    const char* className = ob.getSubObject(subName.c_str())->getTypeId().getName();
    std::string mod = ob.getClassTypeId().getModuleName(className);

    if (!hasGeometryHandler(mod)) {
        return false;
    }

    auto handler = getGeometryHandler(mod);

    std::string obName = static_cast<std::string>(ob.getNameInDocument());
    Measure::MeasureAngleInfo info = handler(&obName, &subName);

    vecOut = info.orientation;
    return true;
}

Base::Vector3d MeasureAngle::getLoc(App::DocumentObject& ob, std::string& subName) {
    const char* className = ob.getSubObject(subName.c_str())->getTypeId().getName();
    std::string mod = ob.getClassTypeId().getModuleName(className);

    if (!hasGeometryHandler(mod)) {
        return Base::Vector3d();
    }

    auto handler = getGeometryHandler(mod);
    std::string obName = static_cast<std::string>(ob.getNameInDocument());
    MeasureAngleInfo info = handler(&obName, &subName);

    return info.position;
}

gp_Vec MeasureAngle::vector1() {

    App::DocumentObject* ob = Element1.getValue();
    std::vector<std::string> subs = Element1.getSubValues();

    if (!ob || !ob->isValid() || subs.size() < 1 ) {
        return gp_Vec();
    }

    Base::Vector3d vec;
    getVec(*ob, subs.at(0), vec);
    return gp_Vec(vec.x, vec.y, vec.z);
}

gp_Vec MeasureAngle::vector2() {
    App::DocumentObject* ob = Element2.getValue();
    std::vector<std::string> subs = Element2.getSubValues();

    if (!ob || !ob->isValid() || subs.size() < 1 ) {
        return gp_Vec();
    }

    Base::Vector3d vec;
    getVec(*ob, subs.at(0), vec);
    return gp_Vec(vec.x, vec.y, vec.z);
}

gp_Vec MeasureAngle::location1() {

    App::DocumentObject* ob = Element1.getValue();
    std::vector<std::string> subs = Element1.getSubValues();

    if (!ob || !ob->isValid() || subs.size() < 1 ) {
        return {};
    }
    auto temp = getLoc(*ob, subs.at(0));
    return {temp.x, temp.y, temp.z};
}
gp_Vec MeasureAngle::location2() {
    App::DocumentObject* ob = Element2.getValue();
    std::vector<std::string> subs = Element2.getSubValues();

    if (!ob || !ob->isValid() || subs.size() < 1 ) {
        return {};
    }

    auto temp = getLoc(*ob, subs.at(0));
    return {temp.x, temp.y, temp.z};
}

App::DocumentObjectExecReturn *MeasureAngle::execute()
{

    App::DocumentObject* ob1 = Element1.getValue();
    std::vector<std::string> subs1 = Element1.getSubValues();

    App::DocumentObject* ob2 = Element2.getValue();
    std::vector<std::string> subs2 = Element2.getSubValues();

    if (!ob1 || !ob1->isValid() || !ob2 || !ob2->isValid()) {
        return new App::DocumentObjectExecReturn("Submitted object(s) is not valid");
    }

    if (subs1.size() < 1 || subs2.size() < 1) {
        return new App::DocumentObjectExecReturn("No geometry element picked");
    }

    Base::Vector3d vec1;
    getVec(*ob1, subs1.at(0), vec1);

    Base::Vector3d vec2;
    getVec(*ob2, subs2.at(0), vec2);

    Angle.setValue(Base::toDegrees(vec1.GetAngle(vec2)));

    return DocumentObject::StdReturn;
}

void MeasureAngle::onChanged(const App::Property* prop)
{

    if (prop == &Element1 || prop == &Element2) {
        if (!isRestoring()) {
            App::DocumentObjectExecReturn *ret = recompute();
            delete ret;
        }
    }
    DocumentObject::onChanged(prop);
}

//! Return the object we are measuring
//! used by the viewprovider in determining visibility
std::vector<App::DocumentObject*> MeasureAngle::getSubject() const
{
    return {Element1.getValue()};
}

namespace Measure{
// explicit template instantiation
template class MeasureExport MeasureBaseExtendable<Measure::MeasureAngleInfo>;
}
