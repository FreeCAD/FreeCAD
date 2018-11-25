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

#include <Mod/Part/App/TopoShapePy.h>
#include <Base/VectorPy.h>

#include "Area.h"

// inclusion of the generated files (generated out of AreaPy.xml)
#include "AreaPy.h"
#include "AreaPy.cpp"


static PyObject * areaAbort(PyObject *, PyObject *args, PyObject *kwd) {
    static char *kwlist[] = {"aborting", NULL};
    PyObject *pObj = Py_True;
    if (!PyArg_ParseTupleAndKeywords(args,kwd,"|O",kwlist,&pObj))
        return 0;
    Area::abort(PyObject_IsTrue(pObj));
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject * areaSetParams(PyObject *, PyObject *args, PyObject *kwd) {

    static char *kwlist[] = {PARAM_FIELD_STRINGS(NAME,AREA_PARAMS_STATIC_CONF),NULL};

    if(args && PySequence_Size(args)>0) 
        PyErr_SetString(PyExc_ValueError,"Non-keyword argument is not supported");

    //Declare variables defined in the NAME field of the CONF parameter list
    PARAM_PY_DECLARE(PARAM_FNAME,AREA_PARAMS_STATIC_CONF);

    AreaStaticParams params = Area::getDefaultParams();

#define AREA_SET(_param) \
    PARAM_FNAME(_param) = \
        PARAM_TYPED(PARAM_PY_CAST_,_param)(params.PARAM_FNAME(_param));
    //populate the CONF variables with params
    PARAM_FOREACH(AREA_SET,AREA_PARAMS_STATIC_CONF)

    //Parse arguments to overwrite CONF variables 
    if (!PyArg_ParseTupleAndKeywords(args, kwd, 
                "|" PARAM_PY_KWDS(AREA_PARAMS_STATIC_CONF), kwlist, 
                PARAM_REF(PARAM_FNAME,AREA_PARAMS_STATIC_CONF)))
        return 0;

#define AREA_GET(_param) \
    params.PARAM_FNAME(_param) = \
        PARAM_TYPED(PARAM_CAST_PY_,_param)(PARAM_FNAME(_param));
    //populate 'params' with the CONF variables
    PARAM_FOREACH(AREA_GET,AREA_PARAMS_STATIC_CONF)

    Area::setDefaultParams(params);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject* areaGetParams(PyObject *, PyObject *args) {
    if (!PyArg_ParseTuple(args, ""))
        return 0;

    const AreaStaticParams &params = Area::getDefaultParams();

    PyObject *dict = PyDict_New();
#define AREA_SRC(_param) params.PARAM_FNAME(_param)
    PARAM_PY_DICT_SET_VALUE(dict,NAME,AREA_SRC,AREA_PARAMS_STATIC_CONF)
    return dict;
}

static PyObject * areaGetParamsDesc(PyObject *, PyObject *args, PyObject *kwd) {
    PyObject *pcObj = Py_False;
    static char *kwlist[] = {"as_string", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwd, "|O",kwlist,&pcObj))
        return 0;

#if PY_MAJOR_VERSION < 3
    if(PyObject_IsTrue(pcObj)) 
        return PyString_FromString(PARAM_PY_DOC(NAME,AREA_PARAMS_STATIC_CONF));
#else
    if(PyObject_IsTrue(pcObj)) 
        return PyUnicode_FromString(PARAM_PY_DOC(NAME,AREA_PARAMS_STATIC_CONF));
#endif
    PyObject *dict = PyDict_New();
    PARAM_PY_DICT_SET_DOC(dict,NAME,AREA_PARAMS_STATIC_CONF)
    return dict;
}

static const PyMethodDef areaOverrides[] = {
    {
        "setParams",NULL,0,
        "setParam(key=value...): Set algorithm parameters. You can call getParamsDesc() to \n"
        "get a list of supported parameters and their descriptions.\n"
        PARAM_PY_DOC(NAME,AREA_PARAMS_CONF)
    },
    {
        "add",NULL,0,
        "add((shape...)," PARAM_PY_ARGS_DOC(ARG,AREA_PARAMS_OPCODE) "):\n"
        "Add TopoShape(s) with given operation code\n"
        PARAM_PY_DOC(ARG,AREA_PARAMS_OPCODE)
        "\nThe first shape's wires will be unioned together regardless of the op code given\n"
        "(except for 'Compound'). Subsequent shape's wire will be combined using the op code.\n"
        "All shape wires shall be coplanar, and are used to determine a working plane for face\n"
        "making and offsetting. You can call setPlane() to supply a reference shape to determine\n"
        "the workplane in case the added shapes are all colinear lines.\n",
    },

    {
        "makeOffset",NULL,0,
        "makeOffset(index=-1, " PARAM_PY_ARGS_DOC(ARG,AREA_PARAMS_OFFSET) "):\n"
        "Make an 2D offset of the shape.\n"
        "\n* index (-1): the index of the section. -1 means all sections. No effect on planar shape.\n"
        PARAM_PY_DOC(ARG,AREA_PARAMS_OFFSET),
    },
    {
        "makePocket",NULL,0,
        "makePocket(index=-1, " PARAM_PY_ARGS_DOC(ARG,AREA_PARAMS_POCKET) "):\n"
        "Generate pocket toolpath of the shape.\n"
        "\n* index (-1): the index of the section. -1 means all sections. No effect on planar shape.\n"
        PARAM_PY_DOC(ARG,AREA_PARAMS_POCKET),
    },
    {
        "makeSections",NULL,0,
        "makeSections(" PARAM_PY_ARGS_DOC(ARG,AREA_PARAMS_SECTION_EXTRA) ", heights=[], plane=None):\n"
        "Make a list of area holding the sectioned children shapes on given heights\n"
        PARAM_PY_DOC(ARG,AREA_PARAMS_SECTION_EXTRA)
        "\n* heights ([]): a list of section heights, the meaning of the value is determined by 'mode'.\n"
        "If not specified, the current SectionCount, and SectionOffset of this Area is used.\n"
        "\n* plane (None): optional shape to specify a section plane. If not give, the current workplane\n"
        "of this Area is used if section mode is 'Workplane'.",
    },
    {
        "setDefaultParams",reinterpret_cast<PyCFunction>(reinterpret_cast<void (*) (void)>(areaSetParams)), METH_VARARGS|METH_KEYWORDS|METH_STATIC,
        "setDefaultParams(key=value...):\n"
        "Static method to set the default parameters of all following Path.Area, plus the following\n"
        "additional parameters.\n"
    },
    {
        "getDefaultParams",(PyCFunction)areaGetParams, METH_VARARGS|METH_STATIC,
        "getDefaultParams(): Static method to return the current default parameters."
    },
    {
        "abort",reinterpret_cast<PyCFunction>(reinterpret_cast<void (*) (void)>(areaAbort)), METH_VARARGS|METH_KEYWORDS|METH_STATIC,
        "abort(aborting=True): Static method to abort any ongoing operation\n"
        "\nTo ensure no stray abortion is left in the previous operation, it is advised to manually clear\n"
        "the aborting flag by calling abort(False) before starting a new operation.",
    },
    {
        "getParamsDesc",reinterpret_cast<PyCFunction>(reinterpret_cast<void (*) (void)>(areaGetParamsDesc)), METH_VARARGS|METH_KEYWORDS|METH_STATIC,
        "getParamsDesc(as_string=False): Returns a list of supported parameters and their descriptions.\n"
        "\n* as_string: if False, then return a dictionary of documents of all supported parameters."
    },
};

struct AreaPyModifier {
    AreaPyModifier() {
        for(auto &method : Path::AreaPy::Methods) {
            if(!method.ml_name) continue;
            for(auto &entry : areaOverrides) {
                if(std::strcmp(method.ml_name,entry.ml_name)==0) {
                    if(entry.ml_doc)
                        method.ml_doc = entry.ml_doc;
                    if(entry.ml_meth)
                        method.ml_meth = entry.ml_meth;
                    if(entry.ml_flags)
                        method.ml_flags = entry.ml_flags;
                    break;
                }
            }
        }
    }
};

static AreaPyModifier mod;

using namespace Path;

// returns a string which represents the object e.g. when printed in python
std::string AreaPy::representation(void) const
{
    std::stringstream str;
    str << "<Area object at " << getAreaPtr() << ">";
    return str.str();
}

PyObject *AreaPy::PyMake(struct _typeobject *, PyObject *args, PyObject *kwd)  // Python wrapper
{
    AreaPy* ret = new AreaPy(new Area);
    if(!ret->setParams(args,kwd)) {
        Py_DecRef(ret);
        return 0;
    }
    return ret;
}

// constructor method
int AreaPy::PyInit(PyObject* , PyObject* )
{
    return 0;
}

PyObject* AreaPy::setPlane(PyObject *args) {
    PyObject *pcObj;
    if (!PyArg_ParseTuple(args, "O!", &(Part::TopoShapePy::Type), &pcObj))
        return 0;

#define GET_TOPOSHAPE(_p) static_cast<Part::TopoShapePy*>(_p)->getTopoShapePtr()->getShape()
    getAreaPtr()->setPlane(GET_TOPOSHAPE(pcObj));
    Py_INCREF(this);
    return this;
}

PyObject* AreaPy::getShape(PyObject *args, PyObject *keywds)
{
    PyObject *pcObj = Py_False;
    short index=-1;
    static char *kwlist[] = {"index","rebuild", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, keywds,"|hO",kwlist,&index,&pcObj))
        return 0;

    if(PyObject_IsTrue(pcObj))
        getAreaPtr()->clean();
    return Py::new_reference_to(Part::shape2pyshape(getAreaPtr()->getShape(index)));
}

