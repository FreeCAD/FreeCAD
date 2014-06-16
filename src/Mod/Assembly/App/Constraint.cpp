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
#include "Product.h"


using namespace Assembly;

namespace Assembly {

struct ConstraintInitException : std::exception {
    const char* what() const throw() {
        return "Constraint cout not be initialised: unsoported geometry";
    }
};
struct ConstraintPartException : std::exception {
    const char* what() const throw() {
        return "Constraint cout not be initialised: parts are invalid";
    }
};
struct ConstraintLinkException : std::exception {
    const char* what() const throw() {
        return "Constraint cout not be initialised: unsoported link type";
    }
};

PROPERTY_SOURCE(Assembly::Constraint, App::DocumentObject)

const char* Constraint::OrientationEnums[]    = {"Parallel","Equal","Opposite","Perpendicular",NULL};
const char* Constraint::TypeEnums[]    = {"Fix","Distance","Orientation","Angle","Align","Coincident","None",NULL};
const char* Constraint::SolutionSpaceEnums[]    = {"Bidirectional","PositivDirectional","NegativeDirectional",NULL};

Constraint::Constraint()
{
    ADD_PROPERTY(First, (0));
    ADD_PROPERTY(Second,(0));
    ADD_PROPERTY(Value,(0));
    ADD_PROPERTY(Orientation, (long(0)));
    Orientation.setEnums(OrientationEnums);
    ADD_PROPERTY(Type, (long(6)));
    Type.setEnums(TypeEnums);
    ADD_PROPERTY(SolutionSpace, (long(0)));
    SolutionSpace.setEnums(SolutionSpaceEnums);
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
    return App::DocumentObject::StdReturn;
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



