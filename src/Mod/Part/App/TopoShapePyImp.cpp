/***************************************************************************
 *   Copyright (c) J�rgen Riegel          (juergen.riegel@web.de) 2008     *
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
# include <sstream>
# include <BRepMesh.hxx>
# include <BRepBuilderAPI_Copy.hxx>
# include <BRepBuilderAPI_Sewing.hxx>
# include <BRepClass3d_SolidClassifier.hxx>
# include <BRepFilletAPI_MakeFillet.hxx>
# include <BRepFilletAPI_MakeChamfer.hxx>
# include <BRepOffsetAPI_MakePipe.hxx>
# include <BRepOffsetAPI_MakePipeShell.hxx>
# include <BRepTools.hxx>
# include <gp_Ax1.hxx>
# include <gp_Ax2.hxx>
# include <gp_Dir.hxx>
# include <gp_Pnt.hxx>
# include <gp_Trsf.hxx>
# include <TopExp_Explorer.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Iterator.hxx>
# include <TopTools_IndexedMapOfShape.hxx>
# include <TopLoc_Location.hxx>
# include <TopExp.hxx>
# include <Precision.hxx>
#endif

#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <BRepAlgo_NormalProjection.hxx>


#include <Base/GeometryPyCXX.h>
#include <Base/Matrix.h>
#include <Base/Rotation.h>
#include <Base/MatrixPy.h>
#include <Base/Vector3D.h>
#include <Base/VectorPy.h>
#include <CXX/Extensions.hxx>

#include "TopoShape.h"
#include "TopoShapePy.h"
#include "TopoShapePy.cpp"

#include "GeometryPy.h"
#include "TopoShapeFacePy.h"
#include "TopoShapeEdgePy.h"
#include "TopoShapeWirePy.h"
#include "TopoShapeVertexPy.h"
#include "TopoShapeSolidPy.h"
#include "TopoShapeShellPy.h"
#include "TopoShapeCompSolidPy.h"
#include "TopoShapeCompoundPy.h"

using namespace Part;

#ifndef M_PI
    #define M_PI    3.14159265358979323846 /* pi */
#endif

#ifndef M_PI_2
    #define M_PI_2  1.57079632679489661923 /* pi/2 */
#endif

namespace Py {
    typedef ExtensionObject<TopoShapePy> TopoShape;
    template<>
    bool TopoShape::accepts (PyObject *pyob) const
    {
        return (pyob && PyObject_TypeCheck(pyob, &(Part::TopoShapePy::Type)));
    }
}

// returns a string which represents the object e.g. when printed in python
std::string TopoShapePy::representation(void) const
{
    std::stringstream str;
    str << "<Shape object at " << getTopoShapePtr() << ">";

    return str.str();
}

PyObject *TopoShapePy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of TopoShapePy and the Twin object 
    return new TopoShapePy(new TopoShape);
}

int TopoShapePy::PyInit(PyObject* args, PyObject*)
{
    PyObject *pcObj=0;
    if (!PyArg_ParseTuple(args, "|O!", &(PyList_Type), &pcObj))
        return -1;

    if (pcObj) {
        TopoShape shape;
        try {
            Py::List list(pcObj);
            bool first = true;
            for (Py::List::iterator it = list.begin(); it != list.end(); ++it) {
                if (PyObject_TypeCheck((*it).ptr(), &(Part::GeometryPy::Type))) {
                    TopoDS_Shape sh = static_cast<GeometryPy*>((*it).ptr())->
                        getGeometryPtr()->toShape();
                    if (first) {
                        first = false;
                        shape._Shape = sh;
                    }
                    else {
                        shape._Shape = shape.fuse(sh);
                    }
                }
            }
        }
        catch (Standard_Failure) {
            Handle_Standard_Failure e = Standard_Failure::Caught();
            PyErr_SetString(PyExc_Exception, e->GetMessageString());
            return -1;
        }

        getTopoShapePtr()->_Shape = shape._Shape;
    }

    return 0;
}

PyObject* TopoShapePy::copy(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;

    const TopoDS_Shape& shape = this->getTopoShapePtr()->_Shape;
    PyTypeObject* type = this->GetType();
    PyObject* cpy = 0;
    // let the type object decide
    if (type->tp_new)
        cpy = type->tp_new(type, this, 0);
    if (!cpy) {
        PyErr_SetString(PyExc_TypeError, "failed to create copy of shape");
        return 0;
    }

    static_cast<TopoShapePy*>(cpy)->getTopoShapePtr()->_Shape = shape;
    return cpy;
}

PyObject* TopoShapePy::replaceShape(PyObject *args)
{
    PyObject *l;
    if (!PyArg_ParseTuple(args, "O!",&PyList_Type,&l))
        return NULL;

    try {
        Py::List list(l);
        std::vector< std::pair<TopoDS_Shape, TopoDS_Shape> > shapes;
        for (Py::List::iterator it = list.begin(); it != list.end(); ++it) {
            Py::Tuple tuple(*it);
            Py::TopoShape sh1(tuple[0]);
            Py::TopoShape sh2(tuple[1]);
            shapes.push_back(std::make_pair(
                sh1.extensionObject()->getTopoShapePtr()->_Shape,
                sh2.extensionObject()->getTopoShapePtr()->_Shape)
            );
        }
        PyTypeObject* type = this->GetType();
        PyObject* inst = type->tp_new(type, this, 0);
        static_cast<TopoShapePy*>(inst)->getTopoShapePtr()->_Shape = 
            this->getTopoShapePtr()->replaceShape(shapes);
        return inst;
    }
    catch (const Py::Exception&) {
        return 0;
    }
    catch (...) {
        PyErr_SetString(PyExc_Exception, "failed to replace shape");
        return 0;
    }
}

PyObject* TopoShapePy::removeShape(PyObject *args)
{
    PyObject *l;
    if (!PyArg_ParseTuple(args, "O!",&PyList_Type,&l))
        return NULL;

    try {
        Py::List list(l);
        std::vector<TopoDS_Shape> shapes;
        for (Py::List::iterator it = list.begin(); it != list.end(); ++it) {
            Py::Tuple tuple(*it);
            Py::TopoShape sh(tuple[0]);
            shapes.push_back(
                sh.extensionObject()->getTopoShapePtr()->_Shape
            );
        }
        PyTypeObject* type = this->GetType();
        PyObject* inst = type->tp_new(type, this, 0);
        static_cast<TopoShapePy*>(inst)->getTopoShapePtr()->_Shape = 
            this->getTopoShapePtr()->removeShape(shapes);
        return inst;
    }
    catch (...) {
        PyErr_SetString(PyExc_Exception, "failed to remove shape");
        return 0;
    }
}

