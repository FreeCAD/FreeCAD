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

#include "FemConstraintPulley.h"

#include <Mod/Part/App/PartFeature.h>
#include <Base/Console.h>

using namespace Fem;

PROPERTY_SOURCE(Fem::ConstraintPulley, Fem::ConstraintBearing);

ConstraintPulley::ConstraintPulley()
{
    ADD_PROPERTY(Diameter,(0));
    ADD_PROPERTY(OtherDiameter,(0));
    ADD_PROPERTY(CenterDistance,(0));
    ADD_PROPERTY_TYPE(Angle,(0),"ConstraintPulley",App::PropertyType(App::Prop_ReadOnly|App::Prop_Output),
                      "Angle of pulley forces");
}

App::DocumentObjectExecReturn *ConstraintPulley::execute(void)
{
    return ConstraintBearing::execute();
}

void ConstraintPulley::onChanged(const App::Property* prop)
{
    ConstraintBearing::onChanged(prop);

    if ((prop == &Diameter) || (prop == &OtherDiameter) || (prop == &CenterDistance)) {
        if (CenterDistance.getValue() > Precision::Confusion()) {
            Angle.setValue(asin((Diameter.getValue() - OtherDiameter.getValue())/2/CenterDistance.getValue()));
            Angle.touch();
        }
    }
}
