/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2002     *
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
# include <Python.h>
#endif

#include <Mod/Part/App/TopoShapePy.h>
#include "ProjectionAlgos.h"
#include <Base/Console.h>
#include <Base/VectorPy.h>


using namespace Drawing;
using namespace Part;

static PyObject * 
project(PyObject *self, PyObject *args)
{
    PyObject *pcObjShape;
    PyObject *pcObjDir=0;

    if (!PyArg_ParseTuple(args, "O!|O!", &(TopoShapePy::Type), &pcObjShape,&(Base::VectorPy::Type), &pcObjDir))     // convert args: Python->C
        return NULL;                             // NULL triggers exception

    PY_TRY {
        TopoShapePy* pShape = static_cast<TopoShapePy*>(pcObjShape);
        Base::Vector3d Vector(0,0,1);
        if (pcObjDir)
            Vector = *static_cast<Base::VectorPy*>(pcObjDir)->getVectorPtr();

        ProjectionAlgos Alg(pShape->getTopoShapePtr()->_Shape,Base::Vector3f((float)Vector.x,(float)Vector.y,(float)Vector.z));

        Py::List list;
        list.append(Py::Object(new TopoShapePy(new TopoShape(Alg.V)) , true));
        list.append(Py::Object(new TopoShapePy(new TopoShape(Alg.V1)), true));
        list.append(Py::Object(new TopoShapePy(new TopoShape(Alg.H)) , true));
        list.append(Py::Object(new TopoShapePy(new TopoShape(Alg.H1)), true));

        return Py::new_reference_to(list);

    } PY_CATCH;

}

static PyObject * 
projectEx(PyObject *self, PyObject *args)
{
    PyObject *pcObjShape;
    PyObject *pcObjDir=0;

    if (!PyArg_ParseTuple(args, "O!|O!", &(TopoShapePy::Type), &pcObjShape,&(Base::VectorPy::Type), &pcObjDir))     // convert args: Python->C
        return NULL;                             // NULL triggers exception

    PY_TRY {
        TopoShapePy* pShape = static_cast<TopoShapePy*>(pcObjShape);
        Base::Vector3d Vector(0,0,1);
        if (pcObjDir)
            Vector = *static_cast<Base::VectorPy*>(pcObjDir)->getVectorPtr();

        ProjectionAlgos Alg(pShape->getTopoShapePtr()->_Shape,Base::Vector3f((float)Vector.x,(float)Vector.y,(float)Vector.z));

        Py::List list;
        list.append(Py::Object(new TopoShapePy(new TopoShape(Alg.V)) , true));
        list.append(Py::Object(new TopoShapePy(new TopoShape(Alg.V1)), true));
        list.append(Py::Object(new TopoShapePy(new TopoShape(Alg.VN)), true));
        list.append(Py::Object(new TopoShapePy(new TopoShape(Alg.VO)), true));
        list.append(Py::Object(new TopoShapePy(new TopoShape(Alg.VI)), true));
        list.append(Py::Object(new TopoShapePy(new TopoShape(Alg.H)) , true));
        list.append(Py::Object(new TopoShapePy(new TopoShape(Alg.H1)), true));
        list.append(Py::Object(new TopoShapePy(new TopoShape(Alg.HN)), true));
        list.append(Py::Object(new TopoShapePy(new TopoShape(Alg.HO)), true));
        list.append(Py::Object(new TopoShapePy(new TopoShape(Alg.HI)), true));

        return Py::new_reference_to(list);

    } PY_CATCH;
}

static PyObject * 
projectToSVG(PyObject *self, PyObject *args)
{
    PyObject *pcObjShape;
    PyObject *pcObjDir=0;
    const char *type=0;
    float scale=1.0f;

    if (!PyArg_ParseTuple(args, "O!|O!sf", &(TopoShapePy::Type), &pcObjShape,
                                           &(Base::VectorPy::Type), &pcObjDir, &type, &scale))
        return NULL;

    PY_TRY {
        TopoShapePy* pShape = static_cast<TopoShapePy*>(pcObjShape);
        Base::Vector3d Vector(0,0,1);
        if (pcObjDir)
            Vector = static_cast<Base::VectorPy*>(pcObjDir)->value();
        ProjectionAlgos Alg(pShape->getTopoShapePtr()->_Shape,Base::Vector3f((float)Vector.x,(float)Vector.y,(float)Vector.z));

        bool hidden = false;
        if (type && std::string(type) == "ShowHiddenLines")
            hidden = true;

        Py::String result(Alg.getSVG(hidden?ProjectionAlgos::WithHidden:ProjectionAlgos::Plain, scale));
        return Py::new_reference_to(result);

    } PY_CATCH;
}

/* registration table  */
struct PyMethodDef Drawing_methods[] = {
   {"project"       ,project      ,METH_VARARGS,
     "[visiblyG0,visiblyG1,hiddenG0,hiddenG1] = project(TopoShape[,App.Vector Direction, string type]) -- Project a shape and return the visible/invisible parts of it."},
   {"projectEx"       ,projectEx      ,METH_VARARGS,
     "[V,V1,VN,VO,VI,H,H1,HN,HO,HI] = projectEx(TopoShape[,App.Vector Direction, string type]) -- Project a shape and return the all parts of it."},
   {"projectToSVG"       ,projectToSVG      ,METH_VARARGS,
     "string = projectToSVG(TopoShape[,App.Vector Direction, string type]) -- Project a shape and return the SVG representation as string."},
    {NULL, NULL}        /* end of table marker */
};