PyObject*  TopoShapePy::read(PyObject *args)
{
    char* filename;
    if (!PyArg_ParseTuple(args, "s", &filename))
        return NULL;

    getTopoShapePtr()->read(filename);
    Py_Return;
}

PyObject* TopoShapePy::writeInventor(PyObject * args)
{
    double dev=0.3, angle=0.4;
    int mode=2;
    if (!PyArg_ParseTuple(args, "|idd", &mode,&dev,&angle))
        return NULL;

    std::stringstream result;
    BRepMesh::Mesh(getTopoShapePtr()->_Shape,dev);
    if (mode == 0)
        getTopoShapePtr()->exportFaceSet(dev, angle, result);
    else if (mode == 1)
        getTopoShapePtr()->exportLineSet(result);
    else {
        getTopoShapePtr()->exportFaceSet(dev, angle, result);
        getTopoShapePtr()->exportLineSet(result);
    }
    // NOTE: Cleaning the triangulation may cause problems on some algorithms like BOP
    //BRepTools::Clean(getTopoShapePtr()->_Shape); // remove triangulation
    return Py::new_reference_to(Py::String(result.str()));
}

PyObject*  TopoShapePy::exportIges(PyObject *args)
{
    char* filename;
    if (!PyArg_ParseTuple(args, "s", &filename))
        return NULL;

    try {
        // write iges file
        getTopoShapePtr()->exportIges(filename);
    }
    catch (const Base::Exception& e) {
        PyErr_SetString(PyExc_Exception,e.what());
        return NULL;
    }

    Py_Return;
}

PyObject*  TopoShapePy::exportStep(PyObject *args)
{
    char* filename;
    if (!PyArg_ParseTuple(args, "s", &filename))
        return NULL;

    try {
        // write step file
        getTopoShapePtr()->exportStep(filename);
    }
    catch (const Base::Exception& e) {
        PyErr_SetString(PyExc_Exception,e.what());
        return NULL;
    }

    Py_Return;
}

PyObject*  TopoShapePy::exportBrep(PyObject *args)
{
    char* filename;
    if (!PyArg_ParseTuple(args, "s", &filename))
        return NULL;

    try {
        // write brep file
        getTopoShapePtr()->exportBrep(filename);
    }
    catch (const Base::Exception& e) {
        PyErr_SetString(PyExc_Exception,e.what());
        return NULL;
    }

    Py_Return;
}

PyObject*  TopoShapePy::exportStl(PyObject *args)
{
    char* filename;
    if (!PyArg_ParseTuple(args, "s", &filename))
        return NULL;

    try {
        // write stl file
        getTopoShapePtr()->exportStl(filename);
    }
    catch (const Base::Exception& e) {
        PyErr_SetString(PyExc_Exception,e.what());
        return NULL;
    }

    Py_Return;
}

PyObject* TopoShapePy::extrude(PyObject *args)
{
    PyObject *pVec;
    if (PyArg_ParseTuple(args, "O!", &(Base::VectorPy::Type), &pVec)) {
        try {
            Base::Vector3d vec = static_cast<Base::VectorPy*>(pVec)->value();
            TopoDS_Shape shape = this->getTopoShapePtr()->makePrism(gp_Vec(vec.x,vec.y,vec.z));
            TopAbs_ShapeEnum type = shape.ShapeType();
            switch (type)
            {
            case TopAbs_COMPOUND:
                return new TopoShapeCompoundPy(new TopoShape(shape));
            case TopAbs_COMPSOLID:
                return new TopoShapeCompSolidPy(new TopoShape(shape));
            case TopAbs_SOLID:
                return new TopoShapeSolidPy(new TopoShape(shape));
            case TopAbs_SHELL:
                return new TopoShapeShellPy(new TopoShape(shape));
            case TopAbs_FACE:
                return new TopoShapeFacePy(new TopoShape(shape));
            case TopAbs_WIRE:
                break;
            case TopAbs_EDGE:
                return new TopoShapeEdgePy(new TopoShape(shape));
            case TopAbs_VERTEX:
                break;
            case TopAbs_SHAPE:
                break;
            default:
                break;
            }

            PyErr_SetString(PyExc_Exception, "extrusion for this shape type not supported");
            return 0;
        }
        catch (Standard_Failure) {
            Handle_Standard_Failure e = Standard_Failure::Caught();
            PyErr_SetString(PyExc_Exception, e->GetMessageString());
            return 0;
        }
    }

    return 0;
}

PyObject* TopoShapePy::revolve(PyObject *args)
{
    PyObject *pPos,*pDir;
    double d=360;
    if (PyArg_ParseTuple(args, "O!O!|d", &(Base::VectorPy::Type), &pPos, &(Base::VectorPy::Type), &pDir,&d)) {
        try {
            const TopoDS_Shape& input = this->getTopoShapePtr()->_Shape;
            if (input.IsNull()) {
                PyErr_SetString(PyExc_Exception, "empty shape cannot be revolved");
                return 0;
            }

            TopExp_Explorer xp;
            xp.Init(input,TopAbs_SOLID);
            if (xp.More()) {
                PyErr_SetString(PyExc_Exception, "shape must not contain solids");
                return 0;
            }
            xp.Init(input,TopAbs_COMPSOLID);
            if (xp.More()) {
                PyErr_SetString(PyExc_Exception, "shape must not contain compound solids");
                return 0;
            }

            Base::Vector3d pos = static_cast<Base::VectorPy*>(pPos)->value();
            Base::Vector3d dir = static_cast<Base::VectorPy*>(pDir)->value();
            TopoDS_Shape shape = this->getTopoShapePtr()->revolve(
                gp_Ax1(gp_Pnt(pos.x,pos.y,pos.z), gp_Dir(dir.x,dir.y,dir.z)),d*(M_PI/180));
            TopAbs_ShapeEnum type = shape.ShapeType();
            switch (type)
            {
            case TopAbs_COMPOUND:
                return new TopoShapeCompoundPy(new TopoShape(shape));
            case TopAbs_COMPSOLID:
                return new TopoShapeCompSolidPy(new TopoShape(shape));
            case TopAbs_SOLID:
                return new TopoShapeSolidPy(new TopoShape(shape));
            case TopAbs_SHELL:
                return new TopoShapeShellPy(new TopoShape(shape));
            case TopAbs_FACE:
                return new TopoShapeFacePy(new TopoShape(shape));
            case TopAbs_WIRE:
                break;
            case TopAbs_EDGE:
                return new TopoShapeEdgePy(new TopoShape(shape));
            case TopAbs_VERTEX:
                break;
            case TopAbs_SHAPE:
                break;
            default:
                break;
            }

            PyErr_SetString(PyExc_Exception, "revolution for this shape type not supported");
            return 0;
        }
        catch (Standard_Failure) {
            Handle_Standard_Failure e = Standard_Failure::Caught();
            PyErr_SetString(PyExc_Exception, e->GetMessageString());
            return 0;
        }
    }

    return 0;
}

