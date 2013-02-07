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
#endif

#include "FemConstraint.h"

#include <Base/Console.h>

using namespace Fem;

const char* Constraint::TypeEnums[]= {"Force on geometry","Fixed",
                                      "Bearing (radial free)", "Bearing (radial fixed)",
                                      "Pulley", "Gear (straight toothed)", NULL};

PROPERTY_SOURCE(Fem::Constraint, App::DocumentObject);

Constraint::Constraint()
{
    ADD_PROPERTY(Type,((long)0));
    Type.setEnums(TypeEnums);
    ADD_PROPERTY(Force,(0.0));
    ADD_PROPERTY_TYPE(References,(0,0),"Constraint",(App::PropertyType)(App::Prop_None),"Elements where the constraint is applied");
    ADD_PROPERTY_TYPE(Direction,(0),"Constraint",(App::PropertyType)(App::Prop_None),"Element giving direction of constraint");
    ADD_PROPERTY(Reversed,(0));
    ADD_PROPERTY(Distance,(0.0));
    ADD_PROPERTY_TYPE(Location,(0),"Constraint",(App::PropertyType)(App::Prop_None),"Element giving location where constraint is applied");
    ADD_PROPERTY(Diameter,(0.0));
    ADD_PROPERTY(OtherDiameter,(0.0));
    ADD_PROPERTY(CenterDistance,(0.0));
}

Constraint::~Constraint()
{
}

App::DocumentObjectExecReturn *Constraint::execute(void)
{
    // Ensure that the constraint symbols follow the changed geometry
    References.touch();
    return DocumentObject::StdReturn;
}

void Constraint::onChanged(const App::Property* prop)
{
    DocumentObject::onChanged(prop);
}
