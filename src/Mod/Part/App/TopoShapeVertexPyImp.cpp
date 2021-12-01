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
# include <gp_Pnt.hxx>
# include <gp_Ax1.hxx>
# include <BRep_Tool.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Vertex.hxx>
# include <BRep_Builder.hxx>
# include <BRepBuilderAPI_MakeVertex.hxx>
# include <Geom_CartesianPoint.hxx>
# include <Standard_Failure.hxx>
#endif

#include <Mod/Part/App/TopoShape.h>
#include <Base/VectorPy.h>
#include <Base/Vector3D.h>

#include "PointPy.h"
#include "TopoShapeVertexPy.h"
#include "TopoShapeVertexPy.cpp"

using namespace Part;

// returns a string which represents the object e.g. when printed in python
std::string TopoShapeVertexPy::representation(void) const
{
    std::stringstream str;
    str << "<Vertex object at " << getTopoShapePtr() << ">";

    return str.str();
}

PyObject *TopoShapeVertexPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of TopoShapeVertexPy and the Twin object 
    return new TopoShapeVertexPy(new TopoShape);
}

// constructor method
int TopoShapeVertexPy::PyInit(PyObject* args, PyObject* /*kwd*/)
{
    if (PyArg_ParseTuple(args, "")) {
        // Undefined Vertex
        getTopoShapePtr()->setShape(TopoDS_Vertex());
        return 0;
    }

    PyErr_Clear();
    double x=0.0,y=0.0,z=0.0;
    PyObject *object;
    bool success = false;
    if (PyArg_ParseTuple(args, "|ddd", &x,&y,&z)) {
        // do nothing here
        success = true;
    }
    if (!success) {
        PyErr_Clear(); // set by PyArg_ParseTuple()
        if (PyArg_ParseTuple(args,"O!",&(Base::VectorPy::Type), &object)) {
            // Note: must be static_cast, not reinterpret_cast
            Base::Vector3d* ptr = static_cast<Base::VectorPy*>(object)->getVectorPtr();
            x = ptr->x;
            y = ptr->y;
            z = ptr->z;
            success = true;
        }
    }
    if (!success) {
        PyErr_Clear(); // set by PyArg_ParseTuple()
        if (PyArg_ParseTuple(args,"O!",&(PyTuple_Type), &object)) {
            try {
                Py::Tuple tuple(object);
                x = Py::Float(tuple.getItem(0));
                y = Py::Float(tuple.getItem(1));
                z = Py::Float(tuple.getItem(2));
                success = true;
            }
            catch (const Py::Exception&) {
                return -1;
            }
        }
    }
    if (!success) {
        PyErr_Clear(); // set by PyArg_ParseTuple()
        if (PyArg_ParseTuple(args,"O!",&(PointPy::Type), &object)) {
            Handle(Geom_CartesianPoint) this_point = Handle(Geom_CartesianPoint)::DownCast
                (static_cast<PointPy*>(object)->getGeomPointPtr()->handle());
            gp_Pnt pnt = this_point->Pnt();
            x = pnt.X();
            y = pnt.Y();
            z = pnt.Z();
            success = true;
        }
    }
    if (!success) {
        PyErr_Clear(); // set by PyArg_ParseTuple()
        if (PyArg_ParseTuple(args,"O!",&(Part::TopoShapePy::Type), &object)) {
            TopoShape* ptr = static_cast<TopoShapePy*>(object)->getTopoShapePtr();
            TopoDS_Shape shape = ptr->getShape();
            if (!shape.IsNull() && shape.ShapeType() == TopAbs_VERTEX) {
                TopoShapeVertexPy::PointerType vert = reinterpret_cast<TopoShapeVertexPy::PointerType>(_pcTwinPointer);
                vert->setShape(ptr->getShape());
                return 0;
            }
        }
    }
    if (!success) {
        PyErr_SetString(PyExc_TypeError, "Either three floats, tuple, vector or vertex expected");
        return -1;
    }

    TopoShapeVertexPy::PointerType ptr = reinterpret_cast<TopoShapeVertexPy::PointerType>(_pcTwinPointer);
    BRepBuilderAPI_MakeVertex aBuilder(gp_Pnt(x,y,z));
    TopoDS_Shape s = aBuilder.Vertex();
    ptr->setShape(s);

    return 0;
}

Py::Float TopoShapeVertexPy::getTolerance(void) const
{
    const TopoDS_Vertex& v = TopoDS::Vertex(getTopoShapePtr()->getShape());
    return Py::Float(BRep_Tool::Tolerance(v));
}

void TopoShapeVertexPy::setTolerance(Py::Float tol)
{
    BRep_Builder aBuilder;
    const TopoDS_Vertex& v = TopoDS::Vertex(getTopoShapePtr()->getShape());
    aBuilder.UpdateVertex(v, (double)tol);
}

Py::Float TopoShapeVertexPy::getX(void) const
{
    try {
        const TopoDS_Vertex& v = TopoDS::Vertex(getTopoShapePtr()->getShape());
        return Py::Float(BRep_Tool::Pnt(v).X());
    }
    catch (Standard_Failure& e) {

        throw Py::RuntimeError(e.GetMessageString());
    }
}

Py::Float TopoShapeVertexPy::getY(void) const
{
    try {
        const TopoDS_Vertex& v = TopoDS::Vertex(getTopoShapePtr()->getShape());
        return Py::Float(BRep_Tool::Pnt(v).Y());
    }
    catch (Standard_Failure& e) {

        throw Py::RuntimeError(e.GetMessageString());
    }
}

Py::Float TopoShapeVertexPy::getZ(void) const
{
    try {
        const TopoDS_Vertex& v = TopoDS::Vertex(getTopoShapePtr()->getShape());
        return Py::Float(BRep_Tool::Pnt(v).Z());
    }
    catch (Standard_Failure& e) {

        throw Py::RuntimeError(e.GetMessageString());
    }
}

Py::Object TopoShapeVertexPy::getPoint(void) const
{
    try {
        const TopoDS_Vertex& v = TopoDS::Vertex(getTopoShapePtr()->getShape());
        gp_Pnt p = BRep_Tool::Pnt(v);
        Base::PyObjectBase* pnt = new Base::VectorPy(new Base::Vector3d(p.X(),p.Y(),p.Z()));
        pnt->setNotTracking();
        return Py::asObject(pnt);
    }
    catch (Standard_Failure& e) {

        throw Py::RuntimeError(e.GetMessageString());
    }
}

PyObject *TopoShapeVertexPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int TopoShapeVertexPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0; 
}
