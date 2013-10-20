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

#include <math.h>

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

struct ConstraintInitException : std::exception {
    const char* what() const throw() {
        return "Constraint cout not be initialised: unsoported geometry";
    }
};
struct ConstraintLinkException : std::exception {
    const char* what() const throw() {
        return "Constraint cout not be initialised: unsoported link type";
    }
};

PROPERTY_SOURCE(Assembly::Constraint, App::DocumentObject)

Constraint::Constraint()
{
    ADD_PROPERTY(First, (0));
    ADD_PROPERTY(Second,(0));
    ADD_PROPERTY(Value,(0));
    ADD_PROPERTY(Orientation, (long(0)));
    ADD_PROPERTY(Type, (long(6)));
    ADD_PROPERTY(SolutionSpace, (long(0)));

    std::vector<std::string> vec;
    vec.push_back("Parallel");
    vec.push_back("Equal");
    vec.push_back("Opposite");
    vec.push_back("Perpendicular");
    Orientation.setEnumVector(vec);

    std::vector<std::string> vec2;
    vec2.push_back("Fix");
    vec2.push_back("Distance");
    vec2.push_back("Orientation");
    vec2.push_back("Angle");
    vec2.push_back("Align");
    vec2.push_back("Coincident");
    vec2.push_back("None");
    Type.setEnumVector(vec2);
    
    std::vector<std::string> vec3;
    vec3.push_back("Bidirectional");
    vec3.push_back("Positiv directional");
    vec3.push_back("Negative directional");
    SolutionSpace.setEnumVector(vec3);
}

short Constraint::mustExecute() const
{
    //if (Sketch.isTouched() ||
    //    Length.isTouched())
    //    return 1;
    return 0;
}

App::DocumentObjectExecReturn* Constraint::execute(void)
{
    touch();
    return App::DocumentObject::StdReturn;
}

boost::shared_ptr<Geometry3D> Constraint::initLink(App::PropertyLinkSub& link) {

    //empty links are allows
    if(!link.getValue())
        return boost::shared_ptr<Geometry3D>();

    //check if we have Assembly::ItemPart
    if(link.getValue()->getTypeId() != ItemPart::getClassTypeId()) {
        throw ConstraintLinkException();
        return boost::shared_ptr<Geometry3D>();
    };

    Assembly::ItemPart* part = static_cast<Assembly::ItemPart*>(link.getValue());
    if(!part)
        return boost::shared_ptr<Geometry3D>();

    //get the relevant solver in which the part needs to be added
    part->ensureInitialisation();

    return part->getGeometry3D(link.getSubValues()[0].c_str());
}


void Constraint::init(Assembly::ItemAssembly* ass)
{
    Assembly::ItemPart* part1, *part2;

    if(First.getValue()) {
        m_first_geom = initLink(First);
        part1 = static_cast<Assembly::ItemPart*>(First.getValue());
    }

    if(Second.getValue()) {
        m_second_geom = initLink(Second);
        part2= static_cast<Assembly::ItemPart*>(Second.getValue());
    }

    //fix constraint
    if(Type.getValue() == 0) {
        if(part1)
            part1->m_part->fix(true);
        else
            if(part2)
                part2->m_part->fix(true);
    };

    //all other constraints need poth parts
    if(!part1 || !part2)
        return;

    //we may need the orientation
    dcm::Direction dir;
    switch(Orientation.getValue()) {
    case 0:
        dir = dcm::parallel;
        break;
    case 1:
        dir = dcm::equal;
        break;
    case 2:
        dir = dcm::opposite;
        break;
    default:
        dir = dcm::perpendicular;
    };

    //we may need the SolutionSpace
    dcm::SolutionSpace sspace;
    switch(SolutionSpace.getValue()) {
    case 0:
        sspace = dcm::bidirectional;
        break;
    case 1:
        sspace = dcm::positiv_directional;
        break;
    default:
        sspace = dcm::negative_directional;
    };
    
    //distance constraint
    if(Type.getValue() == 1)
        m_constraint = ass->m_solver->createConstraint3D(getNameInDocument(), m_first_geom, m_second_geom, (dcm::distance = Value.getValue()) & (dcm::distance=sspace));

    //orientation constraint
    if(Type.getValue() == 2)
        m_constraint = ass->m_solver->createConstraint3D(getNameInDocument(), m_first_geom, m_second_geom, dcm::orientation = dir);

    //angle constraint
    if(Type.getValue() == 3)
        m_constraint = ass->m_solver->createConstraint3D(getNameInDocument(), m_first_geom, m_second_geom, dcm::angle = Value.getValue()*M_PI/180.);

    //alignemnt constraint
    if(Type.getValue() == 4)
        m_constraint = ass->m_solver->createConstraint3D(getNameInDocument(), m_first_geom, m_second_geom, (dcm::alignment=dir) & (dcm::alignment=Value.getValue()) & (dcm::alignment=sspace));

    //coincident constraint
    if(Type.getValue() == 5)
        m_constraint = ass->m_solver->createConstraint3D(getNameInDocument(), m_first_geom, m_second_geom, dcm::coincidence = dir);
}

PyObject* Constraint::getPyObject(void)
{
    if(PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new ConstraintPy(this),true);
    }
    return Py::new_reference_to(PythonObject);
}


}



