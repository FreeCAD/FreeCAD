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
#include <Base/Tools.h>
# include <BRepExtrema_DistShapeShape.hxx>

#include "MeasureDistance.h"


using namespace Measure;

PROPERTY_SOURCE(Measure::MeasureDistance, Measure::MeasureBase)


MeasureDistance::MeasureDistance()
{
    ADD_PROPERTY_TYPE(Element1,(nullptr), "Measurement", App::Prop_None, "First element of the measurement");
    Element1.setScope(App::LinkScope::Global);
    Element1.setAllowExternal(true);

    ADD_PROPERTY_TYPE(Element2,(nullptr), "Measurement", App::Prop_None, "Second element of the measurement");
    Element2.setScope(App::LinkScope::Global);
    Element2.setAllowExternal(true);

    ADD_PROPERTY_TYPE(Distance,(0.0)       ,"Measurement",App::PropertyType(App::Prop_ReadOnly|App::Prop_Output),
                                            "Distance between the two elements");
    Distance.setUnit(Base::Unit::Length);

    ADD_PROPERTY_TYPE(Position1,(Base::Vector3d(0.0,0.0,0.0)),"Measurement", App::Prop_Hidden, "Position1");
    ADD_PROPERTY_TYPE(Position2,(Base::Vector3d(0.0,1.0,0.0)),"Measurement", App::Prop_Hidden, "Position2");

}

MeasureDistance::~MeasureDistance() = default;


bool MeasureDistance::isValidSelection(const App::MeasureSelection& selection){

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

        if (
            type != App::MeasureElementType::POINT &&
            type != App::MeasureElementType::LINE &&
            type != App::MeasureElementType::LINESEGMENT &&
            type != App::MeasureElementType::CIRCLE &&
            type != App::MeasureElementType::ARC &&
            type != App::MeasureElementType::CURVE &&
            type != App::MeasureElementType::PLANE &&
            type != App::MeasureElementType::CYLINDER
            ) {
            return false;
        }
    }
    return true;
}

bool MeasureDistance::isPrioritizedSelection(const App::MeasureSelection& selection) {
    // TODO: Check if Elements ar parallel
    // if (selection.size() == 2) {
    //     return true;
    // }
    
    return false;
}


void MeasureDistance::parseSelection(const App::MeasureSelection& selection) {

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


bool MeasureDistance::getShape(App::PropertyLinkSub* prop, TopoDS_Shape& rShape) {

    App::DocumentObject* ob = prop->getValue();
    std::vector<std::string> subs = prop->getSubValues();
    
    if (!ob || !ob->isValid() || subs.size() < 1 ) {
        return false;
    }

    std::string subName = subs.at(0);
    const char* className = ob->getSubObject(subName.c_str())->getTypeId().getName();
    std::string mod = ob->getClassTypeId().getModuleName(className);
    
    if (!hasGeometryHandler(mod)) {
        return false;
    }

    auto handler = getGeometryHandler(mod);
    std::string obName = static_cast<std::string>(ob->getNameInDocument());

    auto info = handler(&obName, &subName);
    if (!info.valid) {
        return false;
    }

    rShape = info.shape;
    return true;
}


App::DocumentObjectExecReturn *MeasureDistance::execute()
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

    // Get both shapes from geometry handler
    TopoDS_Shape shape1;
    if (!this->getShape(&Element1, shape1)) {
        return new App::DocumentObjectExecReturn("Could not get shape");
    }

    TopoDS_Shape shape2;
    if (!this->getShape(&Element2, shape2)) {
        return new App::DocumentObjectExecReturn("Could not get shape");
    }

    // Calculate the extrema
    BRepExtrema_DistShapeShape measure(shape1, shape2);
    if (!measure.IsDone() || measure.NbSolution() < 1) {
        return new App::DocumentObjectExecReturn("Could not get extrema");
    }

    Distance.setValue(measure.Value());

    gp_Pnt p1 = measure.PointOnShape1(1);
    Position1.setValue(p1.X(), p1.Y(), p1.Z());

    gp_Pnt p2 = measure.PointOnShape2(1);
    Position2.setValue(p2.X(), p2.Y(), p2.Z());


    return DocumentObject::StdReturn;
}

void MeasureDistance::onChanged(const App::Property* prop)
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
std::vector<App::DocumentObject*> MeasureDistance::getSubject() const
{
    return {Element1.getValue()};
}

