/****************************************************************************
 *   Copyright (c) 2017 Zheng, Lei (realthunder) <realthunder.dev@gmail.com>*
 *                                                                          *
 *   This file is part of the FreeCAD CAx development system.               *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Library General Public            *
 *   License as published by the Free Software Foundation; either           *
 *   version 2 of the License, or (at your option) any later version.       *
 *                                                                          *
 *   This library  is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Library General Public License for more details.                   *
 *                                                                          *
 *   You should have received a copy of the GNU Library General Public      *
 *   License along with this library; see the file COPYING.LIB. If not,     *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,          *
 *   Suite 330, Boston, MA  02111-1307, USA                                 *
 *                                                                          *
 ****************************************************************************/

#include "PreCompiled.h"

#include <Mod/Part/App/OCCError.h>
#include <Mod/Part/App/TopoShapePy.h>

#include "Mod/Path/App/Area.h"

// inclusion of the generated files (generated out of AreaPy.xml)
#include "AreaPy.h"
#include "AreaPy.cpp"


struct AreaDoc {
    const char *name;
    const char *doc;
};

/** Generate doc string from parameter definitions
 * It will generate doc string and replace the one generated from xml 
 * */
static const AreaDoc myDocs[] = {
    {
        "setParams",
        "setParam(key=value...): Set algorithm parameters. You can call getParamsDesc() to \n"
        "get a list of supported parameters and their descriptions.\n"

        PARAM_PY_DOC(NAME,AREA_PARAMS_CONF)
    },
    {
        "add",

        "add((shape...)," PARAM_PY_ARGS_DOC(ARG,AREA_PARAMS_OPCODE) "):\n"
        "Add TopoShape(s) with given operation code\n"
        PARAM_PY_DOC(ARG,AREA_PARAMS_OPCODE)
        "\nThe first shape's wires will be fused together regardless of the op code given.\n"
        "Subsequent shape's wire will be combined using the op code. All shape wires\n"
        "shall be coplanar, and are used to determine a working plane for face making and\n"
        "offseting. You can call setPlane() to supply a reference shape to determin the\n"
        "working plane in case the added shapes are all colinear lines.\n",
    },

    {
        "makeOffset",

        "makeOffset(" PARAM_PY_ARGS_DOC(ARG,AREA_PARAMS_OFFSET) "):\n"
        "\n* index (-1): the index of the section. -1 means all sections. No effect on planar shape.\n"
        "Make an 2D offset of the shape.\n"
        PARAM_PY_DOC(ARG,AREA_PARAMS_OFFSET),
    },
    {
        "makePocket",

        "makePocket(" PARAM_PY_ARGS_DOC(ARG,AREA_PARAMS_POCKET) "):\n"
        "Generate pocket toolpath of the shape.\n"
        "\n* index (-1): the index of the section. -1 means all sections. No effect on planar shape.\n"
        PARAM_PY_DOC(ARG,AREA_PARAMS_POCKET),
    },
};

struct AreaPyDoc {
    AreaPyDoc() {
        for(PyMethodDef &method : Path::AreaPy::Methods) {
            if(!method.ml_name) continue;
            for(const AreaDoc &doc : myDocs) {
                if(std::strcmp(method.ml_name,doc.name)==0) {
                    method.ml_doc = doc.doc;
                    break;
                }
            }
        }
    }
};

static AreaPyDoc doc;

namespace Part {
extern PartExport Py::Object shape2pyshape(const TopoDS_Shape &shape);
}

using namespace Path;

// returns a string which represents the object e.g. when printed in python
std::string AreaPy::representation(void) const
{
    std::stringstream str;
    str << "<Area object at " << getAreaPtr() << ">";
    return str.str();
}

PyObject *AreaPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    return new AreaPy(new Area);
}

// constructor method
int AreaPy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}

