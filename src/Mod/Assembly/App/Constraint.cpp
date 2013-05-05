/***************************************************************************
 *   Copyright (c) 2012 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
 *   Copyright (c) 2013 Stefan Tröger  <stefantroeger@gmx.net>             *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development m_solvertem.         *
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

#include <Standard_Failure.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopExp.hxx>
#include <TopoDS.hxx>
#include <GeomAbs_SurfaceType.hxx>
#include <gp_Pln.hxx>
#include <GeomAbs_CurveType.hxx>
#include <BRep_Tool.hxx>
#include <TopoDS_Vertex.hxx>
#include <gp_Pnt.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <TopoDS_Edge.hxx>
#include <gp_Cylinder.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <TopoDS_Face.hxx>

#include <Base/Placement.h>
#include <Base/Console.h>

#include "Constraint.h"
#include "ConstraintPy.h"
#include "Item.h"
#include "ItemPart.h"


using namespace Assembly;

namespace Assembly {


PROPERTY_SOURCE(Assembly::Constraint, App::DocumentObject)

Constraint::Constraint()
{
    ADD_PROPERTY(First, (0));
    ADD_PROPERTY(Second,(0));
}

short Constraint::mustExecute() const
{
    //if (Sketch.isTouched() ||
    //    Length.isTouched())
    //    return 1;
    return 0;
}

App::DocumentObjectExecReturn *Constraint::execute(void)
{
 
    return App::DocumentObject::StdReturn;
}

void Constraint::init(boost::shared_ptr< Solver > solver) {

    //check if we have Assembly::ItemPart's
    if( First.getValue()->getTypeId() != ItemPart::getClassTypeId() ||
	Second.getValue()->getTypeId() != ItemPart::getClassTypeId() ) {
      Base::Console().Message("Links are not ItemPart's, the constraint is invalid\n");
      return;
    };
      
    //see if the parts are already initialized for the solver
    Assembly::ItemPart* part1 = static_cast<Assembly::ItemPart*>(First.getValue());
    if(!part1->m_part) {
      part1->m_part = solver->createPart(part1->Placement.getValue(), part1->Uid.getValueStr());
      part1->m_part->connectSignal<dcm::recalculated>(boost::bind(&ItemPart::setCalculatedPlacement, part1, _1));
    }
      
    Assembly::ItemPart* part2 = static_cast<Assembly::ItemPart*>(Second.getValue());
    if(!part2->m_part) {
      part2->m_part = solver->createPart(part2->Placement.getValue(), part2->Uid.getValueStr());
      part2->m_part->connectSignal<dcm::recalculated>(boost::bind(&ItemPart::setCalculatedPlacement, part2, _1));
    }

    //let's get the geometrys
    m_first_geom = part1->getGeometry3D(First.getSubValues()[0].c_str());
    m_second_geom = part2->getGeometry3D(Second.getSubValues()[0].c_str());
    
    if(!m_first_geom || !m_second_geom) {
      Base::Console().Message("Unable to initialize geometry\n");
      return;
    };
}

PyObject *Constraint::getPyObject(void)
{
    if (PythonObject.is(Py::_None())){
        // ref counter is set to 1
        PythonObject = Py::Object(new ConstraintPy(this),true);
    }
    return Py::new_reference_to(PythonObject); 
}


}