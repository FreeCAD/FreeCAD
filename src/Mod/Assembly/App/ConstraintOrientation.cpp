/***************************************************************************
 *   Copyright (c) 2012 Juergen Riegel <FreeCAD@juergen-riegel.net>
 *		   2013 Stefan Tröger  <stefantroeger@gmx.net>
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

#include <Base/Placement.h>
#include <Base/Console.h>

#include "ConstraintOrientation.h"
#include "ConstraintPy.h"

#include "ItemPart.h"


using namespace Assembly;

namespace Assembly {


PROPERTY_SOURCE(Assembly::ConstraintOrientation, Assembly::Constraint)

ConstraintOrientation::ConstraintOrientation() {
    ADD_PROPERTY(Orientation, (long(0)));

    std::vector<std::string> vec;
    vec.push_back("Parallel");
    vec.push_back("Perpendicular");
    vec.push_back("Equal");
    vec.push_back("Opposite");
    Orientation.setEnumVector(vec);
}

short ConstraintOrientation::mustExecute() const {
    //if (Sketch.isTouched() ||
    //    Length.isTouched())
    //    return 1;
    return 0;
}

App::DocumentObjectExecReturn* ConstraintOrientation::execute(void) {
    Base::Console().Message("Recalculate orientation constraint\n");
    touch();
    return App::DocumentObject::StdReturn;
}

void ConstraintOrientation::init(ItemAssembly* ass) {
    //init the parts and geometries
    Constraint::init(ass);

    //init the constraint
    dcm::Direction dir;
    switch(Orientation.getValue()) {
        case 0:
            dir = dcm::parallel;
            break;
        case 1:
            dir = dcm::perpendicular;
            break;
        case 2:
            dir = dcm::equal;
            break;
        default:
            dir = dcm::opposite;
    };

    m_constraint = ass->m_solver->createConstraint3D(getNameInDocument(), m_first_geom, m_second_geom, dcm::orientation = dir);
}


}
