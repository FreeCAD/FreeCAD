/***************************************************************************
 *   Copyright (c) 2010 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
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

#include "ConstraintAlignment.h"


using namespace Assembly;

namespace Assembly {


PROPERTY_SOURCE(Assembly::ConstraintAlignment, Assembly::Constraint)

ConstraintAlignment::ConstraintAlignment()
{
    ADD_PROPERTY(Offset,(0));
    ADD_PROPERTY(Orientation, (long(0)));

    std::vector<std::string> vec;
    vec.push_back("Parallel");
    vec.push_back("Equal");
    vec.push_back("Opposite");
    Orientation.setEnumVector(vec);
}

short ConstraintAlignment::mustExecute() const
{
    //if (Sketch.isTouched() ||
    //    Length.isTouched())
    //    return 1;
    return 0;
}

App::DocumentObjectExecReturn *ConstraintAlignment::execute(void)
{
 
    return App::DocumentObject::StdReturn;
}

void ConstraintAlignment::init(ItemAssembly* ass) {

    //cant use the base class init as we only need one part
    Constraint::init(ass);

        //init the constraint
    dcm::Direction dir;
    switch(Orientation.getValue()) {
        case 0:
            dir = dcm::parallel;
            break;
        case 1:
            dir = dcm::equal;
            break;
        default:
            dir = dcm::opposite;
    };

    m_constraint = ass->m_solver->createConstraint3D(getNameInDocument(), m_first_geom, m_second_geom, dcm::alignment(dir, Offset.getValue()));
};

}