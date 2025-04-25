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
#ifndef _PreComp_
# include <BRep_Builder.hxx>
# include <BRepCheck_Analyzer.hxx>
# include <BRepGProp.hxx>
# include <BRepPrimAPI_MakeHalfSpace.hxx>
# include <GProp_GProps.hxx>
# include <GProp_PrincipalProps.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Shell.hxx>
# include <ShapeAnalysis_Shell.hxx>
# include <ShapeUpgrade_ShellSewing.hxx>
#endif

#include <Base/GeometryPyCXX.h>
#include <Base/VectorPy.h>

#include "OCCError.h"
#include "PartPyCXX.h"
#include "Tools.h"
#include "TopoShapeCompoundPy.h"
#include "TopoShapeCompoundPy.h"
#include "TopoShapeFacePy.h"
#include "TopoShapeShellPy.h"
#include "TopoShapeShellPy.cpp"
#include "TopoShapeSolidPy.h"
#include "TopoShapeOpCode.h"

using namespace Part;

// returns a string which represents the object e.g. when printed in python
std::string TopoShapeShellPy::representation() const
{
    // Note: As the return type is 'const char*' we cannot create a temporary
    // char array neither on the stack because the array would be freed when
    // leaving the scope nor on the heap because we would have a memory leak.
    // So we use a static array that is used by all instances of this class.
    // This, however, is not a problem as long as we only use this method in
    // _repr().

    std::stringstream str;
    str << "<Shell object at " << getTopoShapePtr() << ">";

    return str.str();
}

PyObject *TopoShapeShellPy::PyMake(struct _typeobject *, PyObject *, PyObject *)
{
    // create a new instance of TopoShapeSolidPy and the Twin object
    return new TopoShapeShellPy(new TopoShape);
}

// constructor method
int TopoShapeShellPy::PyInit(PyObject* args, PyObject* /*kwd*/)
{
    if (PyArg_ParseTuple(args, "")) {
        // Undefined Shell
        getTopoShapePtr()->setShape(TopoDS_Shell());
        return 0;
    }

    PyErr_Clear();
    PyObject *obj;
    if (!PyArg_ParseTuple(args, "O", &obj))
        return -1;

    try {
        getTopoShapePtr()->makeElementBoolean(Part::OpCodes::Shell, getPyShapes(obj));
    }
    _PY_CATCH_OCC(return (-1))
    return 0;
}

PyObject*  TopoShapeShellPy::add(PyObject *args)
{
    PyObject *obj;
    if (!PyArg_ParseTuple(args, "O!", &(Part::TopoShapeFacePy::Type), &obj))
        return nullptr;

    BRep_Builder builder;
    TopoDS_Shape shell = getTopoShapePtr()->getShape();
    if (shell.IsNull()) {
        builder.MakeShell(TopoDS::Shell(shell));
    }

    try {
        const TopoShape& shape = *static_cast<TopoShapeFacePy*>(obj)->getTopoShapePtr();
        const auto &sh = shape.getShape();
        if (!sh.IsNull()) {
            builder.Add(shell, sh);
            BRepCheck_Analyzer check(shell);
            getTopoShapePtr()->mapSubElement(shape);
            if (!check.IsValid()) {
                ShapeUpgrade_ShellSewing sewShell;
                getTopoShapePtr()->setShape(sewShell.ApplySewing(shell));
            }
        }
        else {
            Standard_Failure::Raise("cannot add empty shape");
        }
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }

    getTopoShapePtr()->setShape(shell);

    Py_Return;
}

PyObject*  TopoShapeShellPy::getFreeEdges(PyObject *args) const
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;
    ShapeAnalysis_Shell as;
    as.LoadShells(getTopoShapePtr()->getShape());
    as.CheckOrientedShells(getTopoShapePtr()->getShape(), Standard_True, Standard_True);

    TopoDS_Compound comp = as.FreeEdges();
    TopoShape res;
    res.setShape(comp);
    res.mapSubElement(*getTopoShapePtr());
    return Py::new_reference_to(shape2pyshape(res));
}