PyObject*  TopoShapePy::check(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    if (!getTopoShapePtr()->_Shape.IsNull()) {
        std::stringstream str;
        if (!getTopoShapePtr()->analyze(str)) {
            PyErr_SetString(PyExc_StandardError, str.str().c_str());
            PyErr_Print();
        }
    }

    Py_Return; 
}

PyObject*  TopoShapePy::fuse(PyObject *args)
{
    PyObject *pcObj;
    if (!PyArg_ParseTuple(args, "O!", &(TopoShapePy::Type), &pcObj))
        return NULL;

    TopoDS_Shape shape = static_cast<TopoShapePy*>(pcObj)->getTopoShapePtr()->_Shape;
    try {
        // Let's call algorithm computing a fuse operation:
        TopoDS_Shape fusShape = this->getTopoShapePtr()->fuse(shape);
        return new TopoShapePy(new TopoShape(fusShape));
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return NULL;
    }
    catch (const std::exception& e) {
        PyErr_SetString(PyExc_Exception, e.what());
        return NULL;
    }
}

PyObject*  TopoShapePy::oldFuse(PyObject *args)
{
    PyObject *pcObj;
    if (!PyArg_ParseTuple(args, "O!", &(TopoShapePy::Type), &pcObj))
        return NULL;

    TopoDS_Shape shape = static_cast<TopoShapePy*>(pcObj)->getTopoShapePtr()->_Shape;
    try {
        // Let's call algorithm computing a fuse operation:
        TopoDS_Shape fusShape = this->getTopoShapePtr()->oldFuse(shape);
        return new TopoShapePy(new TopoShape(fusShape));
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return NULL;
    }
    catch (const std::exception& e) {
        PyErr_SetString(PyExc_Exception, e.what());
        return NULL;
    }
}

PyObject*  TopoShapePy::common(PyObject *args)
{
    PyObject *pcObj;
    if (!PyArg_ParseTuple(args, "O!", &(TopoShapePy::Type), &pcObj))
        return NULL;

    TopoDS_Shape shape = static_cast<TopoShapePy*>(pcObj)->getTopoShapePtr()->_Shape;
    try {
        // Let's call algorithm computing a common operation:
        TopoDS_Shape comShape = this->getTopoShapePtr()->common(shape);
        return new TopoShapePy(new TopoShape(comShape));
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return NULL;
    }
    catch (const std::exception& e) {
        PyErr_SetString(PyExc_Exception, e.what());
        return NULL;
    }
}

PyObject*  TopoShapePy::section(PyObject *args)
{
    PyObject *pcObj;
    if (!PyArg_ParseTuple(args, "O!", &(TopoShapePy::Type), &pcObj))
        return NULL;

    TopoDS_Shape shape = static_cast<TopoShapePy*>(pcObj)->getTopoShapePtr()->_Shape;
    try {
        // Let's call algorithm computing a section operation:
        TopoDS_Shape secShape = this->getTopoShapePtr()->section(shape);
        return new TopoShapePy(new TopoShape(secShape));
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return NULL;
    }
    catch (const std::exception& e) {
        PyErr_SetString(PyExc_Exception, e.what());
        return NULL;
    }
}

PyObject*  TopoShapePy::slice(PyObject *args)
{
    PyObject *dir;
    double d;
    if (!PyArg_ParseTuple(args, "O!d", &(Base::VectorPy::Type), &dir, &d))
        return NULL;

    try {
        Base::Vector3d vec = Py::Vector(dir, false).toVector();
        std::list<TopoDS_Wire> slice = this->getTopoShapePtr()->slice(vec, d);
        Py::List wire;
        for (std::list<TopoDS_Wire>::iterator it = slice.begin(); it != slice.end(); ++it) {
            wire.append(Py::asObject(new TopoShapeWirePy(new TopoShape(*it))));
        }

        return Py::new_reference_to(wire);
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return NULL;
    }
    catch (const std::exception& e) {
        PyErr_SetString(PyExc_Exception, e.what());
        return NULL;
    }
}

PyObject*  TopoShapePy::slices(PyObject *args)
{
    PyObject *dir, *dist;
    if (!PyArg_ParseTuple(args, "O!O!", &(Base::VectorPy::Type), &dir,
                                        &PyList_Type, &dist))
        return NULL;

    try {
        Base::Vector3d vec = Py::Vector(dir, false).toVector();
        Py::List list(dist);
        std::vector<double> d;
        d.reserve(list.size());
        for (Py::List::iterator it = list.begin(); it != list.end(); ++it)
            d.push_back((double)Py::Float(*it));
        TopoDS_Compound slice = this->getTopoShapePtr()->slices(vec, d);
        return new TopoShapeCompoundPy(new TopoShape(slice));
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return NULL;
    }
    catch (const std::exception& e) {
        PyErr_SetString(PyExc_Exception, e.what());
        return NULL;
    }
}

PyObject*  TopoShapePy::cut(PyObject *args)
{
    PyObject *pcObj;
    if (!PyArg_ParseTuple(args, "O!", &(TopoShapePy::Type), &pcObj))
        return NULL;

    TopoDS_Shape shape = static_cast<TopoShapePy*>(pcObj)->getTopoShapePtr()->_Shape;
    try {
        // Let's call algorithm computing a cut operation:
        TopoDS_Shape cutShape = this->getTopoShapePtr()->cut(shape);
        return new TopoShapePy(new TopoShape(cutShape));
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return NULL;
    }
    catch (const std::exception& e) {
        PyErr_SetString(PyExc_Exception, e.what());
        return NULL;
    }
}

PyObject*  TopoShapePy::sewShape(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;

    try {
        getTopoShapePtr()->sewShape();
        Py_Return;
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return NULL;
    }
}

