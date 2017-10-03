/***************************************************************************
 *   Copyright (c) 2015 Stefan Tröger <stefantroeger@gmx.net>              *
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
#ifndef _PreComp_
# include <cfloat>
#include <BRepBuilderAPI_MakeFace.hxx>
#endif

#include "ShapeBinder.h"
#include <Mod/Part/App/TopoShape.h>

#ifndef M_PI
#define M_PI       3.14159265358979323846
#endif

using namespace PartDesign;

// ============================================================================

PROPERTY_SOURCE(PartDesign::ShapeBinder, Part::Feature)

ShapeBinder::ShapeBinder()
{
    ADD_PROPERTY_TYPE(Support, (0,0), "",(App::PropertyType)(App::Prop_None),"Support of the geometry");
    Placement.setStatus(App::Property::Hidden, true);
}

ShapeBinder::~ShapeBinder()
{
}

short int ShapeBinder::mustExecute(void) const {

    if(Support.isTouched())
        return 1;

    return Part::Feature::mustExecute();
}

App::DocumentObjectExecReturn* ShapeBinder::execute(void) {

    if(! this->isRestoring()){
        Part::Feature* obj = nullptr;
        std::vector<std::string> subs;

        ShapeBinder::getFilteredReferences(&Support, obj, subs);
        //if we have a link we rebuild the shape, but we change nothing if we are a simple copy
        if(obj) {
            Part::TopoShape shape = ShapeBinder::buildShapeFromReferences(obj, subs);
            Base::Placement placement(shape.getTransform());
            Shape.setValue(shape);
            Placement.setValue(placement);
        }
    }

    return Part::Feature::execute();
}

void ShapeBinder::getFilteredReferences(App::PropertyLinkSubList* prop, Part::Feature*& obj, std::vector< std::string >& subobjects) {

    obj = nullptr;
    subobjects.clear();

    auto objs = prop->getValues();
    auto subs = prop->getSubValues();

    if(objs.empty()) {
        return;
    }

    //we only allow one part feature, so get the first one we find
    size_t index = 0;
    while(index < objs.size() && !objs[index]->isDerivedFrom(Part::Feature::getClassTypeId()))
        index++;

    //do we have any part feature?
    if(index >= objs.size())
        return;

    obj = static_cast<Part::Feature*>(objs[index]);

    //if we have no subshpape we use the whole shape
    if(subs[index].empty()) {
            return;
    }

    //collect all subshapes for the object
    index = 0;
    for(std::string sub : subs) {

        //we only allow subshapes from a single Part::Feature
        if(objs[index] != obj)
            continue;

        //in this mode the full shape is not allowed, as we already started the subshape
        //processing
        if(sub.empty())
            continue;

        subobjects.push_back(sub);
    }
}


Part::TopoShape ShapeBinder::buildShapeFromReferences( Part::Feature* obj, std::vector< std::string > subs) {

    if(!obj)
        return TopoDS_Shape();

    if(subs.empty())
        return obj->Shape.getShape();

    //if we use multiple subshapes we build a shape from them by fusing them together
    Part::TopoShape base;
    std::vector<TopoDS_Shape> operators;
    for(std::string sub : subs) {

        if(base.isNull())
            base = obj->Shape.getShape().getSubShape(sub.c_str());
        else
            operators.push_back(obj->Shape.getShape().getSubShape(sub.c_str()));
    }

    try {
        if(!operators.empty() && !base.isNull())
            return base.fuse(operators);
    }
    catch(...) {
        return base;
    }
    return base;
}

void ShapeBinder::handleChangedPropertyType(Base::XMLReader &reader, const char *TypeName, App::Property *prop)
{
    // The type of Support was App::PropertyLinkSubList in the past
    if (prop == &Support && strcmp(TypeName, "App::PropertyLinkSubList") == 0) {
        Support.Restore(reader);
    }
}
