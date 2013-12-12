/***************************************************************************
 *   Copyright (c) 2012 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <gp_Ax2.hxx>
# include <gp_Dir.hxx>
# include <gp_Pnt.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Wire.hxx>
# include <BRepOffsetAPI_MakePipeShell.hxx>
# include <TopTools_ListIteratorOfListOfShape.hxx>
#endif

#include "TopoShapePy.h"
#include "BRepOffsetAPI_MakePipeShellPy.h"
#include "BRepOffsetAPI_MakePipeShellPy.cpp"
#include "Tools.h"
#include <Base/VectorPy.h>
#include <Base/GeometryPyCXX.h>

using namespace Part;

PyObject *BRepOffsetAPI_MakePipeShellPy::PyMake(struct _typeobject *, PyObject *args, PyObject *)  // Python wrapper
{
    // create a new instance of BRepOffsetAPI_MakePipeShellPy and the Twin object 
    PyObject* obj;
    if (!PyArg_ParseTuple(args, "O!",&(TopoShapePy::Type),&obj))
        return 0;
    const TopoDS_Shape& wire = static_cast<TopoShapePy*>(obj)->getTopoShapePtr()->_Shape;
    if (!wire.IsNull() && wire.ShapeType() == TopAbs_WIRE) {
        return new BRepOffsetAPI_MakePipeShellPy(new BRepOffsetAPI_MakePipeShell(TopoDS::Wire(wire)));
    }

    PyErr_SetString(PyExc_Exception, "A valid wire is needed as argument");
    return 0;
}

// constructor method
int BRepOffsetAPI_MakePipeShellPy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}

// returns a string which represents the object e.g. when printed in python
std::string BRepOffsetAPI_MakePipeShellPy::representation(void) const
{
    return std::string("<BRepOffsetAPI_MakePipeShell object>");
}

PyObject* BRepOffsetAPI_MakePipeShellPy::setFrenetMode(PyObject *args)
{
    PyObject *obj;
    if (!PyArg_ParseTuple(args, "O!",&PyBool_Type,&obj))
        return 0;
    this->getBRepOffsetAPI_MakePipeShellPtr()->SetMode(PyObject_IsTrue(obj) ? Standard_True : Standard_False);
    Py_Return;
}

PyObject* BRepOffsetAPI_MakePipeShellPy::setTrihedronMode(PyObject *args)
{
    PyObject *pnt, *dir;
    if (!PyArg_ParseTuple(args, "O!O!",&Base::VectorPy::Type,&pnt
                                      ,&Base::VectorPy::Type,&dir))
        return 0;
    gp_Pnt p = Base::convertTo<gp_Pnt>(Py::Vector(pnt,false).toVector());
    gp_Dir d = Base::convertTo<gp_Dir>(Py::Vector(dir,false).toVector());
    this->getBRepOffsetAPI_MakePipeShellPtr()->SetMode(gp_Ax2(p,d));
    Py_Return;
}

PyObject* BRepOffsetAPI_MakePipeShellPy::setBiNormalMode(PyObject *args)
{
    PyObject *dir;
    if (!PyArg_ParseTuple(args, "O!",&Base::VectorPy::Type,&dir))
        return 0;
    gp_Dir d = Base::convertTo<gp_Dir>(Py::Vector(dir,false).toVector());
    this->getBRepOffsetAPI_MakePipeShellPtr()->SetMode(d);
    Py_Return;
}

PyObject* BRepOffsetAPI_MakePipeShellPy::setSpineSupport(PyObject *args)
{
    PyObject *shape;
    if (!PyArg_ParseTuple(args, "O!",&Part::TopoShapePy::Type,&shape))
        return 0;
    const TopoDS_Shape& s = static_cast<Part::TopoShapePy*>(shape)->getTopoShapePtr()->_Shape;
    Standard_Boolean ok = this->getBRepOffsetAPI_MakePipeShellPtr()->SetMode(s);
    return Py::new_reference_to(Py::Boolean(ok ? true : false));
}

PyObject* BRepOffsetAPI_MakePipeShellPy::setAuxiliarySpine(PyObject *args)
{
    PyObject *spine, *curv, *keep;
    if (!PyArg_ParseTuple(args, "O!O!O!",&Part::TopoShapePy::Type,&spine
                                        ,&PyBool_Type,&curv
                                        ,&PyBool_Type,&keep))
        return 0;
    const TopoDS_Shape& s = static_cast<Part::TopoShapePy*>(spine)->getTopoShapePtr()->_Shape;
    if (s.IsNull() || s.ShapeType() != TopAbs_WIRE) {
        PyErr_SetString(PyExc_TypeError, "spine is not a wire");
        return 0;
    }

    this->getBRepOffsetAPI_MakePipeShellPtr()->SetMode(
        TopoDS::Wire(s),
        PyObject_IsTrue(curv) ? Standard_True : Standard_False,
        PyObject_IsTrue(keep) ? Standard_True : Standard_False);
    Py_Return;
}

PyObject* BRepOffsetAPI_MakePipeShellPy::add(PyObject *args)
{
    PyObject *prof, *curv=Py_False, *keep=Py_False;
    if (!PyArg_ParseTuple(args, "O!|O!O!",&Part::TopoShapePy::Type,&prof
                                         ,&PyBool_Type,&curv
                                         ,&PyBool_Type,&keep))
        return 0;
    const TopoDS_Shape& s = static_cast<Part::TopoShapePy*>(prof)->getTopoShapePtr()->_Shape;
    this->getBRepOffsetAPI_MakePipeShellPtr()->Add(s,
        PyObject_IsTrue(curv) ? Standard_True : Standard_False,
        PyObject_IsTrue(keep) ? Standard_True : Standard_False);
    Py_Return;
}

PyObject* BRepOffsetAPI_MakePipeShellPy::remove(PyObject *args)
{
    PyObject *prof;
    if (!PyArg_ParseTuple(args, "O!",&Part::TopoShapePy::Type,&prof))
        return 0;
    const TopoDS_Shape& s = static_cast<Part::TopoShapePy*>(prof)->getTopoShapePtr()->_Shape;
    this->getBRepOffsetAPI_MakePipeShellPtr()->Delete(s);
    Py_Return;
}

PyObject* BRepOffsetAPI_MakePipeShellPy::isReady(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;
    Standard_Boolean ok = this->getBRepOffsetAPI_MakePipeShellPtr()->IsReady();
    return Py::new_reference_to(Py::Boolean(ok ? true : false));
}

PyObject* BRepOffsetAPI_MakePipeShellPy::getStatus(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;
    Standard_Integer val = this->getBRepOffsetAPI_MakePipeShellPtr()->GetStatus();
    return Py::new_reference_to(Py::Int(val));
}

PyObject* BRepOffsetAPI_MakePipeShellPy::makeSolid(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;
    Standard_Boolean ok = this->getBRepOffsetAPI_MakePipeShellPtr()->MakeSolid();
    return Py::new_reference_to(Py::Boolean(ok ? true : false));
}

PyObject* BRepOffsetAPI_MakePipeShellPy::build(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;
    this->getBRepOffsetAPI_MakePipeShellPtr()->Build();
    Py_Return;
}

PyObject* BRepOffsetAPI_MakePipeShellPy::shape(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;
    const TopoDS_Shape& shape = this->getBRepOffsetAPI_MakePipeShellPtr()->Shape();
    return new TopoShapePy(new TopoShape(shape));
}

PyObject* BRepOffsetAPI_MakePipeShellPy::firstShape(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;
    TopoDS_Shape shape = this->getBRepOffsetAPI_MakePipeShellPtr()->FirstShape();
    return new TopoShapePy(new TopoShape(shape));
}

PyObject* BRepOffsetAPI_MakePipeShellPy::lastShape(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;
    TopoDS_Shape shape = this->getBRepOffsetAPI_MakePipeShellPtr()->LastShape();
    return new TopoShapePy(new TopoShape(shape));
}

PyObject* BRepOffsetAPI_MakePipeShellPy::generated(PyObject *args)
{
    PyObject *shape;
    if (!PyArg_ParseTuple(args, "O!",&Part::TopoShapePy::Type,&shape))
        return 0;
    const TopoDS_Shape& s = static_cast<Part::TopoShapePy*>(shape)->getTopoShapePtr()->_Shape;
    const TopTools_ListOfShape& list = this->getBRepOffsetAPI_MakePipeShellPtr()->Generated(s);

    Py::List shapes;
    TopTools_ListIteratorOfListOfShape it;
    for (it.Initialize(list); it.More(); it.Next()) {
        const TopoDS_Shape& s = it.Value();
        shapes.append(Py::asObject(new TopoShapePy(new TopoShape(s))));
    }
    return Py::new_reference_to(shapes);
}

PyObject* BRepOffsetAPI_MakePipeShellPy::setTolerance(PyObject *args)
{
    double tol3d, boundTol, tolAngular;
    if (!PyArg_ParseTuple(args, "ddd",&tol3d,&boundTol,&tolAngular))
        return 0;
    this->getBRepOffsetAPI_MakePipeShellPtr()->SetTolerance(tol3d, boundTol, tolAngular);
    Py_Return;
}

PyObject* BRepOffsetAPI_MakePipeShellPy::setTransitionMode(PyObject *args)
{
    int mode;
    if (!PyArg_ParseTuple(args, "i",&mode))
        return 0;
    this->getBRepOffsetAPI_MakePipeShellPtr()->SetTransitionMode(BRepBuilderAPI_TransitionMode(mode));
    Py_Return;
}

PyObject *BRepOffsetAPI_MakePipeShellPy::getCustomAttributes(const char* attr) const
{
    return 0;
}

int BRepOffsetAPI_MakePipeShellPy::setCustomAttributes(const char* attr, PyObject *obj)
{
    return 0; 
}
