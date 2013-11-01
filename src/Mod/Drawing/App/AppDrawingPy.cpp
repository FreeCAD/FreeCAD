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
#include <boost/regex.hpp>


using namespace Drawing;
using namespace Part;
using namespace std;

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

        ProjectionAlgos Alg(pShape->getTopoShapePtr()->_Shape,Vector);

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

        ProjectionAlgos Alg(pShape->getTopoShapePtr()->_Shape,Vector);

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
    float tol=0.1f;

    if (!PyArg_ParseTuple(args, "O!|O!sff", &(TopoShapePy::Type), &pcObjShape,
                                            &(Base::VectorPy::Type), &pcObjDir, &type, &scale, &tol))
        return NULL;

    PY_TRY {
        TopoShapePy* pShape = static_cast<TopoShapePy*>(pcObjShape);
        Base::Vector3d Vector(0,0,1);
        if (pcObjDir)
            Vector = static_cast<Base::VectorPy*>(pcObjDir)->value();
        ProjectionAlgos Alg(pShape->getTopoShapePtr()->_Shape,Vector);

        bool hidden = false;
        if (type && std::string(type) == "ShowHiddenLines")
            hidden = true;

        Py::String result(Alg.getSVG(hidden?ProjectionAlgos::WithHidden:ProjectionAlgos::Plain, scale, tol));
        return Py::new_reference_to(result);

    } PY_CATCH;
}

static PyObject * 
projectToDXF(PyObject *self, PyObject *args)
{
    PyObject *pcObjShape;
    PyObject *pcObjDir=0;
    const char *type=0;
    float scale=1.0f;
    float tol=0.1f;

    if (!PyArg_ParseTuple(args, "O!|O!sff", &(TopoShapePy::Type), &pcObjShape,
                                            &(Base::VectorPy::Type), &pcObjDir, &type, &scale, &tol))
        return NULL;

    PY_TRY {
        TopoShapePy* pShape = static_cast<TopoShapePy*>(pcObjShape);
        Base::Vector3d Vector(0,0,1);
        if (pcObjDir)
            Vector = static_cast<Base::VectorPy*>(pcObjDir)->value();
        ProjectionAlgos Alg(pShape->getTopoShapePtr()->_Shape,Vector);

        bool hidden = false;
        if (type && std::string(type) == "ShowHiddenLines")
            hidden = true;

        Py::String result(Alg.getDXF(hidden?ProjectionAlgos::WithHidden:ProjectionAlgos::Plain, scale, tol));
        return Py::new_reference_to(result);

    } PY_CATCH;
}

static PyObject * 
removeSvgTags(PyObject *self, PyObject *args)
{
    const char* svgcode;
    if (!PyArg_ParseTuple(args, "s",&svgcode))
        return NULL; 

    PY_TRY {
        string svg(svgcode);
        string empty = "";
        string endline = "--endOfLine--";
        string linebreak = "\\n";
        // removing linebreaks for regex to work
        boost::regex e1 ("\\n");
        svg = boost::regex_replace(svg, e1, endline);
        // removing starting xml definition
        boost::regex e2 ("<\\?xml.*?\\?>");
        svg = boost::regex_replace(svg, e2, empty);
        // removing starting svg tag
        boost::regex e3 ("<svg.*?>");
        svg = boost::regex_replace(svg, e3, empty);
        // removing sodipodi tags -- DANGEROUS, some sodipodi tags are single, better leave it
        //boost::regex e4 ("<sodipodi.*?>");
        //svg = boost::regex_replace(svg, e4, empty);
        // removing metadata tags
        boost::regex e5 ("<metadata.*?</metadata>");
        svg = boost::regex_replace(svg, e5, empty);
        // removing closing svg tags
        boost::regex e6 ("</svg>");
        svg = boost::regex_replace(svg, e6, empty);
        // restoring linebreaks
        boost::regex e7 ("--endOfLine--");
        svg = boost::regex_replace(svg, e7, linebreak);
        Py::String result(svg);
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
   {"projectToDXF"       ,projectToDXF      ,METH_VARARGS,
     "string = projectToDXF(TopoShape[,App.Vector Direction, string type]) -- Project a shape and return the DXF representation as string."},
   {"removeSvgTags"       ,removeSvgTags      ,METH_VARARGS,
     "string = removeSvgTags(string) -- Removes the opening and closing svg tags and other metatags from a svg code, making it embeddable"},
    {NULL, NULL}        /* end of table marker */
};