PyObject*  TopoShapePy::removeInternalWires(PyObject *args)
{
    double minArea;
    if (!PyArg_ParseTuple(args, "d",&minArea))
        return NULL;

    try {
        bool ok = getTopoShapePtr()->removeInternalWires(minArea);
        PyObject* ret = ok ? Py_True : Py_False;
        Py_INCREF(ret);
        return ret;
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return NULL;
    }
}

PyObject*  TopoShapePy::mirror(PyObject *args)
{
    PyObject *v1, *v2;
    if (!PyArg_ParseTuple(args, "O!O!", &(Base::VectorPy::Type),&v1,
                                        &(Base::VectorPy::Type),&v2))
        return NULL;

    Base::Vector3d base = Py::Vector(v1,false).toVector();
    Base::Vector3d norm = Py::Vector(v2,false).toVector();

    try {
        gp_Ax2 ax2(gp_Pnt(base.x,base.y,base.z), gp_Dir(norm.x,norm.y,norm.z));
        TopoDS_Shape shape = this->getTopoShapePtr()->mirror(ax2);
        return new TopoShapePy(new TopoShape(shape));
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return NULL;
    }
}

PyObject*  TopoShapePy::transformGeometry(PyObject *args)
{
    PyObject *obj;
    if (!PyArg_ParseTuple(args, "O!", &(Base::MatrixPy::Type),&obj))
        return NULL;

    Base::Matrix4D mat = static_cast<Base::MatrixPy*>(obj)->value();
    try {
        TopoDS_Shape shape = this->getTopoShapePtr()->transformGShape(mat);
        return new TopoShapePy(new TopoShape(shape));
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return NULL;
    }
}

PyObject*  TopoShapePy::transformShape(PyObject *args)
{
    PyObject *obj;
    if (!PyArg_ParseTuple(args, "O!", &(Base::MatrixPy::Type),&obj))
        return NULL;

    Base::Matrix4D mat = static_cast<Base::MatrixPy*>(obj)->value();
    try {
        this->getTopoShapePtr()->transformShape(mat);
        Py_Return;
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return NULL;
    }
}

PyObject*  TopoShapePy::translate(PyObject *args)
{
    PyObject *obj;
    if (!PyArg_ParseTuple(args, "O", &obj))
        return 0;

    Base::Vector3d vec;
    if (PyObject_TypeCheck(obj, &(Base::VectorPy::Type))) {
        vec = static_cast<Base::VectorPy*>(obj)->value();
    }
    else if (PyObject_TypeCheck(obj, &PyTuple_Type)) {
        vec = Base::getVectorFromTuple<double>(obj);
    }
    else {
        PyErr_SetString(PyExc_TypeError, "either vector or tuple expected");
        return 0;
    }

    gp_Trsf mov;
    mov.SetTranslation(gp_Vec(vec.x,vec.y,vec.z));
    TopLoc_Location loc(mov);
    getTopoShapePtr()->_Shape.Move(loc);
    Py_Return;
}

PyObject*  TopoShapePy::rotate(PyObject *args)
{
    PyObject *obj1, *obj2;
    double angle;
    if (!PyArg_ParseTuple(args, "OOd", &obj1, &obj2, &angle))
        return NULL;

    try {
        // Vector also supports sequence
        Py::Sequence p1(obj1), p2(obj2);
        // Convert into OCC representation
        gp_Pnt pos = gp_Pnt((double)Py::Float(p1[0]),
                            (double)Py::Float(p1[1]),
                            (double)Py::Float(p1[2]));
        gp_Dir dir = gp_Dir((double)Py::Float(p2[0]),
                            (double)Py::Float(p2[1]),
                            (double)Py::Float(p2[2]));

        gp_Ax1 axis(pos, dir);
        gp_Trsf mov;
        mov.SetRotation(axis, angle*(M_PI/180));
        TopLoc_Location loc(mov);
        getTopoShapePtr()->_Shape.Move(loc);
        Py_Return;
    }
    catch (const Py::Exception&) {
        return NULL;
    }
}

PyObject*  TopoShapePy::scale(PyObject *args)
{
    double factor;
    PyObject* p=0;
    if (!PyArg_ParseTuple(args, "d|O!", &factor, &(Base::VectorPy::Type), &p))
        return NULL;

    gp_Pnt pos(0,0,0);
    if (p) {
        Base::Vector3d pnt = static_cast<Base::VectorPy*>(p)->value();
        pos.SetX(pnt.x);
        pos.SetY(pnt.y);
        pos.SetZ(pnt.z);
    }
    if (fabs(factor) < Precision::Confusion()) {
        PyErr_SetString(PyExc_Exception, "scale factor too small");
        return NULL;
    }

    try {
        gp_Trsf scl;
        scl.SetScale(pos, factor);
        TopLoc_Location loc(scl);
        getTopoShapePtr()->_Shape.Move(loc);
        Py_Return;
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return NULL;
    }
}

PyObject* TopoShapePy::makeFillet(PyObject *args)
{
    // use one radius for all edges
    double radius;
    PyObject *obj;
    if (PyArg_ParseTuple(args, "dO!", &radius, &(PyList_Type), &obj)) {
        try {
            const TopoDS_Shape& shape = this->getTopoShapePtr()->_Shape;
            BRepFilletAPI_MakeFillet mkFillet(shape);
            Py::List list(obj);
            for (Py::List::iterator it = list.begin(); it != list.end(); ++it) {
                if (PyObject_TypeCheck((*it).ptr(), &(Part::TopoShapePy::Type))) {
                    const TopoDS_Shape& edge = static_cast<TopoShapePy*>((*it).ptr())->getTopoShapePtr()->_Shape;
                    if (edge.ShapeType() == TopAbs_EDGE) {
                        //Add edge to fillet algorithm
                        mkFillet.Add(radius, TopoDS::Edge(edge));
                    }
                }
            }
            return new TopoShapePy(new TopoShape(mkFillet.Shape()));
        }
        catch (Standard_Failure) {
            Handle_Standard_Failure e = Standard_Failure::Caught();
            PyErr_SetString(PyExc_Exception, e->GetMessageString());
            return NULL;
        }
    }

    // use two radii for all edges
    PyErr_Clear();
    double radius1, radius2;
    if (PyArg_ParseTuple(args, "ddO!", &radius1, &radius2, &(PyList_Type), &obj)) {
        try {
            const TopoDS_Shape& shape = this->getTopoShapePtr()->_Shape;
            BRepFilletAPI_MakeFillet mkFillet(shape);
            Py::List list(obj);
            for (Py::List::iterator it = list.begin(); it != list.end(); ++it) {
                if (PyObject_TypeCheck((*it).ptr(), &(Part::TopoShapePy::Type))) {
                    const TopoDS_Shape& edge = static_cast<TopoShapePy*>((*it).ptr())->getTopoShapePtr()->_Shape;
                    if (edge.ShapeType() == TopAbs_EDGE) {
                        //Add edge to fillet algorithm
                        mkFillet.Add(radius1, radius2, TopoDS::Edge(edge));
                    }
                }
            }
            return new TopoShapePy(new TopoShape(mkFillet.Shape()));
        }
        catch (Standard_Failure) {
            Handle_Standard_Failure e = Standard_Failure::Caught();
            PyErr_SetString(PyExc_Exception, e->GetMessageString());
            return NULL;
        }
    }

    PyErr_SetString(PyExc_TypeError, "This method accepts:\n"
        "-- one radius and a list of edges\n"
        "-- two radii and a list of edges");
    return NULL;
}

