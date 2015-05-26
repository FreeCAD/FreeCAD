/***************************************************************************
 *   Copyright (c) J�rgen Riegel          (juergen.riegel@web.de) 2010     *
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

#include <Base/VectorPy.h>

#include <Mod/Part/App/Geometry.h>
#include <Mod/Part/App/GeometryCurvePy.h>
#include <Mod/Part/App/LinePy.h>
#include <Mod/Part/App/TopoShapePy.h>
#include <CXX/Objects.hxx>

#include "Sketch.h"
#include "Constraint.h"
#include "ConstraintPy.h"

// inclusion of the generated files (generated out of SketchPy.xml)
#include "SketchPy.h"
#include "SketchPy.cpp"

using namespace Sketcher;
using namespace Part;

// returns a string which represents the object e.g. when printed in python
std::string SketchPy::representation(void) const
{
    return std::string("<Sketch object>");
}

PyObject *SketchPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of SketchPy and the Twin object 
    return new SketchPy(new Sketch());
}

// constructor method
int SketchPy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}

// +++ methodes implementer ++++++++++++++++++++++++++++++++++++++++++++++++

PyObject* SketchPy::solve(PyObject *args)
{
    return Py::new_reference_to(Py::Int(getSketchPtr()->solve()));
}

PyObject* SketchPy::addGeometry(PyObject *args)
{
    PyObject *pcObj;
    if (!PyArg_ParseTuple(args, "O", &pcObj))
        return 0;

    if (PyObject_TypeCheck(pcObj, &(Part::GeometryPy::Type))) {
        Part::Geometry *geo = static_cast<Part::GeometryPy*>(pcObj)->getGeometryPtr();
        return Py::new_reference_to(Py::Int(this->getSketchPtr()->addGeometry(geo)));
    }
    else if (PyObject_TypeCheck(pcObj, &(PyList_Type)) ||
             PyObject_TypeCheck(pcObj, &(PyTuple_Type))) {
        std::vector<Part::Geometry *> geoList;
        Py::Sequence list(pcObj);
        for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
            if (PyObject_TypeCheck((*it).ptr(), &(Part::GeometryPy::Type))) {
                Part::Geometry *geo = static_cast<Part::GeometryPy*>((*it).ptr())->getGeometryPtr();
                geoList.push_back(geo);
            }
        }

        int ret = this->getSketchPtr()->addGeometry(geoList) + 1;
        std::size_t numGeo = geoList.size();
        Py::Tuple tuple(numGeo);
        for (std::size_t i=0; i<numGeo; ++i) {
            int geoId = ret - int(numGeo - i);
            tuple.setItem(i, Py::Int(geoId));
        }
        return Py::new_reference_to(tuple);
    }

    std::string error = std::string("type must be 'Geometry' or list of 'Geometry', not ");
    error += pcObj->ob_type->tp_name;
    throw Py::TypeError(error);
}

PyObject* SketchPy::addConstraint(PyObject *args)
{
    PyObject *pcObj;
    if (!PyArg_ParseTuple(args, "O", &pcObj))
        return 0;

    if (PyObject_TypeCheck(pcObj, &(PyList_Type)) || PyObject_TypeCheck(pcObj, &(PyTuple_Type))) {
        std::vector<Constraint*> values;
        Py::Sequence list(pcObj);
        for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
            if (PyObject_TypeCheck((*it).ptr(), &(ConstraintPy::Type))) {
                Constraint *con = static_cast<ConstraintPy*>((*it).ptr())->getConstraintPtr();
                values.push_back(con);
            }
        }

        int ret = getSketchPtr()->addConstraints(values) + 1;
        std::size_t numCon = values.size();
        Py::Tuple tuple(numCon);
        for (std::size_t i=0; i<numCon; ++i) {
            int conId = ret - int(numCon - i);
            tuple.setItem(i, Py::Int(conId));
        }
        return Py::new_reference_to(tuple);
    }
    else if(PyObject_TypeCheck(pcObj, &(ConstraintPy::Type))) {
        ConstraintPy  *pcObject = static_cast<ConstraintPy*>(pcObj);
        int ret = getSketchPtr()->addConstraint(pcObject->getConstraintPtr());
        return Py::new_reference_to(Py::Int(ret));
    }
    else {
        std::string error = std::string("type must be 'Constraint' or list of 'Constraint', not ");
        error += pcObj->ob_type->tp_name;
        throw Py::TypeError(error);
    }
}

PyObject* SketchPy::clear(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;

    getSketchPtr()->clear();

    Py_RETURN_NONE;
}

PyObject* SketchPy::movePoint(PyObject *args)
{
    int index1,index2;
    PyObject *pcObj;
    int relative=0;
    if (!PyArg_ParseTuple(args, "iiO!|i", &index1,&index2,&(Base::VectorPy::Type),&pcObj,&relative))
        return 0;
    Base::Vector3d* toPoint = static_cast<Base::VectorPy*>(pcObj)->getVectorPtr();

    return Py::new_reference_to(Py::Int(getSketchPtr()->movePoint(index1,(Sketcher::PointPos)index2,*toPoint,(relative>0))));
}

// +++ attributes implementer ++++++++++++++++++++++++++++++++++++++++++++++++

Py::Int SketchPy::getConstraint(void) const
{
    //return Py::Int();
    throw Py::AttributeError("Not yet implemented");
}

Py::Tuple SketchPy::getConstraints(void) const
{
    //return Py::Tuple();
    throw Py::AttributeError("Not yet implemented");
}

Py::Tuple SketchPy::getGeometries(void) const
{
    return getSketchPtr()->getPyGeometry();
}

Py::Object SketchPy::getShape(void) const
{
    return Py::Object(new TopoShapePy(new TopoShape(getSketchPtr()->toShape())));
}


// +++ custom attributes implementer ++++++++++++++++++++++++++++++++++++++++


PyObject *SketchPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int SketchPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0; 
}