PyObject* AreaPy::add(PyObject *args, PyObject *keywds)
{
    PARAM_PY_DECLARE_INIT(PARAM_FARG,AREA_PARAMS_OPCODE)
    PyObject *pcObj;

    //Strangely, PyArg_ParseTupleAndKeywords requires all arguments to be keyword based,
    //even non-optional ones? That doesn't make sense in python. Seems only in python 3
    //they added '$' to address that issue.
    static char *kwlist[] = {"shape",PARAM_FIELD_STRINGS(ARG,AREA_PARAMS_OPCODE), NULL};

    if (!PyArg_ParseTupleAndKeywords(args, keywds, 
                "O|" PARAM_PY_KWDS(AREA_PARAMS_OPCODE), 
                kwlist,&pcObj,PARAM_REF(PARAM_FARG,AREA_PARAMS_OPCODE)))
        return 0;

    if (PyObject_TypeCheck(pcObj, &(Part::TopoShapePy::Type))) {
        getAreaPtr()->add(GET_TOPOSHAPE(pcObj),op);
        Py_INCREF(this);
        return this;
    } else if (PyObject_TypeCheck(pcObj, &(PyList_Type)) ||
             PyObject_TypeCheck(pcObj, &(PyTuple_Type))) {
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
        Py_INCREF(this);
        return this;
    }

    PyErr_SetString(PyExc_TypeError, "shape must be 'TopoShape' or list of 'TopoShape'");
    return 0;
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
        return 0;

    //Expand the variable as function call arguments
    TopoDS_Shape resultShape = getAreaPtr()->makeOffset(index,
                        PARAM_PY_FIELDS(PARAM_FARG,AREA_PARAMS_OFFSET));
    return Py::new_reference_to(Part::shape2pyshape(resultShape));
}