PyObject*  TopoShapeShellPy::getBadEdges(PyObject *args) const
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;
    ShapeAnalysis_Shell as;
    as.LoadShells(getTopoShapePtr()->getShape());
    as.CheckOrientedShells(getTopoShapePtr()->getShape(), Standard_True, Standard_True);

    TopoDS_Compound comp = as.BadEdges();
    TopoShape res;
    res.setShape(comp);
    res.mapSubElement(*getTopoShapePtr());
    return Py::new_reference_to(shape2pyshape(res));
}

PyObject* TopoShapeShellPy::makeHalfSpace(PyObject *args) const
{
    PyObject* pPnt;
    if (!PyArg_ParseTuple(args, "O!",&(Base::VectorPy::Type),&pPnt))
        return nullptr;

    try {
        Base::Vector3d pt = Py::Vector(pPnt,false).toVector();
        BRepPrimAPI_MakeHalfSpace mkHS(TopoDS::Shell(this->getTopoShapePtr()->getShape()), gp_Pnt(pt.x,pt.y,pt.z));
        return new TopoShapeSolidPy(new TopoShape(mkHS.Solid()));
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return nullptr;
    }
}

Py::Object TopoShapeShellPy::getMass() const
{
    GProp_GProps props;
    BRepGProp::SurfaceProperties(getTopoShapePtr()->getShape(), props);
    double c = props.Mass();
    return Py::Float(c);
}

Py::Object TopoShapeShellPy::getCenterOfMass() const
{
    GProp_GProps props;
    BRepGProp::SurfaceProperties(getTopoShapePtr()->getShape(), props);
    gp_Pnt c = props.CentreOfMass();
    return Py::Vector(Base::Vector3d(c.X(),c.Y(),c.Z()));
}

Py::Object TopoShapeShellPy::getMatrixOfInertia() const
{
    GProp_GProps props;
    BRepGProp::SurfaceProperties(getTopoShapePtr()->getShape(), props);
    gp_Mat m = props.MatrixOfInertia();
    Base::Matrix4D mat;
    for (int i=0; i<3; i++) {
        for (int j=0; j<3; j++) {
            mat[i][j] = m(i+1,j+1);
        }
    }
    return Py::Matrix(mat);
}

Py::Object TopoShapeShellPy::getStaticMoments() const
{
    GProp_GProps props;
    BRepGProp::SurfaceProperties(getTopoShapePtr()->getShape(), props);
    Standard_Real lx,ly,lz;
    props.StaticMoments(lx,ly,lz);
    Py::Tuple tuple(3);
    tuple.setItem(0, Py::Float(lx));
    tuple.setItem(1, Py::Float(ly));
    tuple.setItem(2, Py::Float(lz));
    return tuple;
}

Py::Dict TopoShapeShellPy::getPrincipalProperties() const
{
    GProp_GProps props;
    BRepGProp::SurfaceProperties(getTopoShapePtr()->getShape(), props);
    GProp_PrincipalProps pprops = props.PrincipalProperties();

    Py::Dict dict;
    dict.setItem("SymmetryAxis", Py::Boolean(pprops.HasSymmetryAxis() ? true : false));
    dict.setItem("SymmetryPoint", Py::Boolean(pprops.HasSymmetryPoint() ? true : false));
    Standard_Real lx,ly,lz;
    pprops.Moments(lx,ly,lz);
    Py::Tuple tuple(3);
    tuple.setItem(0, Py::Float(lx));
    tuple.setItem(1, Py::Float(ly));
    tuple.setItem(2, Py::Float(lz));
    dict.setItem("Moments",tuple);
    dict.setItem("FirstAxisOfInertia",Py::Vector(Base::convertTo
        <Base::Vector3d>(pprops.FirstAxisOfInertia())));
    dict.setItem("SecondAxisOfInertia",Py::Vector(Base::convertTo
        <Base::Vector3d>(pprops.SecondAxisOfInertia())));
    dict.setItem("ThirdAxisOfInertia",Py::Vector(Base::convertTo
        <Base::Vector3d>(pprops.ThirdAxisOfInertia())));

    Standard_Real Rxx,Ryy,Rzz;
    pprops.RadiusOfGyration(Rxx,Ryy,Rzz);
    Py::Tuple rog(3);
    rog.setItem(0, Py::Float(Rxx));
    rog.setItem(1, Py::Float(Ryy));
    rog.setItem(2, Py::Float(Rzz));
    dict.setItem("RadiusOfGyration",rog);
    return dict;
}

PyObject *TopoShapeShellPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int TopoShapeShellPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