PyObject* TopoShapePy::makeChamfer(PyObject *args)
{
    // use one radius for all edges
    double radius;
    PyObject *obj;
    if (PyArg_ParseTuple(args, "dO!", &radius, &(PyList_Type), &obj)) {
        try {
            const TopoDS_Shape& shape = this->getTopoShapePtr()->_Shape;
            BRepFilletAPI_MakeChamfer mkChamfer(shape);
            TopTools_IndexedMapOfShape mapOfEdges;
            TopTools_IndexedDataMapOfShapeListOfShape mapEdgeFace;
            TopExp::MapShapesAndAncestors(shape, TopAbs_EDGE, TopAbs_FACE, mapEdgeFace);
            TopExp::MapShapes(shape, TopAbs_EDGE, mapOfEdges);
            Py::List list(obj);
            for (Py::List::iterator it = list.begin(); it != list.end(); ++it) {
                if (PyObject_TypeCheck((*it).ptr(), &(Part::TopoShapePy::Type))) {
                    const TopoDS_Shape& edge = static_cast<TopoShapePy*>((*it).ptr())->getTopoShapePtr()->_Shape;
                    if (edge.ShapeType() == TopAbs_EDGE) {
                        //Add edge to fillet algorithm
                        const TopoDS_Face& face = TopoDS::Face(mapEdgeFace.FindFromKey(edge).First());
                        mkChamfer.Add(radius, TopoDS::Edge(edge), face);
                    }
                }
            }
            return new TopoShapePy(new TopoShape(mkChamfer.Shape()));
        }
        catch (Standard_Failure) {
            Handle_Standard_Failure e = Standard_Failure::Caught();
            PyErr_SetString(PyExc_Exception, e->GetMessageString());
            return NULL;
        }
    }

    // use two radii for all edges
    PyErr_Clear();
    double radius1, radius2;
    if (PyArg_ParseTuple(args, "ddO!", &radius1, &radius2, &(PyList_Type), &obj)) {
        try {
            const TopoDS_Shape& shape = this->getTopoShapePtr()->_Shape;
            BRepFilletAPI_MakeChamfer mkChamfer(shape);
            TopTools_IndexedMapOfShape mapOfEdges;
            TopTools_IndexedDataMapOfShapeListOfShape mapEdgeFace;
            TopExp::MapShapesAndAncestors(shape, TopAbs_EDGE, TopAbs_FACE, mapEdgeFace);
            TopExp::MapShapes(shape, TopAbs_EDGE, mapOfEdges);
            Py::List list(obj);
            for (Py::List::iterator it = list.begin(); it != list.end(); ++it) {
                if (PyObject_TypeCheck((*it).ptr(), &(Part::TopoShapePy::Type))) {
                    const TopoDS_Shape& edge = static_cast<TopoShapePy*>((*it).ptr())->getTopoShapePtr()->_Shape;
                    if (edge.ShapeType() == TopAbs_EDGE) {
                        //Add edge to fillet algorithm
                        const TopoDS_Face& face = TopoDS::Face(mapEdgeFace.FindFromKey(edge).First());
                        mkChamfer.Add(radius1, radius2, TopoDS::Edge(edge), face);
                    }
                }
            }
            return new TopoShapePy(new TopoShape(mkChamfer.Shape()));
        }
        catch (Standard_Failure) {
            Handle_Standard_Failure e = Standard_Failure::Caught();
            PyErr_SetString(PyExc_Exception, e->GetMessageString());
            return NULL;
        }
    }

    PyErr_SetString(PyExc_TypeError, "This method accepts:\n"
        "-- one radius and a list of edges\n"
        "-- two radii and a list of edges");
    return NULL;
}

PyObject* TopoShapePy::makeThickness(PyObject *args)
{
    PyObject *obj;
    double offset, tolerance;
    if (!PyArg_ParseTuple(args, "O!dd", &(PyList_Type), &obj, &offset, &tolerance))
        return 0;

    try {
        TopTools_ListOfShape facesToRemove;
        Py::List list(obj);
        for (Py::List::iterator it = list.begin(); it != list.end(); ++it) {
            if (PyObject_TypeCheck((*it).ptr(), &(Part::TopoShapePy::Type))) {
                const TopoDS_Shape& shape = static_cast<TopoShapePy*>((*it).ptr())->getTopoShapePtr()->_Shape;
                facesToRemove.Append(shape);
            }
        }

        TopoDS_Shape shape = this->getTopoShapePtr()->makeThickSolid(facesToRemove, offset, tolerance);
        return new TopoShapeSolidPy(new TopoShape(shape));
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return NULL;
    }
}

PyObject* TopoShapePy::makeOffsetShape(PyObject *args)
{
    double offset, tolerance;
    PyObject* inter = Py_False;
    PyObject* self_inter = Py_False;
    short offsetMode = 0, join = 0;
    if (!PyArg_ParseTuple(args, "dd|O!O!hh",
        &offset, &tolerance,
        &(PyBool_Type), &inter,
        &(PyBool_Type), &self_inter,
        &offsetMode, &join))
        return 0;

    try {
        TopoDS_Shape shape = this->getTopoShapePtr()->makeOffset(offset, tolerance,
            (inter == Py_True), (self_inter == Py_True), offsetMode, join);
        return new TopoShapePy(new TopoShape(shape));
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return NULL;
    }
}

PyObject*  TopoShapePy::reverse(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;

    getTopoShapePtr()->_Shape.Reverse();
    Py_Return;
}

PyObject*  TopoShapePy::complement(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;

    getTopoShapePtr()->_Shape.Complement();
    Py_Return;
}