PyObject* AreaPy::makePocket(PyObject *args, PyObject *keywds)
{
    static char *kwlist[] = {"index",PARAM_FIELD_STRINGS(ARG,AREA_PARAMS_POCKET), NULL};
    short index = -1;

    PARAM_PY_DECLARE_INIT(PARAM_FARG,AREA_PARAMS_POCKET)
    //Override pocket mode default
    mode = Area::PocketModeZigZagOffset;

    if (!PyArg_ParseTupleAndKeywords(args, keywds, 
                "|h" PARAM_PY_KWDS(AREA_PARAMS_POCKET), kwlist, 
                &index,PARAM_REF(PARAM_FARG,AREA_PARAMS_POCKET)))
        return 0;

    TopoDS_Shape resultShape = getAreaPtr()->makePocket(index,
                        PARAM_PY_FIELDS(PARAM_FARG,AREA_PARAMS_POCKET));
    return Py::new_reference_to(Part::shape2pyshape(resultShape));
}

PyObject* AreaPy::makeSections(PyObject *args, PyObject *keywds)
{
    static char *kwlist[] = {PARAM_FIELD_STRINGS(ARG,AREA_PARAMS_SECTION_EXTRA), 
                            "heights", "plane", NULL};
    PyObject *heights = NULL;
    PyObject *plane = NULL;

    PARAM_PY_DECLARE_INIT(PARAM_FARG,AREA_PARAMS_SECTION_EXTRA)

    if (!PyArg_ParseTupleAndKeywords(args, keywds, 
                "|" PARAM_PY_KWDS(AREA_PARAMS_SECTION_EXTRA) "OO!", kwlist, 
                PARAM_REF(PARAM_FARG,AREA_PARAMS_SECTION_EXTRA),
                &heights, &(Part::TopoShapePy::Type), &plane))
        return 0;

    std::vector<double> h;
    if(heights) {
        if (PyObject_TypeCheck(heights, &(PyFloat_Type)))
            h.push_back(PyFloat_AsDouble(heights));
        else if (PyObject_TypeCheck(heights, &(PyList_Type)) ||
            PyObject_TypeCheck(heights, &(PyTuple_Type))) {
            Py::Sequence shapeSeq(heights);
            h.reserve(shapeSeq.size());
            for (Py::Sequence::iterator it = shapeSeq.begin(); it != shapeSeq.end(); ++it) {
                PyObject* item = (*it).ptr();
                if(!PyObject_TypeCheck(item, &(PyFloat_Type))) {
                    PyErr_SetString(PyExc_TypeError, "heights must only contain float type");
                    return 0;
                }
                h.push_back(PyFloat_AsDouble(item));
            }
        }else{
            PyErr_SetString(PyExc_TypeError, "heights must be of type float or list/tuple of float");
            return 0;
        }
    }

    std::vector<std::shared_ptr<Area> > sections = getAreaPtr()->makeSections(
                        PARAM_PY_FIELDS(PARAM_FARG,AREA_PARAMS_SECTION_EXTRA),
                        h,plane?GET_TOPOSHAPE(plane):TopoDS_Shape());

    Py::List ret;
    for(auto &area : sections) 
        ret.append(Py::asObject(new AreaPy(new Area(*area,true))));
    return Py::new_reference_to(ret);
}

