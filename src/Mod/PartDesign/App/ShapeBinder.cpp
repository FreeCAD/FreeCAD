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
# include <boost/bind.hpp>
# include <gp_Lin.hxx>
# include <gp_Pln.hxx>
# include <BRep_Builder.hxx>
# include <BRepBuilderAPI_MakeEdge.hxx>
# include <BRepBuilderAPI_MakeFace.hxx>
#endif

#include "ShapeBinder.h"
#include <App/Document.h>
#include <App/GroupExtension.h>
#include <App/OriginFeature.h>
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
    ADD_PROPERTY_TYPE(TraceSupport, (false), "", App::Prop_None, "Trace support shape");
}

ShapeBinder::~ShapeBinder()
{
    this->connectDocumentChangedObject.disconnect();
}

short int ShapeBinder::mustExecute(void) const {

    if (Support.isTouched())
        return 1;
    if (TraceSupport.isTouched())
        return 1;

    return Part::Feature::mustExecute();
}

App::DocumentObjectExecReturn* ShapeBinder::execute(void) {

    if (!this->isRestoring()) {
        App::GeoFeature* obj = nullptr;
        std::vector<std::string> subs;

        ShapeBinder::getFilteredReferences(&Support, obj, subs);

        //if we have a link we rebuild the shape, but we change nothing if we are a simple copy
        if (obj) {
            Part::TopoShape shape(ShapeBinder::buildShapeFromReferences(obj, subs));
            //now, shape is in object's CS, and includes local Placement of obj but nothing else.

            if (TraceSupport.getValue()) {
                //compute the transform, and apply it to the shape.
                Base::Placement sourceCS = //full placement of container of obj
                        obj->globalPlacement() * obj->Placement.getValue().inverse();
                Base::Placement targetCS = //full placement of container of this shapebinder
                        this->globalPlacement() * this->Placement.getValue().inverse();
                Base::Placement transform = targetCS.inverse() * sourceCS;
                shape.setPlacement(transform * shape.getPlacement());
            }

            this->Placement.setValue(shape.getTransform());
            this->Shape.setValue(shape);
        }
    }

    return Part::Feature::execute();
}

void ShapeBinder::getFilteredReferences(App::PropertyLinkSubList* prop, App::GeoFeature*& obj,
                                        std::vector< std::string >& subobjects)
{
    obj = nullptr;
    subobjects.clear();

    auto objs = prop->getValues();
    auto subs = prop->getSubValues();

    if (objs.empty()) {
        return;
    }

    //we only allow one part feature, so get the first one we find
    size_t index = 0;
    for (auto* it : objs) {
        if (it && it->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {
            obj = static_cast<Part::Feature*>(it);
            break;
        }
        index++;
    }

    //do we have any part feature?
    if (obj) {
        //if we have no subshpape we use the whole shape
        if (subs[index].empty()) {
            return;
        }

        //collect all subshapes for the object
        for (index = 0; index < objs.size(); index++) {
            //we only allow subshapes from a single Part::Feature
            if (objs[index] != obj)
                continue;

            //in this mode the full shape is not allowed, as we already started the subshape
            //processing
            if (subs[index].empty())
                continue;

            subobjects.push_back(subs[index]);
        }
    }
    else {
        // search for Origin features
        for (auto* it : objs) {
            if (it && it->getTypeId().isDerivedFrom(App::Line::getClassTypeId())) {
                obj = static_cast<App::GeoFeature*>(it);
                break;
            }
            else if (it && it->getTypeId().isDerivedFrom(App::Plane::getClassTypeId())) {
                obj = static_cast<App::GeoFeature*>(it);
                break;
            }
        }
    }
}

Part::TopoShape ShapeBinder::buildShapeFromReferences(App::GeoFeature* obj, std::vector< std::string > subs) {

    if (!obj)
        return TopoDS_Shape();

    if (obj->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {
        Part::Feature* part = static_cast<Part::Feature*>(obj);
        if (subs.empty())
            return part->Shape.getValue();

        std::vector<TopoDS_Shape> shapes;
        for (std::string sub : subs) {
            shapes.push_back(part->Shape.getShape().getSubShape(sub.c_str()));
        }

        if (shapes.size() == 1) {
            //single subshape. Return directly.
            return shapes[0];
        }
        else {
            //multiple subshapes. Make a compound.
            BRep_Builder builder;
            TopoDS_Compound cmp;
            builder.MakeCompound(cmp);
            for(const TopoDS_Shape& sh : shapes){
                builder.Add(cmp, sh);
            }
            return cmp;
        }
    }
    else if (obj->getTypeId().isDerivedFrom(App::Line::getClassTypeId())) {
        gp_Lin line;
        BRepBuilderAPI_MakeEdge mkEdge(line);
        Part::TopoShape shape(mkEdge.Shape());
        shape.setPlacement(obj->Placement.getValue());
        return shape;
    }
    else if (obj->getTypeId().isDerivedFrom(App::Plane::getClassTypeId())) {
        gp_Pln plane;
        BRepBuilderAPI_MakeFace mkFace(plane);
        Part::TopoShape shape(mkFace.Shape());
        shape.setPlacement(obj->Placement.getValue());
        return shape;
    }

    return TopoDS_Shape();
}

void ShapeBinder::handleChangedPropertyType(Base::XMLReader &reader, const char *TypeName, App::Property *prop)
{
    // The type of Support was App::PropertyLinkSubList in the past
    if (prop == &Support && strcmp(TypeName, "App::PropertyLinkSubList") == 0) {
        Support.Restore(reader);
    }
}

void ShapeBinder::onSettingDocument()
{
    App::Document* document = getDocument();
    if (document) {
        this->connectDocumentChangedObject = document->signalChangedObject.connect(boost::bind
            (&ShapeBinder::slotChangedObject, this, _1, _2));
    }
}

void ShapeBinder::slotChangedObject(const App::DocumentObject& Obj, const App::Property& Prop)
{
    App::Document* doc = getDocument();
    if (!doc || doc->testStatus(App::Document::Restoring))
        return;
    if (this == &Obj)
        return;
    if (!TraceSupport.getValue())
        return;
    if (!Prop.getTypeId().isDerivedFrom(App::PropertyPlacement::getClassTypeId()))
        return;

    App::GeoFeature* obj = nullptr;
    std::vector<std::string> subs;
    ShapeBinder::getFilteredReferences(&Support, obj, subs);
    if (obj) {
        if (obj == &Obj) {
            // the directly referenced object has changed
            enforceRecompute();
        }
        else if (Obj.hasExtension(App::GroupExtension::getExtensionClassTypeId())) {
            // check if the changed property belongs to a group-like object
            // like Body or Part
            std::vector<App::DocumentObject*> chain;
            std::vector<App::DocumentObject*> list = getInListRecursive();
            chain.insert(chain.end(), list.begin(), list.end());
            list = obj->getInListRecursive();
            chain.insert(chain.end(), list.begin(), list.end());

            auto it = std::find(chain.begin(), chain.end(), &Obj);
            if (it != chain.end()) {
                enforceRecompute();
            }
        }
    }
}