PyObject*  TopoShapePy::nullify(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;

    getTopoShapePtr()->_Shape.Nullify();
    Py_Return;
}

PyObject*  TopoShapePy::isNull(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    bool null = getTopoShapePtr()->isNull();
    return Py_BuildValue("O", (null ? Py_True : Py_False));
}

PyObject*  TopoShapePy::isClosed(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    try {
        if (getTopoShapePtr()->_Shape.IsNull())
            Standard_Failure::Raise("Cannot determine the 'Closed'' flag of an empty shape");
        return Py_BuildValue("O", (getTopoShapePtr()->isClosed() ? Py_True : Py_False));
    }
    catch (...) {
        PyErr_SetString(PyExc_RuntimeError, "check failed, shape may be empty");
        return NULL;
    }
}

PyObject*  TopoShapePy::isEqual(PyObject *args)
{
    PyObject *pcObj;
    if (!PyArg_ParseTuple(args, "O!", &(TopoShapePy::Type), &pcObj))
        return NULL;

    TopoDS_Shape shape = static_cast<TopoShapePy*>(pcObj)->getTopoShapePtr()->_Shape;
    Standard_Boolean test = (getTopoShapePtr()->_Shape == shape);
    return Py_BuildValue("O", (test ? Py_True : Py_False));
}

PyObject*  TopoShapePy::isSame(PyObject *args)
{
    PyObject *pcObj;
    if (!PyArg_ParseTuple(args, "O!", &(TopoShapePy::Type), &pcObj))
        return NULL;

    TopoDS_Shape shape = static_cast<TopoShapePy*>(pcObj)->getTopoShapePtr()->_Shape;
    Standard_Boolean test = getTopoShapePtr()->_Shape.IsSame(shape);
    return Py_BuildValue("O", (test ? Py_True : Py_False));
}

PyObject*  TopoShapePy::isValid(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;
    try {
        return Py_BuildValue("O", (getTopoShapePtr()->isValid() ? Py_True : Py_False));
    }
    catch (...) {
        PyErr_SetString(PyExc_RuntimeError, "check failed, shape may be empty");
        return NULL;
    }
}

PyObject*  TopoShapePy::fix(PyObject *args)
{
    double prec, mintol, maxtol;
    if (!PyArg_ParseTuple(args, "ddd", &prec, &mintol, &maxtol))
        return NULL;
    try {
        return Py_BuildValue("O", (getTopoShapePtr()->fix(prec, mintol, maxtol) ? Py_True : Py_False));
    }
    catch (...) {
        PyErr_SetString(PyExc_RuntimeError, "check failed, shape may be empty");
        return NULL;
    }
}

PyObject* TopoShapePy::hashCode(PyObject *args)
{
    int upper = IntegerLast();
    if (!PyArg_ParseTuple(args, "|i",&upper))
        return 0;
    int hc = getTopoShapePtr()->_Shape.HashCode(upper);
    return Py_BuildValue("i", hc);
}

PyObject* TopoShapePy::tessellate(PyObject *args)
{
    try {
        float tolerance;
        if (!PyArg_ParseTuple(args, "f",&tolerance))
            return 0;
        std::vector<Base::Vector3d> Points;
        std::vector<Data::ComplexGeoData::Facet> Facets;
        getTopoShapePtr()->getFaces(Points, Facets,tolerance);
        Py::Tuple tuple(2);
        Py::List vertex;
        for (std::vector<Base::Vector3d>::const_iterator it = Points.begin();
            it != Points.end(); ++it)
            vertex.append(Py::Object(new Base::VectorPy(*it)));
        tuple.setItem(0, vertex);
        Py::List facet;
        for (std::vector<Data::ComplexGeoData::Facet>::const_iterator
            it = Facets.begin(); it != Facets.end(); ++it) {
            Py::Tuple f(3);
            f.setItem(0,Py::Int((int)it->I1));
            f.setItem(1,Py::Int((int)it->I2));
            f.setItem(2,Py::Int((int)it->I3));
            facet.append(f);
        }
        tuple.setItem(1, facet);
        return Py::new_reference_to(tuple);
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return NULL;
    }
}

PyObject* TopoShapePy::project(PyObject *args)
{
    PyObject *obj;

    BRepAlgo_NormalProjection algo;
    algo.Init(this->getTopoShapePtr()->_Shape);
    if (PyArg_ParseTuple(args, "O!", &(PyList_Type), &obj)) {
        try {
            Py::List list(obj);
            for (Py::List::iterator it = list.begin(); it != list.end(); ++it) {
                if (PyObject_TypeCheck((*it).ptr(), &(Part::TopoShapePy::Type))) {
                    const TopoDS_Shape& shape = static_cast<TopoShapePy*>((*it).ptr())->getTopoShapePtr()->_Shape;
                    algo.Add(shape);
                }
            }

            algo.Compute3d(Standard_True);
            algo.SetLimit(Standard_True);
            algo.SetParams(1.e-6, 1.e-6, GeomAbs_C1, 14, 16);
            //algo.SetDefaultParams();
            algo.Build();
            return new TopoShapePy(new TopoShape(algo.Projection()));
        }
        catch (Standard_Failure) {
            PyErr_SetString(PyExc_Exception, "Failed to project shape");
            return NULL;
        }
    }

    return 0;
}

PyObject* TopoShapePy::makeShapeFromMesh(PyObject *args)
{
    PyObject *tup;
    float tolerance;
    if (!PyArg_ParseTuple(args, "O!f",&PyTuple_Type, &tup, &tolerance))
        return 0;

    try {
        Py::Tuple tuple(tup);
        Py::List vertex(tuple[0]);
        Py::List facets(tuple[1]);

        std::vector<Base::Vector3d> Points;
        for (Py::List::iterator it = vertex.begin(); it != vertex.end(); ++it) {
            Py::Vector vec(*it);
            Points.push_back(vec.toVector());
        }
        std::vector<Data::ComplexGeoData::Facet> Facets;
        for (Py::List::iterator it = facets.begin(); it != facets.end(); ++it) {
            Data::ComplexGeoData::Facet face;
            Py::Tuple f(*it);
            face.I1 = (int)Py::Int(f[0]);
            face.I2 = (int)Py::Int(f[1]);
            face.I3 = (int)Py::Int(f[2]);
            Facets.push_back(face);
        }

        getTopoShapePtr()->setFaces(Points, Facets,tolerance);
        Py_Return;
    }
    catch (const Py::Exception&) {
        return 0;
    }
}

