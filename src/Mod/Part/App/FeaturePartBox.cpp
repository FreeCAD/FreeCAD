/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2002     *
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
# include <BRepPrimAPI_MakeBox.hxx>
# include <Precision.hxx>
#endif


#include <Base/Console.h>
#include <Base/Reader.h>
#include "FeaturePartBox.h"


using namespace Part;


PROPERTY_SOURCE(Part::Box, Part::Primitive)


Box::Box()
{
    ADD_PROPERTY_TYPE(Length,(10.0f),"Box",App::Prop_None,"The length of the box");
    ADD_PROPERTY_TYPE(Width ,(10.0f),"Box",App::Prop_None,"The width of the box");
    ADD_PROPERTY_TYPE(Height,(10.0f),"Box",App::Prop_None,"The height of the box");
}

short Box::mustExecute() const
{
    if (Length.isTouched() ||
        Height.isTouched() ||
        Width.isTouched() )
        return 1;
    return Primitive::mustExecute();
}

App::DocumentObjectExecReturn *Box::execute(void)
{
    double L = Length.getValue();
    double W = Width.getValue();
    double H = Height.getValue();

    if (L < Precision::Confusion())
        return new App::DocumentObjectExecReturn("Length of box too small");

    if (W < Precision::Confusion())
        return new App::DocumentObjectExecReturn("Width of box too small");

    if (H < Precision::Confusion())
        return new App::DocumentObjectExecReturn("Height of box too small");

    try {
        // Build a box using the dimension attributes
        BRepPrimAPI_MakeBox mkBox(L, W, H);
        TopoDS_Shape ResultShape = mkBox.Shape();
        this->Shape.setValue(ResultShape,false);
        return Primitive::execute();
    }
    catch (Standard_Failure& e) {
        return new App::DocumentObjectExecReturn(e.GetMessageString());
    }
}

/**
 * This method was added for backward-compatibility. In former versions
 * of Box we had the properties x,y,z and l,h,w which have changed to
 * Location -- as replacement for x,y and z and Length, Height and Width.
 */
void Box::Restore(Base::XMLReader &reader)
{
    reader.readElement("Properties");
    int Cnt = reader.getAttributeAsInteger("Count");
    int transientCount = 0;
    if(reader.hasAttribute("TransientCount"))
        transientCount = reader.getAttributeAsUnsigned("TransientCount");
    Cnt += transientCount;

    bool location_xyz = false;
    bool location_axis = false;
    bool distance_lhw = false;
    Base::Placement plm;
    App::PropertyDistance x,y,z;
    App::PropertyDistance l,w,h;
    App::PropertyVector Axis, Location;
    Axis.setValue(0.0f,0.0f,1.0f);
    for (int i=0 ;i<Cnt;i++) {
        reader.readElement(i<transientCount?"_Property":"Property");
        const char* PropName = reader.getAttribute("name");
        const char* TypeName = reader.getAttribute("type");
        App::Property* prop = getPropertyByName(PropName);
        if(prop && reader.hasAttribute("status"))
            prop->setStatusValue(reader.getAttributeAsUnsigned("status"));
        if (prop && strcmp(prop->getTypeId().getName(), TypeName) == 0) {
            if (!prop->testStatus(App::Property::Transient) &&
                !(getPropertyType(prop) & App::Prop_Transient))
                prop->Restore(reader);
            if(i>=transientCount)
                reader.readEndElement("Property");
            continue;
        }
        if (!prop) {
            // in case this comes from an old document we must use the new properties
            if (strcmp(PropName, "l") == 0) {
                distance_lhw = true;
                prop = &l;
            }
            else if (strcmp(PropName, "w") == 0) {
                distance_lhw = true;
                prop = &h; // by mistake w was considered as height
            }
            else if (strcmp(PropName, "h") == 0) {
                distance_lhw = true;
                prop = &w; // by mistake h was considered as width
            }
            else if (strcmp(PropName, "x") == 0) {
                location_xyz = true;
                prop = &x;
            }
            else if (strcmp(PropName, "y") == 0) {
                location_xyz = true;
                prop = &y;
            }
            else if (strcmp(PropName, "z") == 0) {
                location_xyz = true;
                prop = &z;
            }
            else if (strcmp(PropName, "Axis") == 0) {
                location_axis = true;
                prop = &Axis;
            }
            else if (strcmp(PropName, "Location") == 0) {
                location_axis = true;
                prop = &Location;
            }
        }
        else if (strcmp(PropName, "Length") == 0 && strcmp(TypeName,"PropertyDistance") == 0) {
            distance_lhw = true;
            prop = &l;
        }
        else if (strcmp(PropName, "Height") == 0 && strcmp(TypeName,"PropertyDistance") == 0) {
            distance_lhw = true;
            prop = &h;
        }
        else if (strcmp(PropName, "Width") == 0 && strcmp(TypeName,"PropertyDistance") == 0) {
            distance_lhw = true;
            prop = &w;
        }

        // NOTE: We must also check the type of the current property because a subclass
        // of PropertyContainer might change the type of a property but not its name.
        // In this case we would force to read-in a wrong property type and the behaviour
        // would be undefined.
        std::string tn = TypeName;
        if (strcmp(TypeName,"PropertyDistance") == 0) // missing prefix App::
            tn = std::string("App::") + tn;
        if (prop && strcmp(prop->getTypeId().getName(), tn.c_str()) == 0)
            prop->Restore(reader);

        if(i>=transientCount)
            reader.readEndElement("Property");
    }

    if (distance_lhw) {
        this->Length.setValue(l.getValue());
        this->Height.setValue(h.getValue());
        this->Width.setValue(w.getValue());
    }

    // for 0.7 releases or earlier
    if (location_xyz) {
        plm.setPosition(Base::Vector3d(x.getValue(),y.getValue(),z.getValue()));
        this->Placement.setValue(this->Placement.getValue() * plm);
        this->Shape.setStatus(App::Property::User1, true); // override the shape's location later on
    }
    // for 0.8 releases
    else if (location_axis) {
        Base::Vector3d d = Axis.getValue();
        Base::Vector3d p = Location.getValue();
        Base::Rotation rot(Base::Vector3d(0.0,0.0,1.0),
                           Base::Vector3d(d.x,d.y,d.z));
        plm.setRotation(rot);
        plm.setPosition(Base::Vector3d(p.x,p.y,p.z));
        this->Placement.setValue(this->Placement.getValue() * plm);
        this->Shape.setStatus(App::Property::User1, true); // override the shape's location later on
    }

    reader.readEndElement("Properties");
}

void Box::onChanged(const App::Property* prop)
{
    if (prop == &Length || prop == &Width || prop == &Height) {
        if (!isRestoring()) {
            App::DocumentObjectExecReturn *ret = recompute();
            delete ret;
        }
    }
    else if (prop == &this->Shape) {
        // see Box::Restore
        if (this->Shape.testStatus(App::Property::User1)) {
            this->Shape.setStatus(App::Property::User1, false);
            App::DocumentObjectExecReturn *ret = recompute();
            delete ret;
            return;
        }
    }
    Part::Primitive::onChanged(prop);
}
