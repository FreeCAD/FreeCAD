/****************************************************************************
 *   Copyright (c) 2017 Zheng Lei (realthunder) <realthunder.dev@gmail.com> *
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

#include <Base/GeometryPyCXX.h>
#include <Base/PyWrapParseTupleAndKeywords.h>
#include <Mod/Part/App/OCCError.h>
#include <Mod/Part/App/TopoShapePy.h>

// inclusion of the generated files (generated out of AreaPy.xml)
#include "PathPy.h"
#include "AreaPy.h"
#include "AreaPy.cpp"


static PyObject * areaAbort(PyObject *, PyObject *args, PyObject *kwd) {
    static const std::array<const char *, 2> kwlist{"aborting", nullptr};
    PyObject *pObj = Py_True;
    if (!Base::Wrapped_ParseTupleAndKeywords(args,kwd,"|O!",kwlist,&PyBool_Type,&pObj)) {
        return nullptr;
    }

    Area::abort(Base::asBoolean(pObj));

    Py_Return;
}

static PyObject * areaSetParams(PyObject *, PyObject *args, PyObject *kwd) {

    static const std::array<const char *, 43> kwlist {PARAM_FIELD_STRINGS(NAME,AREA_PARAMS_STATIC_CONF),nullptr};

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
    if (!Base::Wrapped_ParseTupleAndKeywords(args, kwd,
                "|" PARAM_PY_KWDS(AREA_PARAMS_STATIC_CONF), kwlist,
                PARAM_REF(PARAM_FNAME,AREA_PARAMS_STATIC_CONF)))
        return nullptr;

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
        return nullptr;

    const AreaStaticParams &params = Area::getDefaultParams();

    PyObject *dict = PyDict_New();
#define AREA_SRC(_param) params.PARAM_FNAME(_param)
    PARAM_PY_DICT_SET_VALUE(dict,NAME,AREA_SRC,AREA_PARAMS_STATIC_CONF)
    return dict;
}

static PyObject * areaGetParamsDesc(PyObject *, PyObject *args, PyObject *kwd) {
    PyObject *pcObj = Py_False;
    static const std::array<const char *, 2> kwlist {"as_string", nullptr};
    if (!Base::Wrapped_ParseTupleAndKeywords(args, kwd, "|O!",kwlist,&PyBool_Type,&pcObj))
        return nullptr;

    if (Base::asBoolean(pcObj))
        return PyUnicode_FromString(PARAM_PY_DOC(NAME,AREA_PARAMS_STATIC_CONF));

    PyObject *dict = PyDict_New();
    PARAM_PY_DICT_SET_DOC(dict,NAME,AREA_PARAMS_STATIC_CONF)
    return dict;
}

static const PyMethodDef areaOverrides[] = {
    {
        "setParams",nullptr,0,
        "setParam(key=value...): Set algorithm parameters. You can call getParamsDesc() to \n"
        "get a list of supported parameters and their descriptions.\n"
        PARAM_PY_DOC(NAME,AREA_PARAMS_CONF)
    },
    {
        "add",nullptr,0,
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
        "makeOffset",nullptr,0,
        "makeOffset(index=-1, " PARAM_PY_ARGS_DOC(ARG,AREA_PARAMS_OFFSET) "):\n"
        "Make an 2D offset of the shape.\n"
        "\n* index (-1): the index of the section. -1 means all sections. No effect on planar shape.\n"
        PARAM_PY_DOC(ARG,AREA_PARAMS_OFFSET),
    },
    {
        "makePocket",nullptr,0,
        "makePocket(index=-1, " PARAM_PY_ARGS_DOC(ARG,AREA_PARAMS_POCKET) "):\n"
        "Generate pocket toolpath of the shape.\n"
        "\n* index (-1): the index of the section. -1 means all sections. No effect on planar shape.\n"
        PARAM_PY_DOC(ARG,AREA_PARAMS_POCKET),
    },
    {
        "makeSections",nullptr,0,
        "makeSections(" PARAM_PY_ARGS_DOC(ARG,AREA_PARAMS_SECTION_EXTRA) ", heights=[], plane=None):\n"
        "Make a list of area holding the sectioned children shapes on given heights\n"
        PARAM_PY_DOC(ARG,AREA_PARAMS_SECTION_EXTRA)
        "\n* heights ([]): a list of section heights, the meaning of the value is determined by 'mode'.\n"
        "If not specified, the current SectionCount, and SectionOffset of this Area is used.\n"
        "\n* plane (None): optional shape to specify a section plane. If not give, the current workplane\n"
        "of this Area is used if section mode is 'Workplane'.",
    },
    {
        "getClearedArea",nullptr,0,
        "getClearedArea(path, diameter, zmax, bbox):\n"
        "Gets the area cleared when a tool of the specified diameter follows the gcode represented in the path, ignoring cleared space above zmax and path segments that don't affect space within the x/y space of bbox.\n",
    },
    {
        "getRestArea",nullptr,0,
        "getRestArea(clearedAreas, diameter):\n"
        "Rest machining: gets the area left to be machined, assuming some of this area has already been cleared previous tool paths.\n"
        "clearedAreas: the regions already cleared.\n"
        "diameter: the tool diameter that finishes clearing this area.\n",
    },
    {
        "toTopoShape",nullptr,0,
        "toTopoShape():\n"
    },
    {
        "setDefaultParams",reinterpret_cast<PyCFunction>(reinterpret_cast<void (*) ()>(areaSetParams)), METH_VARARGS|METH_KEYWORDS|METH_STATIC,
        "setDefaultParams(key=value...):\n"
        "Static method to set the default parameters of all following Path.Area, plus the following\n"
        "additional parameters.\n"
    },
    {
        "getDefaultParams",(PyCFunction)areaGetParams, METH_VARARGS|METH_STATIC,
        "getDefaultParams(): Static method to return the current default parameters."
    },
    {
        "abort",reinterpret_cast<PyCFunction>(reinterpret_cast<void (*) ()>(areaAbort)), METH_VARARGS|METH_KEYWORDS|METH_STATIC,
        "abort(aborting=True): Static method to abort any ongoing operation\n"
        "\nTo ensure no stray abortion is left in the previous operation, it is advised to manually clear\n"
        "the aborting flag by calling abort(False) before starting a new operation.",
    },
    {
        "getParamsDesc",reinterpret_cast<PyCFunction>(reinterpret_cast<void (*) ()>(areaGetParamsDesc)), METH_VARARGS|METH_KEYWORDS|METH_STATIC,
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
std::string AreaPy::representation() const
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
        return nullptr;
    }

    // If setParams() was successful it increments the ref counter.
    // So, it must be decremented again.
    Py_DecRef(ret);
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
        return nullptr;

#define GET_TOPOSHAPE(_p) static_cast<Part::TopoShapePy*>(_p)->getTopoShapePtr()->getShape()
    getAreaPtr()->setPlane(GET_TOPOSHAPE(pcObj));
    Py_INCREF(this);
    return this;
}

PyObject* AreaPy::getShape(PyObject *args, PyObject *keywds)
{
    PyObject *pcObj = Py_False;
    short index=-1;
    static const std::array<const char *, 3> kwlist{"index", "rebuild", nullptr};
    if (!Base::Wrapped_ParseTupleAndKeywords(args, keywds,"|hO!",kwlist,&index,&PyBool_Type,&pcObj))
        return nullptr;

    PY_TRY {
        if (Base::asBoolean(pcObj))
            getAreaPtr()->clean();
        return Py::new_reference_to(Part::shape2pyshape(getAreaPtr()->getShape(index)));
    } PY_CATCH_OCC
}

PyObject* AreaPy::add(PyObject *args, PyObject *keywds)
{
    PARAM_PY_DECLARE_INIT(PARAM_FARG,AREA_PARAMS_OPCODE)
    PyObject *pcObj;

    //Strangely, PyArg_ParseTupleAndKeywords requires all arguments to be keyword based,
    //even non-optional ones? That doesn't make sense in python. Seems only in python 3
    //they added '$' to address that issue.
    static const std::array<const char *, 3> kwlist {"shape",PARAM_FIELD_STRINGS(ARG,AREA_PARAMS_OPCODE), nullptr};

    if (!Base::Wrapped_ParseTupleAndKeywords(args, keywds,
                "O|" PARAM_PY_KWDS(AREA_PARAMS_OPCODE),
                kwlist,&pcObj,PARAM_REF(PARAM_FARG,AREA_PARAMS_OPCODE)))
        return nullptr;

    PY_TRY {
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
                    return nullptr;
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
    } PY_CATCH_OCC

    PyErr_SetString(PyExc_TypeError, "shape must be 'TopoShape' or list of 'TopoShape'");
    return nullptr;
}

PyObject* AreaPy::makeOffset(PyObject *args, PyObject *keywds)
{
    //Generate a keyword string defined in the ARG field of OFFSET parameter list
    static const std::array<const char *, 6> kwlist {"index",PARAM_FIELD_STRINGS(ARG,AREA_PARAMS_OFFSET), nullptr};
    short index = -1;

    //Declare variables defined in the ARG field of the OFFSET parameter list with
    //initialization to defaults
    PARAM_PY_DECLARE_INIT(PARAM_FARG,AREA_PARAMS_OFFSET)

    //Parse arguments to overwrite the defaults
    if (!Base::Wrapped_ParseTupleAndKeywords(args, keywds,
                "|h" PARAM_PY_KWDS(AREA_PARAMS_OFFSET), kwlist,
                &index,PARAM_REF(PARAM_FARG,AREA_PARAMS_OFFSET)))
        return nullptr;

    PY_TRY {
        //Expand the variable as function call arguments
        TopoDS_Shape resultShape = getAreaPtr()->makeOffset(index,
                            PARAM_PY_FIELDS(PARAM_FARG,AREA_PARAMS_OFFSET));
        return Py::new_reference_to(Part::shape2pyshape(resultShape));
    } PY_CATCH_OCC
}

PyObject* AreaPy::makePocket(PyObject *args, PyObject *keywds)
{
    static const std::array<const char *, 11> kwlist {"index",PARAM_FIELD_STRINGS(ARG,AREA_PARAMS_POCKET), nullptr};
    short index = -1;

    PARAM_PY_DECLARE_INIT(PARAM_FARG,AREA_PARAMS_POCKET)
    //Override pocket mode default
    mode = Area::PocketModeZigZagOffset;

    if (!Base::Wrapped_ParseTupleAndKeywords(args, keywds,
                "|h" PARAM_PY_KWDS(AREA_PARAMS_POCKET), kwlist,
                &index,PARAM_REF(PARAM_FARG,AREA_PARAMS_POCKET))) {
        return nullptr;
    }

    PY_TRY {
        TopoDS_Shape resultShape = getAreaPtr()->makePocket(index,
                            PARAM_PY_FIELDS(PARAM_FARG,AREA_PARAMS_POCKET));
        return Py::new_reference_to(Part::shape2pyshape(resultShape));
    } PY_CATCH_OCC
}

PyObject* AreaPy::makeSections(PyObject *args, PyObject *keywds)
{
    static const std::array<const char *, 5> kwlist {PARAM_FIELD_STRINGS(ARG,AREA_PARAMS_SECTION_EXTRA),
                            "heights", "plane", nullptr};
    PyObject *heights = nullptr;
    PyObject *plane = nullptr;

    PARAM_PY_DECLARE_INIT(PARAM_FARG,AREA_PARAMS_SECTION_EXTRA)

    if (!Base::Wrapped_ParseTupleAndKeywords(args, keywds,
                "|" PARAM_PY_KWDS(AREA_PARAMS_SECTION_EXTRA) "OO!", kwlist,
                PARAM_REF(PARAM_FARG,AREA_PARAMS_SECTION_EXTRA),
                &heights, &(Part::TopoShapePy::Type), &plane)) {
        return nullptr;
    }

    PY_TRY {
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
                        return nullptr;
                    }
                    h.push_back(PyFloat_AsDouble(item));
                }
            }else{
                PyErr_SetString(PyExc_TypeError, "heights must be of type float or list/tuple of float");
                return nullptr;
            }
        }

        std::vector<std::shared_ptr<Area> > sections = getAreaPtr()->makeSections(
                            PARAM_PY_FIELDS(PARAM_FARG,AREA_PARAMS_SECTION_EXTRA),
                            h,plane?GET_TOPOSHAPE(plane):TopoDS_Shape());

        Py::List ret;
        for(auto &area : sections)
            ret.append(Py::asObject(new AreaPy(new Area(*area,true))));
        return Py::new_reference_to(ret);
    } PY_CATCH_OCC
}

PyObject* AreaPy::getClearedArea(PyObject *args)
{
    PY_TRY {
        PyObject *pyPath, *pyBbox;
        double diameter, zmax;
        if (!PyArg_ParseTuple(args, "OddO", &pyPath, &diameter, &zmax, &pyBbox))
            return nullptr;
	if (!PyObject_TypeCheck(pyPath, &(PathPy::Type))) {
		PyErr_SetString(PyExc_TypeError, "path must be of type PathPy");
		return nullptr;
	}
	if (!PyObject_TypeCheck(pyBbox, &(Base::BoundBoxPy::Type))) {
		PyErr_SetString(PyExc_TypeError, "bbox must be of type BoundBoxPy");
		return nullptr;
	}
	const PathPy *path = static_cast<PathPy*>(pyPath);
        const Py::BoundingBox bbox(pyBbox, false);
        std::shared_ptr<Area> clearedArea = getAreaPtr()->getClearedArea(path->getToolpathPtr(), diameter, zmax, bbox.getValue());
        auto pyClearedArea = Py::asObject(new AreaPy(new Area(*clearedArea, true)));
        return Py::new_reference_to(pyClearedArea);
    } PY_CATCH_OCC
}

PyObject* AreaPy::getRestArea(PyObject *args)
{
    PY_TRY {
        PyObject *pyClearedAreas;
        std::vector<std::shared_ptr<Area>> clearedAreas;
        double diameter;
        if (!PyArg_ParseTuple(args, "Od", &pyClearedAreas, &diameter))
            return nullptr;
        if (pyClearedAreas && PyObject_TypeCheck(pyClearedAreas, &PyList_Type)) {
            Py::Sequence clearedAreasSeq(pyClearedAreas);
            clearedAreas.reserve(clearedAreasSeq.size());
            for (Py::Sequence::iterator it = clearedAreasSeq.begin(); it != clearedAreasSeq.end(); ++it) {
                PyObject *item = (*it).ptr();
                if (!PyObject_TypeCheck(item, &(AreaPy::Type))) {
                    PyErr_SetString(PyExc_TypeError, "cleared areas must only contain AreaPy type");
                    return nullptr;
                }
                clearedAreas.push_back(std::make_shared<Area>(*static_cast<AreaPy*>(item)->getAreaPtr(), true));
            }
        } else {
            PyErr_SetString(PyExc_TypeError, "clearedAreas must be of type list of AreaPy");
            return nullptr;
        }

        std::shared_ptr<Area> restArea = getAreaPtr()->getRestArea(clearedAreas, diameter);
        if (!restArea) {
            return Py_None;
        }
        auto pyRestArea = Py::asObject(new AreaPy(new Area(*restArea, true)));
        return Py::new_reference_to(pyRestArea);
    } PY_CATCH_OCC
}

PyObject* AreaPy::toTopoShape(PyObject *args)
{
    PY_TRY {
      if (!PyArg_ParseTuple(args, ""))
          return nullptr;
      return Py::new_reference_to(Part::shape2pyshape(getAreaPtr()->toTopoShape()));
    } PY_CATCH_OCC
}

PyObject* AreaPy::setDefaultParams(PyObject *, PyObject *)
{
    return nullptr;
}

PyObject* AreaPy::setParams(PyObject *args, PyObject *keywds)
{
    static const std::array<const char *, 43> kwlist {PARAM_FIELD_STRINGS(NAME,AREA_PARAMS_CONF),nullptr};

    //Declare variables defined in the NAME field of the CONF parameter list
    PARAM_PY_DECLARE(PARAM_FNAME,AREA_PARAMS_CONF);

    AreaParams params = getAreaPtr()->getParams();

    //populate the CONF variables with params
    PARAM_FOREACH(AREA_SET,AREA_PARAMS_CONF)

    //Parse arguments to overwrite CONF variables
    if (!Base::Wrapped_ParseTupleAndKeywords(args, keywds,
                "|" PARAM_PY_KWDS(AREA_PARAMS_CONF), kwlist,
                PARAM_REF(PARAM_FNAME,AREA_PARAMS_CONF)))
        return nullptr;

    PY_TRY {
        //populate 'params' with the CONF variables
        PARAM_FOREACH(AREA_GET,AREA_PARAMS_CONF)

        getAreaPtr()->setParams(params);
        Py_INCREF(this);
        return this;
    } PY_CATCH_OCC
}

PyObject* AreaPy::getParams(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    const AreaParams &params =getAreaPtr()->getParams();

    PyObject *dict = PyDict_New();
    PARAM_PY_DICT_SET_VALUE(dict,NAME,AREA_SRC,AREA_PARAMS_CONF)
    return dict;
}

PyObject* AreaPy::getDefaultParams(PyObject *)
{
    return nullptr;
}

PyObject* AreaPy::abort(PyObject *, PyObject *) {
    return nullptr;
}

PyObject* AreaPy::getParamsDesc(PyObject *, PyObject *)
{
    return nullptr;
}

Py::List AreaPy::getSections() const {
    Py::List ret;
	Area *area = getAreaPtr();
    for(size_t i=0,count=area->getSectionCount(); i<count;++i)
        ret.append(Part::shape2pyshape(getAreaPtr()->getShape(i)));
    return ret;
}

Py::List AreaPy::getShapes() const {
    Py::List ret;
	Area *area = getAreaPtr();
    const std::list<Area::Shape> &shapes = area->getChildren();
    for(auto &s : shapes)
        ret.append(Py::TupleN(Part::shape2pyshape(s.shape),Py::Int(s.op)));
    return ret;
}

Py::Object AreaPy::getWorkplane() const {
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
    return nullptr;
}

int AreaPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
