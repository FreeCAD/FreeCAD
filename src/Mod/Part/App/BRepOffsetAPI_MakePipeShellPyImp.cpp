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
# include <Standard_Version.hxx>
# include <TopTools_ListIteratorOfListOfShape.hxx>
#endif

#include "TopoShapePy.h"
#include "TopoShapeVertexPy.h"
#include "BRepOffsetAPI_MakePipeShellPy.h"
#include "BRepOffsetAPI_MakePipeShellPy.cpp"
#include "Tools.h"
#include "OCCError.h"
#include <Base/VectorPy.h>
#include <Base/GeometryPyCXX.h>

using namespace Part;

PyObject *BRepOffsetAPI_MakePipeShellPy::PyMake(struct _typeobject *, PyObject *args, PyObject *)  // Python wrapper
{
    // create a new instance of BRepOffsetAPI_MakePipeShellPy and the Twin object 
    PyObject* obj;
    if (!PyArg_ParseTuple(args, "O!",&(TopoShapePy::Type),&obj))
        return 0;
    const TopoDS_Shape& wire = static_cast<TopoShapePy*>(obj)->getTopoShapePtr()->getShape();
    if (!wire.IsNull() && wire.ShapeType() == TopAbs_WIRE) {
        return new BRepOffsetAPI_MakePipeShellPy(new BRepOffsetAPI_MakePipeShell(TopoDS::Wire(wire)));
    }

    PyErr_SetString(PartExceptionOCCError, "A valid wire is needed as argument");
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

    try {
        this->getBRepOffsetAPI_MakePipeShellPtr()->SetMode(PyObject_IsTrue(obj) ? Standard_True : Standard_False);
        Py_Return;
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return 0;
    }
}

PyObject* BRepOffsetAPI_MakePipeShellPy::setTrihedronMode(PyObject *args)
{
    PyObject *pnt, *dir;
    if (!PyArg_ParseTuple(args, "O!O!",&Base::VectorPy::Type,&pnt
                                      ,&Base::VectorPy::Type,&dir))
        return 0;

    try {
        gp_Pnt p = Base::convertTo<gp_Pnt>(Py::Vector(pnt,false).toVector());
        gp_Dir d = Base::convertTo<gp_Dir>(Py::Vector(dir,false).toVector());
        this->getBRepOffsetAPI_MakePipeShellPtr()->SetMode(gp_Ax2(p,d));
        Py_Return;
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return 0;
    }
}

PyObject* BRepOffsetAPI_MakePipeShellPy::setBiNormalMode(PyObject *args)
{
    PyObject *dir;
    if (!PyArg_ParseTuple(args, "O!",&Base::VectorPy::Type,&dir))
        return 0;

    try {
        gp_Dir d = Base::convertTo<gp_Dir>(Py::Vector(dir,false).toVector());
        this->getBRepOffsetAPI_MakePipeShellPtr()->SetMode(d);
        Py_Return;
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return 0;
    }
}

PyObject* BRepOffsetAPI_MakePipeShellPy::setSpineSupport(PyObject *args)
{
    PyObject *shape;
    if (!PyArg_ParseTuple(args, "O!",&Part::TopoShapePy::Type,&shape))
        return 0;

    try {
        const TopoDS_Shape& s = static_cast<Part::TopoShapePy*>(shape)->getTopoShapePtr()->getShape();
        Standard_Boolean ok = this->getBRepOffsetAPI_MakePipeShellPtr()->SetMode(s);
        return Py::new_reference_to(Py::Boolean(ok ? true : false));
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return 0;
    }
}

PyObject* BRepOffsetAPI_MakePipeShellPy::setAuxiliarySpine(PyObject *args)
{
#if OCC_VERSION_HEX >= 0x060700
    PyObject *spine, *curv, *keep;
    if (!PyArg_ParseTuple(args, "O!O!O!",&Part::TopoShapePy::Type,&spine
                                        ,&PyBool_Type,&curv
                                        ,&PyLong_Type,&keep))
        return 0;

    try {
        const TopoDS_Shape& s = static_cast<Part::TopoShapePy*>(spine)->getTopoShapePtr()->getShape();
        if (s.IsNull() || s.ShapeType() != TopAbs_WIRE) {
            PyErr_SetString(PyExc_TypeError, "spine is not a wire");
            return 0;
        }

        BRepFill_TypeOfContact typeOfCantact;
        switch (PyLong_AsLong(keep)) {
        case 1:
            typeOfCantact = BRepFill_Contact;
            break;
        case 2:
            typeOfCantact = BRepFill_ContactOnBorder;
            break;
        default:
            typeOfCantact = BRepFill_NoContact;
            break;
        }
        this->getBRepOffsetAPI_MakePipeShellPtr()->SetMode(
            TopoDS::Wire(s),
            PyObject_IsTrue(curv) ? Standard_True : Standard_False,
            typeOfCantact);
        Py_Return;
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return 0;
    }
#else
    PyObject *spine, *curv, *keep;
    if (!PyArg_ParseTuple(args, "O!O!O!",&Part::TopoShapePy::Type,&spine
                                        ,&PyBool_Type,&curv
                                        ,&PyBool_Type,&keep))
        return 0;

    try {
        const TopoDS_Shape& s = static_cast<Part::TopoShapePy*>(spine)->getTopoShapePtr()->getShape();
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
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return 0;
    }
#endif
}

