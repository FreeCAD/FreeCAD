/***************************************************************************
 *   Copyright (c) 2013 Jan Rheinländer <jrheinlaender[at]users.sourceforge.net>     *
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
#include <gp_Pnt.hxx>
#include <gp_Pln.hxx>
#include <gp_Lin.hxx>
#include <TopoDS.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <Precision.hxx>
#endif

#include "FemConstraintForce.h"

#include <Mod/Part/App/PartFeature.h>
#include <Base/Console.h>

using namespace Fem;

PROPERTY_SOURCE(Fem::ConstraintForce, Fem::Constraint);

ConstraintForce::ConstraintForce()
{
    ADD_PROPERTY(Force,(0.0));
    ADD_PROPERTY_TYPE(Direction,(0),"ConstraintForce",(App::PropertyType)(App::Prop_None),
                      "Element giving direction of constraint");
    ADD_PROPERTY(Reversed,(0));
    ADD_PROPERTY_TYPE(Points,(Base::Vector3f()),"ConstraintForce",App::PropertyType(App::Prop_ReadOnly|App::Prop_Output),
                      "Points where arrows are drawn");
    ADD_PROPERTY_TYPE(DirectionVector,(Base::Vector3f(0,0,1)),"ConstraintForce",App::PropertyType(App::Prop_ReadOnly|App::Prop_Output),
                      "Direction of arrows");
    naturalDirectionVector = Base::Vector3f(0,0,1);
    Points.setValues(std::vector<Base::Vector3f>());
}

App::DocumentObjectExecReturn *ConstraintForce::execute(void)
{
    return Constraint::execute();
}

void ConstraintForce::onChanged(const App::Property* prop)
{
    // Note: If we call this at the end, then the arrows are not oriented correctly initially
    // because the NormalDirection has not been calculated yet
    Constraint::onChanged(prop);

    if (prop == &References) {
        std::vector<Base::Vector3f> points;
        std::vector<Base::Vector3f> normals;
        getPoints(points, normals);
        Points.setValues(points); // We don't use the normals because all arrows should have the same direction
        Points.touch(); // This triggers ViewProvider::updateData()
    } else if (prop == &Direction) {
        App::DocumentObject* obj = Direction.getValue();
        std::vector<std::string> names = Direction.getSubValues();
        if (names.size() == 0) {
            return;
        }
        std::string subName = names.front();
        Part::Feature* feat = static_cast<Part::Feature*>(obj);
        TopoDS_Shape sh = feat->Shape.getShape().getSubShape(subName.c_str());
        gp_Dir dir;

        if (sh.ShapeType() == TopAbs_FACE) {
            BRepAdaptor_Surface surface(TopoDS::Face(sh));
            if (surface.GetType() == GeomAbs_Plane) {
                dir = surface.Plane().Axis().Direction();
            } else {
                return; // "Direction must be a planar face or linear edge"
            }
        } else if (sh.ShapeType() == TopAbs_EDGE) {
            BRepAdaptor_Curve line(TopoDS::Edge(sh));
            if (line.GetType() == GeomAbs_Line) {
                dir = line.Line().Direction();
            } else {
                return; // "Direction must be a planar face or linear edge"
            }
        }

        Base::Vector3f direction(dir.X(), dir.Y(), dir.Z());
        direction.Normalize();
        naturalDirectionVector = direction;
        if (Reversed.getValue())
            direction = -direction;
        DirectionVector.setValue(direction);
        DirectionVector.touch();
    } else if (prop == &Reversed) {
        if (Reversed.getValue() && (DirectionVector.getValue() == naturalDirectionVector)) {
            DirectionVector.setValue(-naturalDirectionVector);
            DirectionVector.touch();
        } else if (!Reversed.getValue() && (DirectionVector.getValue() != naturalDirectionVector)) {
            DirectionVector.setValue(naturalDirectionVector);
            DirectionVector.touch();
        }
    } else if (prop == &NormalDirection) {
        // Set a default direction if no direction reference has been given
        if (Direction.getValue() == NULL) {
            DirectionVector.setValue(NormalDirection.getValue());
            naturalDirectionVector = NormalDirection.getValue();
        }
    }
}