PyObject* TopoShapePy::toNurbs(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;

    try {
        // Convert into nurbs
        TopoDS_Shape nurbs = this->getTopoShapePtr()->toNurbs();
        return new TopoShapePy(new TopoShape(nurbs));
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return NULL;
    }
}

PyObject*  TopoShapePy::isInside(PyObject *args)
{
    PyObject *point;
    double tolerance;
    PyObject* checkFace = Py_False;
    TopAbs_State stateIn = TopAbs_IN;
    if (!PyArg_ParseTuple(args, "O!dO!", &(Base::VectorPy::Type), &point, &tolerance,  &PyBool_Type, &checkFace))
        return NULL;
    try {
        TopoDS_Shape shape = getTopoShapePtr()->_Shape;
        BRepClass3d_SolidClassifier solidClassifier(shape);
        Base::Vector3d pnt = static_cast<Base::VectorPy*>(point)->value();
        gp_Pnt vertex = gp_Pnt(pnt.x,pnt.y,pnt.z);
        solidClassifier.Perform(vertex, tolerance);
        Standard_Boolean test = (solidClassifier.State() == stateIn);
        if ( (checkFace == Py_True) && (solidClassifier.IsOnAFace()) )
            test = Standard_True;
        return Py_BuildValue("O", (test ? Py_True : Py_False));
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return NULL;
    }
    catch (const std::exception& e) {
        PyErr_SetString(PyExc_Exception, e.what());
        return NULL;
    }
}

PyObject* TopoShapePy::removeSplitter(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return NULL;

    try {
        // Remove redundant splitter
        TopoDS_Shape shape = this->getTopoShapePtr()->removeSplitter();
        return new TopoShapePy(new TopoShape(shape));
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return NULL;
    }
}

#if 0 // see ComplexGeoDataPy::Matrix which does the same
Py::Object TopoShapePy::getLocation(void) const
{
    const TopLoc_Location& loc = getTopoShapePtr()->_Shape.Location();
    gp_Trsf trf = (gp_Trsf)loc;
    Base::Matrix4D mat;
    mat[0][0] = trf.Value(1,1);
    mat[0][1] = trf.Value(1,2);
    mat[0][2] = trf.Value(1,3);
    mat[0][3] = trf.Value(1,4);
    
    mat[1][0] = trf.Value(2,1);
    mat[1][1] = trf.Value(2,2);
    mat[1][2] = trf.Value(2,3);
    mat[1][3] = trf.Value(2,4);
    
    mat[2][0] = trf.Value(3,1);
    mat[2][1] = trf.Value(3,2);
    mat[2][2] = trf.Value(3,3);
    mat[2][3] = trf.Value(3,4);
    return Py::Object(new Base::MatrixPy(mat));
}

void TopoShapePy::setLocation(Py::Object o)
{
    PyObject* p = o.ptr();
    if (PyObject_TypeCheck(p, &(Base::MatrixPy::Type))) {
        Base::Matrix4D mat = static_cast<Base::MatrixPy*>(p)->value();
        Base::Rotation rot(mat);
        Base::Vector3d axis;
        double angle;
        rot.getValue(axis, angle);
        gp_Trsf trf;
        trf.SetRotation(gp_Ax1(gp_Pnt(), gp_Dir(axis.x, axis.y, axis.z)), angle);
        trf.SetTranslationPart(gp_Vec(mat[0][3],mat[1][3],mat[2][3]));
        TopLoc_Location loc(trf);
        getTopoShapePtr()->_Shape.Location(loc);
    }
    else {
        std::string error = std::string("type must be 'Matrix', not ");
        error += p->ob_type->tp_name;
        throw Py::TypeError(error);
    }
}
#endif

Py::String TopoShapePy::getShapeType(void) const
{
    TopoDS_Shape sh = getTopoShapePtr()->_Shape;
    if (sh.IsNull())
        throw Py::Exception(PyExc_Exception, "cannot determine type of null shape");
    TopAbs_ShapeEnum type = sh.ShapeType();
    std::string name;
    switch (type)
    {
    case TopAbs_COMPOUND:
        name = "Compound";
        break;
    case TopAbs_COMPSOLID:
        name = "CompSolid";
        break;
    case TopAbs_SOLID:
        name = "Solid";
        break;
    case TopAbs_SHELL:
        name = "Shell";
        break;
    case TopAbs_FACE:
        name = "Face";
        break;
    case TopAbs_WIRE:
        name = "Wire";
        break;
    case TopAbs_EDGE:
        name = "Edge";
        break;
    case TopAbs_VERTEX:
        name = "Vertex";
        break;
    case TopAbs_SHAPE:
        name = "Shape";
        break;
    }

    return Py::String(name);
}

Py::String TopoShapePy::getOrientation(void) const
{
    TopoDS_Shape sh = getTopoShapePtr()->_Shape;
    if (sh.IsNull())
        throw Py::Exception(PyExc_Exception, "cannot determine orientation of null shape");
    TopAbs_Orientation type = sh.Orientation();
    std::string name;
    switch (type)
    {
    case TopAbs_FORWARD:
        name = "Forward";
        break;
    case TopAbs_REVERSED:
        name = "Reversed";
        break;
    case TopAbs_INTERNAL:
        name = "Internal";
        break;
    case TopAbs_EXTERNAL:
        name = "External";
        break;
    }

    return Py::String(name);
}

void TopoShapePy::setOrientation(Py::String arg)
{
    TopoDS_Shape& sh = getTopoShapePtr()->_Shape;
    if (sh.IsNull())
        throw Py::Exception(PyExc_Exception, "cannot determine orientation of null shape");
    std::string name = (std::string)arg;
    TopAbs_Orientation type;
    if (name == "Forward") {
        type = TopAbs_FORWARD;
    }
    else if (name == "Reversed") {
        type = TopAbs_REVERSED;
    }
    else if (name == "Internal") {
        type = TopAbs_INTERNAL;
    }
    else if (name == "External") {
        type = TopAbs_EXTERNAL;
    }
    else {
        throw Py::AttributeError("Invalid orientation type");
    }

    sh.Orientation(type);
}

Py::List TopoShapePy::getFaces(void) const
{
    Py::List ret;
    TopTools_IndexedMapOfShape M;

    TopExp_Explorer Ex(getTopoShapePtr()->_Shape,TopAbs_FACE);
    while (Ex.More()) 
    {
        M.Add(Ex.Current());
        Ex.Next();
    }

    for (Standard_Integer k = 1; k <= M.Extent(); k++)
    {
        const TopoDS_Shape& shape = M(k);
        ret.append(Py::Object(new TopoShapeFacePy(new TopoShape(shape)),true));
    }

    return ret;
}