PyObject* BRepOffsetAPI_MakePipeShellPy::add(PyObject *args, PyObject *kwds)
{
    PyObject *prof, *curv=Py_False, *keep=Py_False;
    static char* keywords_pro[] = {"Profile","WithContact","WithCorrection",NULL};
    if (PyArg_ParseTupleAndKeywords(args,kwds, "O!|O!O!", keywords_pro
                                        ,&Part::TopoShapePy::Type,&prof
                                        ,&PyBool_Type,&curv
                                        ,&PyBool_Type,&keep)) {
        try {
            const TopoDS_Shape& s = static_cast<Part::TopoShapePy*>(prof)->getTopoShapePtr()->getShape();
            this->getBRepOffsetAPI_MakePipeShellPtr()->Add(s,
                PyObject_IsTrue(curv) ? Standard_True : Standard_False,
                PyObject_IsTrue(keep) ? Standard_True : Standard_False);
            Py_Return;
        }
        catch (Standard_Failure& e) {
            PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
            return 0;
        }
    }

    PyErr_Clear();
    PyObject *loc;
    static char* keywords_loc[] = {"Profile","Location","WithContact","WithCorrection",NULL};
    if (PyArg_ParseTupleAndKeywords(args,kwds, "O!O!|O!O!", keywords_loc
                                        ,&Part::TopoShapePy::Type,&prof
                                        ,&Part::TopoShapeVertexPy::Type,&loc
                                        ,&PyBool_Type,&curv
                                        ,&PyBool_Type,&keep)) {
        try {
            const TopoDS_Shape& s = static_cast<Part::TopoShapePy*>(prof)->getTopoShapePtr()->getShape();
            const TopoDS_Vertex& v = TopoDS::Vertex(static_cast<Part::TopoShapePy*>(loc)->getTopoShapePtr()->getShape());
            this->getBRepOffsetAPI_MakePipeShellPtr()->Add(s, v,
                PyObject_IsTrue(curv) ? Standard_True : Standard_False,
                PyObject_IsTrue(keep) ? Standard_True : Standard_False);
            Py_Return;
        }
        catch (Standard_Failure& e) {
            PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
            return 0;
        }
    }

    PyErr_SetString(PyExc_TypeError, "Wrong arguments:\n"
                    "add(Profile, WithContact=False, WithCorrection=False)\n"
                    "add(Profile, Location, WithContact=False, WithCorrection=False)"
    );
    return 0;
}

PyObject* BRepOffsetAPI_MakePipeShellPy::remove(PyObject *args)
{
    PyObject *prof;
    if (!PyArg_ParseTuple(args, "O!",&Part::TopoShapePy::Type,&prof))
        return 0;

    try {
        const TopoDS_Shape& s = static_cast<Part::TopoShapePy*>(prof)->getTopoShapePtr()->getShape();
        this->getBRepOffsetAPI_MakePipeShellPtr()->Delete(s);
        Py_Return;
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return 0;
    }
}

PyObject* BRepOffsetAPI_MakePipeShellPy::isReady(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;

    try {
        Standard_Boolean ok = this->getBRepOffsetAPI_MakePipeShellPtr()->IsReady();
        return Py::new_reference_to(Py::Boolean(ok ? true : false));
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return 0;
    }
}

PyObject* BRepOffsetAPI_MakePipeShellPy::getStatus(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;

    try {
        Standard_Integer val = this->getBRepOffsetAPI_MakePipeShellPtr()->GetStatus();
        return Py::new_reference_to(Py::Long(val));
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return 0;
    }
}

PyObject* BRepOffsetAPI_MakePipeShellPy::makeSolid(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;

    try {
        Standard_Boolean ok = this->getBRepOffsetAPI_MakePipeShellPtr()->MakeSolid();
        return Py::new_reference_to(Py::Boolean(ok ? true : false));
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return 0;
    }
}

PyObject* BRepOffsetAPI_MakePipeShellPy::build(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;

    try {
        this->getBRepOffsetAPI_MakePipeShellPtr()->Build();
        Py_Return;
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return 0;
    }
}

PyObject* BRepOffsetAPI_MakePipeShellPy::shape(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;

    try {
        const TopoDS_Shape& shape = this->getBRepOffsetAPI_MakePipeShellPtr()->Shape();
        return new TopoShapePy(new TopoShape(shape));
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return 0;
    }
}

PyObject* BRepOffsetAPI_MakePipeShellPy::firstShape(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;

    try {
        TopoDS_Shape shape = this->getBRepOffsetAPI_MakePipeShellPtr()->FirstShape();
        return new TopoShapePy(new TopoShape(shape));
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return 0;
    }
}