PyObject* AreaPy::setPlane(PyObject *args) {
    PyObject *pcObj;
    if (!PyArg_ParseTuple(args, "O!", &(Part::TopoShapePy::Type), &pcObj))
        Py_Error(Base::BaseExceptionFreeCADError, "Wrong parameters");

#define GET_TOPOSHAPE(_p) static_cast<Part::TopoShapePy*>(_p)->getTopoShapePtr()->getShape()
    getAreaPtr()->setPlane(GET_TOPOSHAPE(pcObj));
    return Py_None;
}

PyObject* AreaPy::getShape(PyObject *args, PyObject *keywds)
{
    PyObject *pcObj = Py_True;
    short index=-1;
    static char *kwlist[] = {"index","rebuild", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, keywds,"|hO",kwlist,&pcObj))
        Py_Error(Base::BaseExceptionFreeCADError, "This method accepts no argument");

    try {
        if(PyObject_IsTrue(pcObj))
            getAreaPtr()->clean(true);
        return Py::new_reference_to(Part::shape2pyshape(getAreaPtr()->getShape(index)));
    }
    PY_CATCH_OCC;
}

PyObject* AreaPy::add(PyObject *args, PyObject *keywds)
{
    PARAM_PY_DECLARE_INIT(PARAM_FARG,AREA_PARAMS_OPCODE)
    PyObject *pcObj;
    static char *kwlist[] = {PARAM_FIELD_STRINGS(ARG,AREA_PARAMS_OPCODE), NULL};
    if (!PyArg_ParseTupleAndKeywords(args, keywds, 
                "O|" PARAM_PY_KWDS(AREA_PARAMS_OPCODE), 
                kwlist,&pcObj,PARAM_REF(PARAM_FARG,AREA_PARAMS_OPCODE)))
        Py_Error(Base::BaseExceptionFreeCADError, "Wrong parameters");

    if (PyObject_TypeCheck(pcObj, &(Part::TopoShapePy::Type))) {
        getAreaPtr()->add(GET_TOPOSHAPE(pcObj),op);
        return Py_None;
    }
    Py::Sequence shapeSeq(pcObj);
    for (Py::Sequence::iterator it = shapeSeq.begin(); it != shapeSeq.end(); ++it) {
        PyObject* item = (*it).ptr();
        if(!PyObject_TypeCheck(item, &(Part::TopoShapePy::Type))) {
            PyErr_SetString(PyExc_TypeError, "non-shape object in sequence");
            return 0;
        }
    }
    for (Py::Sequence::iterator it = shapeSeq.begin(); it != shapeSeq.end(); ++it){
        PyObject* item = (*it).ptr();
        getAreaPtr()->add(GET_TOPOSHAPE(item),
                PARAM_PY_FIELDS(PARAM_FARG,AREA_PARAMS_OPCODE));
    }
    return Py_None;
}

PyObject* AreaPy::makeOffset(PyObject *args, PyObject *keywds)
{
    //Generate a keyword string defined in the ARG field of OFFSET parameter list
    static char *kwlist[] = {"index",PARAM_FIELD_STRINGS(ARG,AREA_PARAMS_OFFSET), NULL};
    short index = -1;

    //Declare variables defined in the ARG field of the OFFSET parameter list with
    //initialization to defaults
    PARAM_PY_DECLARE_INIT(PARAM_FARG,AREA_PARAMS_OFFSET)

    //Parse arguments to overwrite the defaults
    if (!PyArg_ParseTupleAndKeywords(args, keywds, 
                "|h" PARAM_PY_KWDS(AREA_PARAMS_OFFSET), kwlist, 
                &index,PARAM_REF(PARAM_FARG,AREA_PARAMS_OFFSET)))
        Py_Error(Base::BaseExceptionFreeCADError, "Wrong parameters");

    try {
        //Expand the variable as function call arguments
        TopoDS_Shape resultShape = getAreaPtr()->makeOffset(index,
                            PARAM_PY_FIELDS(PARAM_FARG,AREA_PARAMS_OFFSET));
        return Py::new_reference_to(Part::shape2pyshape(resultShape));
    }
    PY_CATCH_OCC;
}

