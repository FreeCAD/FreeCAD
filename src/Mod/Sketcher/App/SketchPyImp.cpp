/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2010     *
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

    if (PyObject_TypeCheck(pcObj, &(LinePy::Type))) {
        GeomLineSegment *line = static_cast<LinePy*>(pcObj)->getGeomLineSegmentPtr();
        return Py::new_reference_to(Py::Int(this->getSketchPtr()->addGeometry(line->clone())));
    }
    Py_Return; 
}

PyObject* SketchPy::addConstraint(PyObject *args)
{
    int ret = -1;
    PyObject *pcObj;
    if (!PyArg_ParseTuple(args, "O", &pcObj))
        return 0;

    if (PyList_Check(pcObj)) {
        Py_ssize_t nSize = PyList_Size(pcObj);
        std::vector<Constraint*> values;
        values.resize(nSize);

        for (Py_ssize_t i=0; i<nSize;++i) {
            PyObject* item = PyList_GetItem(pcObj, i);
            if (!PyObject_TypeCheck(item, &(ConstraintPy::Type))) {
                std::string error = std::string("types in list must be 'Constraint', not ");
                error += item->ob_type->tp_name;
                throw Py::TypeError(error);
            }

            values[i] = static_cast<ConstraintPy*>(item)->getConstraintPtr();
        }

        ret = getSketchPtr()->addConstraints(values);
    }
    else if(PyObject_TypeCheck(pcObj, &(ConstraintPy::Type))) {
        ConstraintPy  *pcObject = static_cast<ConstraintPy*>(pcObj);
        ret = getSketchPtr()->addConstraint(pcObject->getConstraintPtr());
    }
    else {
        std::string error = std::string("type must be 'Constraint' or list of 'Constraint', not ");
        error += pcObj->ob_type->tp_name;
        throw Py::TypeError(error);
    }

    return Py::new_reference_to(Py::Int(ret));

}

PyObject* SketchPy::clear(PyObject *args)
{
    int index;
    if (!PyArg_ParseTuple(args, "i", &index))
        return 0;

    return Py::new_reference_to(Py::Int(getSketchPtr()->addVerticalConstraint(index)));
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


