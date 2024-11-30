/***************************************************************************
 *   Copyright (c) 2007 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <BRepBuilderAPI_MakeEdge.hxx>
# include <gp_Circ.hxx>
# include <TopoDS_Edge.hxx>
#endif

#include <Base/Tools.h>

#include "FeaturePartCircle.h"


using namespace Part;

App::PropertyQuantityConstraint::Constraints Circle::angleRange = {0.0,360.0,1.0};

PROPERTY_SOURCE(Part::Circle, Part::Primitive)


Circle::Circle()
{
    ADD_PROPERTY(Radius,(2.0f));
    ADD_PROPERTY(Angle1,(0.0f));
    Angle1.setConstraints(&angleRange);
    ADD_PROPERTY(Angle2,(360.0f));
    Angle2.setConstraints(&angleRange);
}

Circle::~Circle() = default;

short Circle::mustExecute() const
{
    if (Angle1.isTouched() ||
        Angle2.isTouched() ||
        Radius.isTouched())
        return 1;
    return Part::Primitive::mustExecute();
}

App::DocumentObjectExecReturn *Circle::execute()
{
    gp_Circ circle;
    circle.SetRadius(this->Radius.getValue());

    BRepBuilderAPI_MakeEdge clMakeEdge(circle, Base::toRadians<double>(this->Angle1.getValue()),
                                               Base::toRadians<double>(this->Angle2.getValue()));
    const TopoDS_Edge& edge = clMakeEdge.Edge();
    this->Shape.setValue(edge);
    return Primitive::execute();
}

void Circle::onChanged(const App::Property* prop)
{
    if (!isRestoring()) {
        if (prop == &Radius || prop == &Angle1 || prop == &Angle2){
            try {
                App::DocumentObjectExecReturn *ret = recompute();
                delete ret;
            }
            catch (...) {
            }
        }
    }
    Part::Feature::onChanged(prop); // clazy:exclude=skipped-base-method
}

void Circle::Restore(Base::XMLReader &reader)
{
    Base::ObjectStatusLocker<App::Property::Status, App::Property> lock(App::Property::User1, &Angle2, false);
    Primitive::Restore(reader);

    if (Angle2.testStatus(App::Property::User1)) {
        double tmp = Angle1.getValue();
        Angle1.setValue(Angle2.getValue());
        Angle2.setValue(tmp);
    }
}

void Circle::handleChangedPropertyName(Base::XMLReader &reader, const char * TypeName, const char *PropName)
{
    Base::Type type = Base::Type::fromName(TypeName);
    if (Angle2.getTypeId() == type && strcmp(PropName, "Angle0") == 0) {
        Angle2.Restore(reader);
        // set the flag to swap Angle1/Angle2 afterwards
        Angle2.setStatus(App::Property::User1, true);
    }
    else {
        Primitive::handleChangedPropertyName(reader, TypeName, PropName);
    }
}
