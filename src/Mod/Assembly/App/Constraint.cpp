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
#include "ItemAssembly.h"


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
    touch();
    return App::DocumentObject::StdReturn;
}

boost::shared_ptr<Geometry3D> Constraint::initLink(ItemAssembly* ass, App::PropertyLinkSub& link) {

    //check if we have Assembly::ItemPart
    if( link.getValue()->getTypeId() != ItemPart::getClassTypeId() ) {
      Base::Console().Message("Link is not ItemPart, the constraint is invalid\n");
      return boost::shared_ptr<Geometry3D>();
    };
       
    Assembly::ItemPart* part = static_cast<Assembly::ItemPart*>(link.getValue());
    if(!part) 
      return boost::shared_ptr<Geometry3D>();
    
    //get the relevant solver in which the part needs to be added
    Assembly::ItemAssembly* p_ass = ass->getParentAssembly(part);
    if(!p_ass)
      return boost::shared_ptr<Geometry3D>();
    
    boost::shared_ptr<Solver> solver = p_ass->m_solver;
    if(!solver) 
      return boost::shared_ptr<Geometry3D>();    
    
    if(!solver->hasPart(part->Uid.getValueStr())) {
	part->m_part = solver->createPart(part->Placement.getValue(), part->Uid.getValueStr());
	part->m_part->connectSignal<dcm::recalculated>(boost::bind(&ItemPart::setCalculatedPlacement, part, _1));
    };    
    
    return part->getGeometry3D(link.getSubValues()[0].c_str());
}


void Constraint::init(ItemAssembly* ass) 
{
    m_first_geom = initLink(ass, First);
    m_second_geom = initLink(ass, Second);
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