Py::List TopoShapePy::getVertexes(void) const
{
    Py::List ret;
    TopTools_IndexedMapOfShape M;

    TopExp_Explorer Ex(getTopoShapePtr()->_Shape,TopAbs_VERTEX);
    while (Ex.More()) 
    {
        M.Add(Ex.Current());
        Ex.Next();
    }

    for (Standard_Integer k = 1; k <= M.Extent(); k++)
    {
        const TopoDS_Shape& shape = M(k);
        ret.append(Py::Object(new TopoShapeVertexPy(new TopoShape(shape)),true));
    }

    return ret;
}

Py::List TopoShapePy::getShells(void) const
{
    Py::List ret;
    TopTools_IndexedMapOfShape M;

    TopExp_Explorer Ex(getTopoShapePtr()->_Shape,TopAbs_SHELL);
    while (Ex.More()) 
    {
        M.Add(Ex.Current());
        Ex.Next();
    }

    for (Standard_Integer k = 1; k <= M.Extent(); k++)
    {
        const TopoDS_Shape& shape = M(k);
        ret.append(Py::Object(new TopoShapeShellPy(new TopoShape(shape)),true));
    }

    return ret;
}

Py::List TopoShapePy::getSolids(void) const
{
    Py::List ret;
    TopTools_IndexedMapOfShape M;

    TopExp_Explorer Ex(getTopoShapePtr()->_Shape,TopAbs_SOLID);
    while (Ex.More()) 
    {
        M.Add(Ex.Current());
        Ex.Next();
    }

    for (Standard_Integer k = 1; k <= M.Extent(); k++)
    {
        const TopoDS_Shape& shape = M(k);
        ret.append(Py::Object(new TopoShapeSolidPy(new TopoShape(shape)),true));
    }

    return ret;
}

Py::List TopoShapePy::getCompSolids(void) const
{
    Py::List ret;
    TopTools_IndexedMapOfShape M;

    TopExp_Explorer Ex(getTopoShapePtr()->_Shape,TopAbs_COMPSOLID);
    while (Ex.More()) 
    {
        M.Add(Ex.Current());
        Ex.Next();
    }

    for (Standard_Integer k = 1; k <= M.Extent(); k++)
    {
        const TopoDS_Shape& shape = M(k);
        ret.append(Py::Object(new TopoShapeCompSolidPy(new TopoShape(shape)),true));
    }

    return ret;
}

Py::List TopoShapePy::getEdges(void) const
{
    Py::List ret;
    TopTools_IndexedMapOfShape M;

    TopExp_Explorer Ex(getTopoShapePtr()->_Shape,TopAbs_EDGE);
    while (Ex.More()) 
    {
        M.Add(Ex.Current());
        Ex.Next();
    }

    for (Standard_Integer k = 1; k <= M.Extent(); k++)
    {
        const TopoDS_Shape& shape = M(k);
        ret.append(Py::Object(new TopoShapeEdgePy(new TopoShape(shape)),true));
    }

    return ret;
}

Py::List TopoShapePy::getWires(void) const
{
    Py::List ret;
    TopTools_IndexedMapOfShape M;

    TopExp_Explorer Ex(getTopoShapePtr()->_Shape,TopAbs_WIRE);
    while (Ex.More())
    {
        M.Add(Ex.Current());
        Ex.Next();
    }

    for (Standard_Integer k = 1; k <= M.Extent(); k++)
    {
        const TopoDS_Shape& shape = M(k);
        ret.append(Py::Object(new TopoShapeWirePy(new TopoShape(shape)),true));
    }

    return ret;
}

Py::List TopoShapePy::getCompounds(void) const
{
    Py::List ret;
    TopTools_IndexedMapOfShape M;

    TopExp_Explorer Ex(getTopoShapePtr()->_Shape,TopAbs_COMPOUND);
    while (Ex.More())
    {
        M.Add(Ex.Current());
        Ex.Next();
    }

    for (Standard_Integer k = 1; k <= M.Extent(); k++)
    {
        const TopoDS_Shape& shape = M(k);
        ret.append(Py::Object(new TopoShapeCompoundPy(new TopoShape(shape)),true));
    }

    return ret;
}

Py::Float TopoShapePy::getLength(void) const
{
    GProp_GProps props;
    BRepGProp::LinearProperties(getTopoShapePtr()->_Shape, props);
    return Py::Float(props.Mass());
}

Py::Float TopoShapePy::getArea(void) const
{
    GProp_GProps props;
    BRepGProp::SurfaceProperties(getTopoShapePtr()->_Shape, props);
    return Py::Float(props.Mass());
}

Py::Float TopoShapePy::getVolume(void) const
{
    GProp_GProps props;
    BRepGProp::VolumeProperties(getTopoShapePtr()->_Shape, props);
    return Py::Float(props.Mass());
}

PyObject *TopoShapePy::getCustomAttributes(const char* attr) const
{
    if (!attr) return 0;
    std::string name(attr);
    try {
        if (name.size() > 4 && name.substr(0,4) == "Face" && name[4]>=48 && name[4]<=57) {
            std::auto_ptr<Part::ShapeSegment> s(static_cast<Part::ShapeSegment*>
                (getTopoShapePtr()->getSubElementByName(attr)));
            TopoDS_Shape Shape = s->Shape;
            return new TopoShapeFacePy(new TopoShape(Shape));
        }
        else if (name.size() > 4 && name.substr(0,4) == "Edge" && name[4]>=48 && name[4]<=57) {
            std::auto_ptr<Part::ShapeSegment> s(static_cast<Part::ShapeSegment*>
                (getTopoShapePtr()->getSubElementByName(attr)));
            TopoDS_Shape Shape = s->Shape;
            return new TopoShapeEdgePy(new TopoShape(Shape));
        }
        else if (name.size() > 6 && name.substr(0,6) == "Vertex" && name[6]>=48 && name[6]<=57) {
            std::auto_ptr<Part::ShapeSegment> s(static_cast<Part::ShapeSegment*>
                (getTopoShapePtr()->getSubElementByName(attr)));
            TopoDS_Shape Shape = s->Shape;
            return new TopoShapeVertexPy(new TopoShape(Shape));
        }
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        PyErr_SetString(PyExc_Exception, e->GetMessageString());
        return 0;
    }
    return 0;
}

int TopoShapePy::setCustomAttributes(const char* attr, PyObject *obj)
{
    return 0; 
}
