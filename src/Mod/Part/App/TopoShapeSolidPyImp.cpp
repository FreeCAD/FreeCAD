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

#include <Standard_Version.hxx>
#include <BRepGProp.hxx>
#include <BRepTools.hxx>
#if OCC_VERSION_HEX >= 0x060600
#include <BRepClass3d.hxx>
#endif
#include <GProp_GProps.hxx>
#include <GProp_PrincipalProps.hxx>
#include <BRepBuilderAPI_MakeSolid.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Solid.hxx>
#include <TopoDS_Shell.hxx>
#include <gp_Ax1.hxx>
#include <gp_Pnt.hxx>
#include <gp_Dir.hxx>
#include <Standard_Failure.hxx>

#include <Base/VectorPy.h>
#include <Base/GeometryPyCXX.h>

#include "TopoShape.h"
#include "Tools.h"

// inclusion of the generated files (generated out of TopoShapeSolidPy.xml)
#include "TopoShapeShellPy.h"
#include "TopoShapeSolidPy.h"
#include "TopoShapeSolidPy.cpp"

using namespace Part;

// returns a string which represents the object e.g. when printed in python
std::string TopoShapeSolidPy::representation(void) const
{
    std::stringstream str;
    str << "<Solid object at " << getTopoShapePtr() << ">";

    return str.str();
}

PyObject *TopoShapeSolidPy::PyMake(struct _typeobject *, PyObject *, PyObject *)
{
    // create a new instance of TopoShapeSolidPy and the Twin object 
    return new TopoShapeSolidPy(new TopoShape);
}

// constructor method
int TopoShapeSolidPy::PyInit(PyObject* args, PyObject* /*kwd*/)
{
    PyObject *obj;
    if (!PyArg_ParseTuple(args, "O!", &(TopoShapePy::Type), &obj))
        return -1;

    try {
        BRepBuilderAPI_MakeSolid mkSolid;
        const TopoDS_Shape& shape = static_cast<TopoShapePy*>(obj)
            ->getTopoShapePtr()->_Shape;
        TopExp_Explorer anExp (shape, TopAbs_SHELL);
        int count=0;
        for (; anExp.More(); anExp.Next()) {
            ++count;
            mkSolid.Add(TopoDS::Shell(anExp.Current()));
        }

        if (count == 0)
            Standard_Failure::Raise("No shells found in shape");

        const TopoDS_Solid& solid = mkSolid.Solid();
        getTopoShapePtr()->_Shape = solid;
    }
    catch (Standard_Failure) {
        PyErr_SetString(PyExc_Exception, "creation of solid failed");
        return -1;
    }

    return 0;
}

Py::Object TopoShapeSolidPy::getMass(void) const
{
    GProp_GProps props;
    BRepGProp::VolumeProperties(getTopoShapePtr()->_Shape, props);
    double c = props.Mass();
    return Py::Float(c);
}

Py::Object TopoShapeSolidPy::getCenterOfMass(void) const
{
    GProp_GProps props;
    BRepGProp::VolumeProperties(getTopoShapePtr()->_Shape, props);
    gp_Pnt c = props.CentreOfMass();
    return Py::Vector(Base::Vector3d(c.X(),c.Y(),c.Z()));
}

Py::Object TopoShapeSolidPy::getMatrixOfInertia(void) const
{
    GProp_GProps props;
    BRepGProp::VolumeProperties(getTopoShapePtr()->_Shape, props);
    gp_Mat m = props.MatrixOfInertia();
    Base::Matrix4D mat;
    for (int i=0; i<3; i++) {
        for (int j=0; j<3; j++) {
            mat[i][j] = m(i+1,j+1);
        }
    }
    return Py::Matrix(mat);
}

Py::Object TopoShapeSolidPy::getStaticMoments(void) const
{
    GProp_GProps props;
    BRepGProp::VolumeProperties(getTopoShapePtr()->_Shape, props);
    Standard_Real lx,ly,lz;
    props.StaticMoments(lx,ly,lz);
    Py::Tuple tuple(3);
    tuple.setItem(0, Py::Float(lx));
    tuple.setItem(1, Py::Float(ly));
    tuple.setItem(2, Py::Float(lz));
    return tuple;
}

Py::Dict TopoShapeSolidPy::getPrincipalProperties(void) const
{
    GProp_GProps props;
    BRepGProp::VolumeProperties(getTopoShapePtr()->_Shape, props);
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

Py::Object TopoShapeSolidPy::getOuterShell(void) const
{
    TopoDS_Shell shell;
    const TopoDS_Shape& shape = getTopoShapePtr()->_Shape;
    if (!shape.IsNull() && shape.ShapeType() == TopAbs_SOLID)
#if OCC_VERSION_HEX >= 0x060600
        shell = BRepClass3d::OuterShell(TopoDS::Solid(shape));
#else
        shell = BRepTools::OuterShell(TopoDS::Solid(shape));
#endif
    return Py::Object(new TopoShapeShellPy(new TopoShape(shell)),true);
}

PyObject* TopoShapeSolidPy::getMomentOfInertia(PyObject *args)
{
    PyObject *p,*d;
    if (!PyArg_ParseTuple(args, "O!O!",&Base::VectorPy::Type,&p
                                      ,&Base::VectorPy::Type,&d))
        return 0;
    Base::Vector3d pnt = Py::Vector(p,false).toVector();
    Base::Vector3d dir = Py::Vector(d,false).toVector();

    try {
        GProp_GProps props;
        BRepGProp::VolumeProperties(getTopoShapePtr()->_Shape, props);
        double r = props.MomentOfInertia(gp_Ax1(Base::convertTo<gp_Pnt>(pnt),
                                                Base::convertTo<gp_Dir>(dir)));
        return PyFloat_FromDouble(r);
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }
}

PyObject* TopoShapeSolidPy::getRadiusOfGyration(PyObject *args)
{
    PyObject *p,*d;
    if (!PyArg_ParseTuple(args, "O!O!",&Base::VectorPy::Type,&p
                                      ,&Base::VectorPy::Type,&d))
        return 0;
    Base::Vector3d pnt = Py::Vector(p,false).toVector();
    Base::Vector3d dir = Py::Vector(d,false).toVector();

    try {
        GProp_GProps props;
        BRepGProp::VolumeProperties(getTopoShapePtr()->_Shape, props);
        double r = props.RadiusOfGyration(gp_Ax1(Base::convertTo<gp_Pnt>(pnt),
                                                Base::convertTo<gp_Dir>(dir)));
        return PyFloat_FromDouble(r);
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }
}

PyObject *TopoShapeSolidPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int TopoShapeSolidPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0; 
}
