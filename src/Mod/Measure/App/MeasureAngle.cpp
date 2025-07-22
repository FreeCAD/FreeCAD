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
#include <App/Document.h>
#include <App/MeasureManager.h>
#include <Base/Tools.h>
#include <Base/Precision.h>

#include "MeasureAngle.h"

using namespace Measure;

PROPERTY_SOURCE(Measure::MeasureAngle, Measure::MeasureBase)


MeasureAngle::MeasureAngle()
{
    ADD_PROPERTY_TYPE(Element1,
                      (nullptr),
                      "Measurement",
                      App::Prop_None,
                      "First element of the measurement");
    Element1.setScope(App::LinkScope::Global);
    Element1.setAllowExternal(true);

    ADD_PROPERTY_TYPE(Element2,
                      (nullptr),
                      "Measurement",
                      App::Prop_None,
                      "Second element of the measurement");
    Element2.setScope(App::LinkScope::Global);
    Element2.setAllowExternal(true);

    ADD_PROPERTY_TYPE(Angle,
                      (0.0),
                      "Measurement",
                      App::PropertyType(App::Prop_ReadOnly | App::Prop_Output),
                      "Angle between the two elements");
    Angle.setUnit(Base::Unit::Angle);
}

MeasureAngle::~MeasureAngle() = default;


bool MeasureAngle::isValidSelection(const App::MeasureSelection& selection)
{
    if (selection.size() != 2) {
        return false;
    }

    for (auto element : selection) {
        auto type = App::MeasureManager::getMeasureElementType(element);

        if (type == App::MeasureElementType::INVALID) {
            return false;
        }

        if (!(type == App::MeasureElementType::LINE || type == App::MeasureElementType::PLANE
              || type == App::MeasureElementType::LINESEGMENT)) {
            return false;
        }
    }
    return true;
}

bool MeasureAngle::isPrioritizedSelection(const App::MeasureSelection& selection)
{
    if (selection.size() != 2) {
        return false;
    }

    // Check if the two elements are parallel
    auto element1 = selection.at(0);
    auto objT1 = element1.object;
    App::DocumentObject* ob1 = objT1.getObject();
    std::string sub1 = objT1.getSubName();
    Base::Vector3d vec1;
    getVec(*ob1, sub1, vec1);

    auto element2 = selection.at(1);
    auto objT2 = element2.object;
    App::DocumentObject* ob2 = objT2.getObject();
    std::string sub2 = objT2.getSubName();
    Base::Vector3d vec2;
    getVec(*ob2, sub2, vec2);


    double angle = std::fmod(vec1.GetAngle(vec2), std::numbers::pi);
    return angle > Base::Precision::Angular();
}


void MeasureAngle::parseSelection(const App::MeasureSelection& selection)
{

    assert(selection.size() >= 2);

    auto element1 = selection.at(0);
    auto objT1 = element1.object;
    App::DocumentObject* ob1 = objT1.getObject();
    const std::vector<std::string> elems1 = {objT1.getSubName()};
    Element1.setValue(ob1, elems1);

    auto element2 = selection.at(1);
    auto objT2 = element2.object;
    App::DocumentObject* ob2 = objT2.getObject();
    const std::vector<std::string> elems2 = {objT2.getSubName()};
    Element2.setValue(ob2, elems2);
}


bool MeasureAngle::getVec(App::DocumentObject& ob, std::string& subName, Base::Vector3d& vecOut)
{
    App::SubObjectT subject {&ob, subName.c_str()};
    auto info = getMeasureInfo(subject);
    if (!info || !info->valid) {
        return false;
    }

    auto angleInfo = std::dynamic_pointer_cast<Part::MeasureAngleInfo>(info);
    vecOut = angleInfo->orientation;
    return true;
}

Base::Vector3d MeasureAngle::getLoc(App::DocumentObject& ob, std::string& subName)
{
    App::SubObjectT subject {&ob, subName.c_str()};
    auto info = getMeasureInfo(subject);
    if (!info || !info->valid) {
        return Base::Vector3d();
    }

    auto angleInfo = std::dynamic_pointer_cast<Part::MeasureAngleInfo>(info);
    return angleInfo->position;
}

gp_Vec MeasureAngle::vector1()
{

    App::DocumentObject* ob = Element1.getValue();
    std::vector<std::string> subs = Element1.getSubValues();

    if (!ob || !ob->isValid() || subs.empty()) {
        return {};
    }

    Base::Vector3d vec;
    getVec(*ob, subs.at(0), vec);
    return gp_Vec(vec.x, vec.y, vec.z);
}

gp_Vec MeasureAngle::vector2()
{
    App::DocumentObject* ob = Element2.getValue();
    std::vector<std::string> subs = Element2.getSubValues();

    if (!ob || !ob->isValid() || subs.empty()) {
        return gp_Vec();
    }

    Base::Vector3d vec;
    getVec(*ob, subs.at(0), vec);
    return gp_Vec(vec.x, vec.y, vec.z);
}

gp_Vec MeasureAngle::location1()
{

    App::DocumentObject* ob = Element1.getValue();
    std::vector<std::string> subs = Element1.getSubValues();

    if (!ob || !ob->isValid() || subs.empty()) {
        return {};
    }
    auto temp = getLoc(*ob, subs.at(0));
    return {temp.x, temp.y, temp.z};
}
gp_Vec MeasureAngle::location2()
{
    App::DocumentObject* ob = Element2.getValue();
    std::vector<std::string> subs = Element2.getSubValues();

    if (!ob || !ob->isValid() || subs.empty()) {
        return {};
    }

    auto temp = getLoc(*ob, subs.at(0));
    return {temp.x, temp.y, temp.z};
}

App::DocumentObjectExecReturn* MeasureAngle::execute()
{
    App::DocumentObject* ob1 = Element1.getValue();
    std::vector<std::string> subs1 = Element1.getSubValues();

    App::DocumentObject* ob2 = Element2.getValue();
    std::vector<std::string> subs2 = Element2.getSubValues();

    if (!ob1 || !ob1->isValid() || !ob2 || !ob2->isValid()) {
        return new App::DocumentObjectExecReturn("Submitted object(s) is not valid");
    }

    if (subs1.empty() || subs2.empty()) {
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
            App::DocumentObjectExecReturn* ret = recompute();
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
