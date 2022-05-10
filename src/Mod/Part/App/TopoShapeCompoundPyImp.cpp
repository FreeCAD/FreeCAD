/***************************************************************************
 *   Copyright (c) 2008 Jürgen Riegel <juergen.riegel@web.de>              *
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

#include "TopoShape.h"

#ifndef _PreComp_
# include <BRep_Builder.hxx>
# include <Standard_Failure.hxx>
# include <TopoDS_Compound.hxx>
# include <TopTools_HSequenceOfShape.hxx>
# include <ShapeAnalysis_FreeBounds.hxx>
# include <Precision.hxx>
# include <TopExp_Explorer.hxx>
#endif

#include "OCCError.h"

// inclusion of the generated files (generated out of TopoShapeCompoundPy.xml)
#include "TopoShapeCompoundPy.h"
#include "TopoShapeCompoundPy.cpp"

using namespace Part;

// returns a string which represents the object e.g. when printed in python
std::string TopoShapeCompoundPy::representation(void) const
{
    std::stringstream str;
    str << "<Compound object at " << getTopoShapePtr() << ">";
    return str.str();
}

PyObject *TopoShapeCompoundPy::PyMake(struct _typeobject *, PyObject *, PyObject *)
{
    return new TopoShapeCompoundPy(new TopoShape);
}

// constructor method
int TopoShapeCompoundPy::PyInit(PyObject* args, PyObject* /*kwd*/)
{
    if (PyArg_ParseTuple(args, "")) {
        // Undefined Compound
        getTopoShapePtr()->setShape(TopoDS_Compound());
        return 0;
    }

    PyErr_Clear();
    PyObject *pcObj;
    if (!PyArg_ParseTuple(args, "O", &pcObj))
        return -1;

    BRep_Builder builder;
    TopoDS_Compound Comp;
    builder.MakeCompound(Comp);

    try {
        Py::Sequence list(pcObj);
        for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
            if (PyObject_TypeCheck((*it).ptr(), &(Part::TopoShapePy::Type))) {
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

PyObject*  TopoShapeCompoundPy::add(PyObject *args)
{
    PyObject *obj;
    if (!PyArg_ParseTuple(args, "O!", &(Part::TopoShapePy::Type), &obj))
        return nullptr;

    BRep_Builder builder;
    TopoDS_Shape comp = getTopoShapePtr()->getShape();

    try {
        const TopoDS_Shape& sh = static_cast<TopoShapePy*>(obj)->
            getTopoShapePtr()->getShape();
        if (!sh.IsNull())
            builder.Add(comp, sh);
    }
    catch (Standard_Failure& e) {

        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }

    getTopoShapePtr()->setShape(comp);

    Py_Return;
}

PyObject* TopoShapeCompoundPy::connectEdgesToWires(PyObject *args)
{
    PyObject *shared=Py_True;
    double tol = Precision::Confusion();
    if (!PyArg_ParseTuple(args, "|O!d",&PyBool_Type,&shared,&tol))
        return nullptr;

    try {
        const TopoDS_Shape& s = getTopoShapePtr()->getShape();

        Handle(TopTools_HSequenceOfShape) hEdges = new TopTools_HSequenceOfShape();
        Handle(TopTools_HSequenceOfShape) hWires = new TopTools_HSequenceOfShape();
        for (TopExp_Explorer xp(s, TopAbs_EDGE); xp.More(); xp.Next())
            hEdges->Append(xp.Current());

        ShapeAnalysis_FreeBounds::ConnectEdgesToWires(hEdges, tol, PyObject_IsTrue(shared) ? Standard_True : Standard_False, hWires);

        TopoDS_Compound comp;
        BRep_Builder builder;
        builder.MakeCompound(comp);

        int len = hWires->Length();
        for(int i=1;i<=len;i++) {
            builder.Add(comp, hWires->Value(i));
        }

        getTopoShapePtr()->setShape(comp);
        return new TopoShapeCompoundPy(new TopoShape(comp));
    }
    catch (Standard_Failure& e) {

        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

PyObject *TopoShapeCompoundPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int TopoShapeCompoundPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
