/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2008     *
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
#include <BRep_Builder.hxx>
#include <Standard_Failure.hxx>
#include <TopoDS_CompSolid.hxx>
#endif

#include "OCCError.h"
#include "TopoShape.h"

// inclusion of the generated files (generated out of TopoShapeCompSolidPy.xml)
#include "TopoShapeSolidPy.h"
#include "TopoShapeCompSolidPy.h"
#include "TopoShapeCompSolidPy.cpp"

using namespace Part;

// returns a string which represents the object e.g. when printed in python
std::string TopoShapeCompSolidPy::representation(void) const
{
    std::stringstream str;
    str << "<CompSolid object at " << getTopoShapePtr() << ">";

    return str.str();
}

PyObject *TopoShapeCompSolidPy::PyMake(struct _typeobject *, PyObject *, PyObject *)
{
    return new TopoShapeCompSolidPy(new TopoShape);
}

int TopoShapeCompSolidPy::PyInit(PyObject* args, PyObject* /*kwd*/)
{
    PyObject *pcObj;
    if (!PyArg_ParseTuple(args, "O", &pcObj))
        return -1;

    BRep_Builder builder;
    TopoDS_CompSolid Comp;
    builder.MakeCompSolid(Comp);

    try {
        Py::Sequence list(pcObj);
        for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
            if (PyObject_TypeCheck((*it).ptr(), &(Part::TopoShapeSolidPy::Type))) {
                const TopoDS_Shape& sh = static_cast<TopoShapePy*>((*it).ptr())->
                    getTopoShapePtr()->getShape();
                if (!sh.IsNull())
                    builder.Add(Comp, sh);
            }
        }
    }
    catch (Standard_Failure& e) {

        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return -1;
    }

    getTopoShapePtr()->setShape(Comp);
    return 0;
}

PyObject*  TopoShapeCompSolidPy::add(PyObject *args)
{
    PyObject *obj;
    if (!PyArg_ParseTuple(args, "O!", &(Part::TopoShapeSolidPy::Type), &obj))
        return NULL;

    BRep_Builder builder;
    TopoDS_Shape comp = getTopoShapePtr()->getShape();
    
    try {
        const TopoDS_Shape& sh = static_cast<TopoShapePy*>(obj)->
            getTopoShapePtr()->getShape();
        if (!sh.IsNull())
            builder.Add(comp, sh);
        else
            Standard_Failure::Raise("Cannot empty shape to compound solid");
    }
    catch (Standard_Failure& e) {

        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return 0;
    }

    getTopoShapePtr()->setShape(comp);

    Py_Return;
}

PyObject *TopoShapeCompSolidPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int TopoShapeCompSolidPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0; 
}