PyObject* AreaPy::makePocket(PyObject *args, PyObject *keywds)
{
    static char *kwlist[] = {"index",PARAM_FIELD_STRINGS(ARG,AREA_PARAMS_POCKET), NULL};
    short index = -1;

    PARAM_PY_DECLARE_INIT(PARAM_FARG,AREA_PARAMS_POCKET)

    if (!PyArg_ParseTupleAndKeywords(args, keywds, 
                "|h" PARAM_PY_KWDS(AREA_PARAMS_POCKET), kwlist, 
                &index,PARAM_REF(PARAM_FARG,AREA_PARAMS_POCKET)))
        Py_Error(Base::BaseExceptionFreeCADError, "Wrong parameters");

    try {
        TopoDS_Shape resultShape = getAreaPtr()->makePocket(index,
                            PARAM_PY_FIELDS(PARAM_FARG,AREA_PARAMS_POCKET));
        return Py::new_reference_to(Part::shape2pyshape(resultShape));
    }
    PY_CATCH_OCC;
}


PyObject* AreaPy::setParams(PyObject *args, PyObject *keywds)
{
    static char *kwlist[] = {PARAM_FIELD_STRINGS(NAME,AREA_PARAMS_CONF),NULL};

    //Declare variables defined in the NAME field of the CONF parameter list
    PARAM_PY_DECLARE(PARAM_FNAME,AREA_PARAMS_CONF);

    AreaParams params = getAreaPtr()->getParams();

#define AREA_SET(_param) \
    PARAM_FNAME(_param) = \
        PARAM_TYPED(PARAM_PY_CAST_,_param)(params.PARAM_FNAME(_param));
    //populate the CONF variables with params
    PARAM_FOREACH(AREA_SET,AREA_PARAMS_CONF)

    //Parse arguments to overwrite CONF variables 
    if (!PyArg_ParseTupleAndKeywords(args, keywds, 
                "|" PARAM_PY_KWDS(AREA_PARAMS_CONF), kwlist, 
                PARAM_REF(PARAM_FNAME,AREA_PARAMS_CONF)))
        Py_Error(Base::BaseExceptionFreeCADError, 
            "Wrong parameters, call getParamsDesc() to get supported params");

#define AREA_GET(_param) \
    params.PARAM_FNAME(_param) = \
        PARAM_TYPED(PARAM_CAST_PY_,_param)(PARAM_FNAME(_param));
    //populate 'params' with the CONF variables
    PARAM_FOREACH(AREA_GET,AREA_PARAMS_CONF)

    getAreaPtr()->setParams(params);
    return Py_None;
}

PyObject* AreaPy::getParams(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        Py_Error(Base::BaseExceptionFreeCADError, "This method accepts no argument");

    const AreaParams &params =getAreaPtr()->getParams();

    PyObject *dict = PyDict_New();
#define AREA_SRC(_param) params.PARAM_FNAME(_param)
    PARAM_PY_DICT_SET_VALUE(dict,NAME,AREA_SRC,AREA_PARAMS_CONF)
    return dict;
}

PyObject* AreaPy::getParamsDesc(PyObject *args, PyObject *keywds)
{
    PyObject *pcObj = Py_True;
    static char *kwlist[] = {"as_string", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, keywds,"|O",kwlist,&pcObj))
        Py_Error(Base::BaseExceptionFreeCADError, "This method accepts no argument");

    if(PyObject_IsTrue(pcObj)) 
        return PyString_FromString(PARAM_PY_DOC(NAME,AREA_PARAMS_CONF));

    PyObject *dict = PyDict_New();
    PARAM_PY_DICT_SET_DOC(dict,NAME,AREA_PARAMS_CONF)
    return dict;
}

// custom attributes get/set

PyObject *AreaPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int AreaPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0; 
}


