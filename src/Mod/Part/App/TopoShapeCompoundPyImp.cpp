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

#include "TopoShape.h"
#include <BRep_Builder.hxx>
#include <Standard_Failure.hxx>
#include <TopoDS_Compound.hxx>
#include <TopTools_HSequenceOfShape.hxx>
#include <ShapeAnalysis_FreeBounds.hxx>
#include <Precision.hxx>
#include <TopExp_Explorer.hxx>
#include "TopoShapeOpCode.h"
#include "PartPyCXX.h"
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
    PyObject *pcObj;
    if (!PyArg_ParseTuple(args, "O", &pcObj))
        return -1;

#ifndef FC_NO_ELEMENT_MAP
    PY_TRY {
        getTopoShapePtr()->makECompound(getPyShapes(pcObj));
    } _PY_CATCH_OCC(return(-1))
    return 0;
#else
    BRep_Builder builder;
    TopoDS_Compound Comp;
    builder.MakeCompound(Comp);
    
    PY_TRY {
        Py::Sequence list(pcObj);
        for (Py::Sequence::iterator it = list.begin(); it != list.end(); ++it) {
            if (PyObject_TypeCheck((*it).ptr(), &(Part::TopoShapePy::Type))) {
                const TopoDS_Shape& sh = static_cast<TopoShapePy*>((*it).ptr())->
                    getTopoShapePtr()->getShape();
                if (!sh.IsNull())
                    builder.Add(Comp, sh);
            }
        }
    } _PY_CATCH_OCC(return(-1))

    getTopoShapePtr()->setShape(Comp);
    return 0;
#endif
}

PyObject*  TopoShapeCompoundPy::add(PyObject *args)
{
    PyObject *obj;
    if (!PyArg_ParseTuple(args, "O", &obj))
        return NULL;

    BRep_Builder builder;
    TopoDS_Shape comp = getTopoShapePtr()->getShape();
    auto shapes = getPyShapes(obj);
    
    PY_TRY {
        for(auto &s : shapes) {
            if(!s.isNull())
                builder.Add(comp,s.getShape());
        }
    } PY_CATCH_OCC

#ifndef FC_NO_ELEMENT_MAP
    PY_TRY {
        auto &self = *getTopoShapePtr();
        shapes.push_back(self);
        TopoShape tmp(self.Tag,self.Hasher,comp);
        tmp.mapSubElement(shapes);
        self = tmp;
    }PY_CATCH_OCC
#else
    getTopoShapePtr()->setShape(comp);
#endif
    Py_Return;
}

PyObject* TopoShapeCompoundPy::connectEdgesToWires(PyObject *args)
{
    PyObject *shared=Py_True;
    double tol = Precision::Confusion();
    if (!PyArg_ParseTuple(args, "|O!d",&PyBool_Type,&shared,&tol))
        return 0;

    PY_TRY {
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
    } PY_CATCH_OCC
}

PyObject *TopoShapeCompoundPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int TopoShapeCompoundPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0; 
}
