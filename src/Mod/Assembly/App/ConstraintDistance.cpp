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

#include "ConstraintDistance.h"
#include "ConstraintPy.h"

#include "ItemPart.h"

using namespace Assembly;

namespace Assembly {


PROPERTY_SOURCE(Assembly::ConstraintDistance, Assembly::Constraint)

ConstraintDistance::ConstraintDistance()
{
    ADD_PROPERTY(Distance,(0));
}

PyObject *ConstraintDistance::getPyObject(void)
{
    if (PythonObject.is(Py::_None())){
        // ref counter is set to 1
        PythonObject = Py::Object(new ConstraintPy(this),true);
    }
    return Py::new_reference_to(PythonObject); 
}

short ConstraintDistance::mustExecute() const
{
    //if (Sketch.isTouched() ||
    //    Length.isTouched())
    //    return 1;
    return 0;
}

App::DocumentObjectExecReturn *ConstraintDistance::execute(void)
{
    Base::Console().Message("Recalculate axis constraint\n");
    touch();
    return App::DocumentObject::StdReturn;
}

void ConstraintDistance::init(Assembly::ItemAssembly* ass)
{    
      //init the parts and geometries
      Constraint::init(ass);
      
      //init the constraint
      m_constraint = ass->m_solver->createConstraint3D(getNameInDocument(), m_first_geom, m_second_geom, dcm::distance = Distance.getValue());
}


}