PyObject* BRepOffsetAPI_MakePipeShellPy::lastShape(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;

    try {
        TopoDS_Shape shape = this->getBRepOffsetAPI_MakePipeShellPtr()->LastShape();
        return new TopoShapePy(new TopoShape(shape));
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return 0;
    }
}

PyObject* BRepOffsetAPI_MakePipeShellPy::generated(PyObject *args)
{
    PyObject *shape;
    if (!PyArg_ParseTuple(args, "O!",&Part::TopoShapePy::Type,&shape))
        return 0;

    try {
        const TopoDS_Shape& s = static_cast<Part::TopoShapePy*>(shape)->getTopoShapePtr()->getShape();
        const TopTools_ListOfShape& list = this->getBRepOffsetAPI_MakePipeShellPtr()->Generated(s);

        Py::List shapes;
        TopTools_ListIteratorOfListOfShape it;
        for (it.Initialize(list); it.More(); it.Next()) {
            const TopoDS_Shape& s = it.Value();
            shapes.append(Py::asObject(new TopoShapePy(new TopoShape(s))));
        }
        return Py::new_reference_to(shapes);
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return 0;
    }
}

PyObject* BRepOffsetAPI_MakePipeShellPy::setTolerance(PyObject *args)
{
    double tol3d, boundTol, tolAngular;
    if (!PyArg_ParseTuple(args, "ddd",&tol3d,&boundTol,&tolAngular))
        return 0;

    try {
        this->getBRepOffsetAPI_MakePipeShellPtr()->SetTolerance(tol3d, boundTol, tolAngular);
        Py_Return;
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return 0;
    }
}

PyObject* BRepOffsetAPI_MakePipeShellPy::setTransitionMode(PyObject *args)
{
    int mode;
    if (!PyArg_ParseTuple(args, "i",&mode))
        return 0;

    try {
        this->getBRepOffsetAPI_MakePipeShellPtr()->SetTransitionMode(BRepBuilderAPI_TransitionMode(mode));
        Py_Return;
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return 0;
    }
}

PyObject* BRepOffsetAPI_MakePipeShellPy::setMaxDegree(PyObject *args)
{
    int degree;
    if (!PyArg_ParseTuple(args, "i",&degree))
        return 0;

    try {
#if OCC_VERSION_HEX >= 0x060800
        this->getBRepOffsetAPI_MakePipeShellPtr()->SetMaxDegree(degree);
        Py_Return;
#else
        (void)args;
        PyErr_SetString(PyExc_RuntimeError, "requires OCC >= 6.8");
        return 0;
#endif
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return 0;
    }
}

PyObject* BRepOffsetAPI_MakePipeShellPy::setMaxSegments(PyObject *args)
{
    int nbseg;
    if (!PyArg_ParseTuple(args, "i",&nbseg))
        return 0;

    try {
#if OCC_VERSION_HEX >= 0x060800
        this->getBRepOffsetAPI_MakePipeShellPtr()->SetMaxSegments(nbseg);
        Py_Return;
#else
        (void)args;
        PyErr_SetString(PyExc_RuntimeError, "requires OCC >= 6.8");
        return 0;
#endif
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return 0;
    }
}

PyObject* BRepOffsetAPI_MakePipeShellPy::setForceApproxC1(PyObject *args)
{
    PyObject *obj;
    if (!PyArg_ParseTuple(args, "O!",&PyBool_Type,&obj))
        return 0;

    try {
#if OCC_VERSION_HEX >= 0x060700
        this->getBRepOffsetAPI_MakePipeShellPtr()->SetForceApproxC1(PyObject_IsTrue(obj) ? Standard_True : Standard_False);
        Py_Return;
#else
        PyErr_SetString(PyExc_RuntimeError, "requires OCC >= 6.7");
        return 0;
#endif
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return 0;
    }
}


PyObject* BRepOffsetAPI_MakePipeShellPy::simulate(PyObject *args)
{
    int nbsec;
    if (!PyArg_ParseTuple(args, "i",&nbsec))
        return 0;

    try {
        TopTools_ListOfShape list; 
        this->getBRepOffsetAPI_MakePipeShellPtr()->Simulate(nbsec, list);

        Py::List shapes;
        TopTools_ListIteratorOfListOfShape it;
        for (it.Initialize(list); it.More(); it.Next()) {
            const TopoDS_Shape& s = it.Value();
            shapes.append(Py::asObject(new TopoShapePy(new TopoShape(s))));
        }
        return Py::new_reference_to(shapes);
    }
    catch (Standard_Failure& e) {
        PyErr_SetString(PartExceptionOCCError, e.GetMessageString());
        return 0;
    }
}


PyObject *BRepOffsetAPI_MakePipeShellPy::getCustomAttributes(const char* ) const
{
    return 0;
}

int BRepOffsetAPI_MakePipeShellPy::setCustomAttributes(const char* , PyObject *)
{
    return 0; 
}