PyObject* AreaPy::setDefaultParams(PyObject *, PyObject *)
{
    return 0;
}

PyObject* AreaPy::setParams(PyObject *args, PyObject *keywds)
{
    static char *kwlist[] = {PARAM_FIELD_STRINGS(NAME,AREA_PARAMS_CONF),NULL};

    //Declare variables defined in the NAME field of the CONF parameter list
    PARAM_PY_DECLARE(PARAM_FNAME,AREA_PARAMS_CONF);

    AreaParams params = getAreaPtr()->getParams();

    //populate the CONF variables with params
    PARAM_FOREACH(AREA_SET,AREA_PARAMS_CONF)

    //Parse arguments to overwrite CONF variables 
    if (!PyArg_ParseTupleAndKeywords(args, keywds, 
                "|" PARAM_PY_KWDS(AREA_PARAMS_CONF), kwlist, 
                PARAM_REF(PARAM_FNAME,AREA_PARAMS_CONF)))
        return 0;

    //populate 'params' with the CONF variables
    PARAM_FOREACH(AREA_GET,AREA_PARAMS_CONF)

    getAreaPtr()->setParams(params);
    Py_INCREF(this);
    return this;
}

PyObject* AreaPy::getParams(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;

    const AreaParams &params =getAreaPtr()->getParams();

    PyObject *dict = PyDict_New();
    PARAM_PY_DICT_SET_VALUE(dict,NAME,AREA_SRC,AREA_PARAMS_CONF)
    return dict;
}

PyObject* AreaPy::getDefaultParams(PyObject *)
{
    return 0;
}

PyObject* AreaPy::abort(PyObject *, PyObject *) {
    return 0;
}

PyObject* AreaPy::getParamsDesc(PyObject *, PyObject *)
{
    return 0;
}

Py::List AreaPy::getSections(void) const {
    Py::List ret;
	Area *area = getAreaPtr();
    for(size_t i=0,count=area->getSectionCount(); i<count;++i)
        ret.append(Part::shape2pyshape(getAreaPtr()->getShape(i)));
    return ret;
}

Py::List AreaPy::getShapes(void) const {
    Py::List ret;
	Area *area = getAreaPtr();
    const std::list<Area::Shape> &shapes = area->getChildren();
    for(auto &s : shapes)
        ret.append(Py::TupleN(Part::shape2pyshape(s.shape),Py::Int(s.op)));
    return ret;
}

Py::Object AreaPy::getWorkplane(void) const {
    return Part::shape2pyshape(getAreaPtr()->getPlane());
}

void AreaPy::setWorkplane(Py::Object obj) {
    PyObject* p = obj.ptr();
    if (!PyObject_TypeCheck(p, &(Part::TopoShapePy::Type))) {
        std::string error = std::string("type must be 'TopoShape', not ");
        error += p->ob_type->tp_name;
        throw Py::TypeError(error);
    }
    getAreaPtr()->setPlane(GET_TOPOSHAPE(p));
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
