/***************************************************************************
 *   Copyright (c) Yorik van Havre (yorik@uncreated.net) 2014              *
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
#include "Base/Reader.h"
#include "Mod/Path/App/Tooltable.h"

// inclusion of the generated files (generated out of ToolPy.xml and TooltablePy.xml)
#include "ToolPy.h"
#include "ToolPy.cpp"
#include "TooltablePy.h"
#include "TooltablePy.cpp"

using namespace Path;



// ToolPy



// returns a string which represents the object e.g. when printed in python
std::string ToolPy::representation(void) const
{
    std::stringstream str;
    str.precision(5);
    str << "Tool ";
    str << getToolPtr()->Name;
    return str.str();
}

PyObject *ToolPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of ToolPy and the Twin object
    return new ToolPy(new Tool);
}

// constructor method
int ToolPy::PyInit(PyObject* args, PyObject* kwd)
{
    char *name="Default tool";
    char *type = "Undefined";
    char *mat = "Undefined";
    PyObject *dia = 0;
    PyObject *len = 0;
    PyObject *fla = 0;
    PyObject *cor = 0;
    PyObject *ang = 0;
    PyObject *hei = 0;
    int version = 1;

    static char *kwlist[] = {"name", "tooltype", "material", "diameter", "lengthOffset", "flatRadius", "cornerRadius", "cuttingEdgeAngle", "cuttingEdgeHeight" , "version", NULL};

    PyObject *dict = 0;
    if (!kwd && (PyObject_TypeCheck(args, &PyDict_Type) || PyArg_ParseTuple(args, "O!", &PyDict_Type, &dict))) {
        static PyObject *arg = PyTuple_New(0);
        if (PyObject_TypeCheck(args, &PyDict_Type)) {
          dict = args;
        }
        if (!PyArg_ParseTupleAndKeywords(arg, dict, "|sssOOOOOOi", kwlist, &name, &type, &mat, &dia, &len, &fla, &cor, &ang, &hei, &version)) {
            return -1;
        }
    } else {
        PyErr_Clear();
        if (!PyArg_ParseTupleAndKeywords(args, kwd, "|sssOOOOOO", kwlist, &name, &type, &mat, &dia, &len, &fla, &cor, &ang, &hei)) {
            return -1;
        }
    }

    if (1 != version) {
        PyErr_SetString(PyExc_TypeError, "Unsupported Tool template version");
        return -1;
    }

    getToolPtr()->Name = name;

    std::string typeStr(type);
    getToolPtr()->Type = Tool::getToolType(typeStr);

    std::string matStr(mat);
    getToolPtr()->Material = Tool::getToolMaterial(matStr);

    getToolPtr()->Diameter          = dia ? PyFloat_AsDouble(dia) : 0.0;
    getToolPtr()->LengthOffset      = len ? PyFloat_AsDouble(len) : 0.0;
    getToolPtr()->FlatRadius        = fla ? PyFloat_AsDouble(fla) : 0.0;
    getToolPtr()->CornerRadius      = cor ? PyFloat_AsDouble(cor) : 0.0;
    getToolPtr()->CuttingEdgeAngle  = ang ? PyFloat_AsDouble(ang) : 180.0;
    getToolPtr()->CuttingEdgeHeight = hei ? PyFloat_AsDouble(hei) : 0.0;

    return 0;
}

// attributes get/setters

Py::String ToolPy::getName(void) const
{
    return Py::String(getToolPtr()->Name.c_str());
}

void ToolPy::setName(Py::String arg)
{
    std::string name = arg.as_std_string();
    getToolPtr()->Name = name;
}

Py::String ToolPy::getToolType(void) const
{
  return Py::String(Tool::TypeName(getToolPtr()->Type));
}

void ToolPy::setToolType(Py::String arg)
{
    std::string typeStr(arg.as_std_string());
    getToolPtr()->Type = Tool::getToolType(typeStr);

}

Py::String ToolPy::getMaterial(void) const
{
  return Py::String(Tool::MaterialName(getToolPtr()->Material));
}

void ToolPy::setMaterial(Py::String arg)
{
    std::string matStr(arg.as_std_string());
    getToolPtr()->Material = Tool::getToolMaterial(matStr);
}

Py::Float ToolPy::getDiameter(void) const
{
    return Py::Float(getToolPtr()->Diameter);
}

void  ToolPy::setDiameter(Py::Float arg)
{
    getToolPtr()->Diameter = arg.operator double();
}

Py::Float ToolPy::getLengthOffset(void) const
{
    return Py::Float(getToolPtr()->LengthOffset);
}

void  ToolPy::setLengthOffset(Py::Float arg)
{
    getToolPtr()->LengthOffset = arg.operator double();
}

Py::Float ToolPy::getFlatRadius(void) const
{
    return Py::Float(getToolPtr()->FlatRadius);
}

void  ToolPy::setFlatRadius(Py::Float arg)
{
    getToolPtr()->FlatRadius = arg.operator double();
}

Py::Float ToolPy::getCornerRadius(void) const
{
    return Py::Float(getToolPtr()->CornerRadius);
}

void  ToolPy::setCornerRadius(Py::Float arg)
{
    getToolPtr()->CornerRadius = arg.operator double();
}

Py::Float ToolPy::getCuttingEdgeAngle(void) const
{
    return Py::Float(getToolPtr()->CuttingEdgeAngle);
}

void  ToolPy::setCuttingEdgeAngle(Py::Float arg)
{
    getToolPtr()->CuttingEdgeAngle = arg.operator double();
}

Py::Float ToolPy::getCuttingEdgeHeight(void) const
{
    return Py::Float(getToolPtr()->CuttingEdgeHeight);
}

void  ToolPy::setCuttingEdgeHeight(Py::Float arg)
{
    getToolPtr()->CuttingEdgeHeight = arg.operator double();
}

// custom attributes get/set

PyObject *ToolPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int ToolPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}

PyObject* ToolPy::copy(PyObject * args)
{
    if (PyArg_ParseTuple(args, "")) {
        return new ToolPy(new Path::Tool(*getToolPtr()));
    }
    throw Py::TypeError("This method accepts no argument");
}

PyObject* ToolPy::setFromTemplate(PyObject * args)
{
    char *pstr = 0;
    if (PyArg_ParseTuple(args, "s", &pstr)) {
        // embed actual string in dummy tag so XMLReader can consume that on construction
        std::ostringstream os;
        os << "<snippet>" << pstr <<  "</snippet>";
        std::istringstream is(os.str());
        Base::XMLReader reader("", is);
        getToolPtr()->Restore(reader);
        Py_Return ;
    }

    PyErr_Clear();
    if (!PyInit(args, 0)) {
        Py_Return ;
    }

    PyErr_SetString(PyExc_TypeError, "argument must be a string or dictionary");
    return 0;
}

#if PY_MAJOR_VERSION >= 3
#  define PYSTRING_FROMSTRING(str)  PyUnicode_FromString(str)
#  define PYINT_TYPE                PyLong_Type
#  define PYINT_FROMLONG(l)         PyLong_FromLong(l)
#  define PYINT_ASLONG(o)           PyLong_AsLong(o)
#else
#  define PYSTRING_FROMSTRING(str)  PyString_FromString(str)
#  define PYINT_TYPE                PyInt_Type
#  define PYINT_FROMLONG(l)         PyInt_FromLong(l)
#  define PYINT_ASLONG(o)           PyInt_AsLong(o)
#endif

PyObject* ToolPy::templateAttrs(PyObject * args)
{
    if (!args || PyArg_ParseTuple(args, "")) {
        PyObject *dict = PyDict_New();
        PyDict_SetItemString(dict, "version", PYINT_FROMLONG(1));
        PyDict_SetItemString(dict, "name", PYSTRING_FROMSTRING(getToolPtr()->Name.c_str()));
        PyDict_SetItemString(dict, "tooltype",PYSTRING_FROMSTRING(Tool::TypeName(getToolPtr()->Type)));
        PyDict_SetItemString(dict, "material", PYSTRING_FROMSTRING(Tool::MaterialName(getToolPtr()->Material)));
        PyDict_SetItemString(dict, "diameter", PyFloat_FromDouble(getToolPtr()->Diameter));
        PyDict_SetItemString(dict, "lengthOffset", PyFloat_FromDouble(getToolPtr()->LengthOffset));
        PyDict_SetItemString(dict, "flatRadius",  PyFloat_FromDouble(getToolPtr()->FlatRadius));
        PyDict_SetItemString(dict, "cornerRadius", PyFloat_FromDouble(getToolPtr()->CornerRadius));
        PyDict_SetItemString(dict, "cuttingEdgeAngle", PyFloat_FromDouble(getToolPtr()->CuttingEdgeAngle));
        PyDict_SetItemString(dict, "cuttingEdgeHeight", PyFloat_FromDouble(getToolPtr()->CuttingEdgeHeight));
        return dict;
    }
    throw Py::TypeError("This method accepts no argument");
}

PyObject* ToolPy::getToolTypes(PyObject * args)
{
    if (PyArg_ParseTuple(args, "")) {
        std::vector<std::string> toolTypes = Tool::ToolTypes();
        PyObject *list = PyList_New(0);
        for(unsigned i = 0; i != toolTypes.size(); i++) {

            PyList_Append(list, PYSTRING_FROMSTRING(toolTypes[i].c_str()));
        }
        return list;
    }
    throw Py::TypeError("This method accepts no argument");
}

PyObject* ToolPy::getToolMaterials(PyObject * args)
{
    if (PyArg_ParseTuple(args, "")) {
        std::vector<std::string> toolMaterials = Tool::ToolMaterials();
        PyObject *list = PyList_New(0);
        for(unsigned i = 0; i != toolMaterials.size(); i++) {

            PyList_Append(list, PYSTRING_FROMSTRING(toolMaterials[i].c_str()));
        }
        return list;
    }
    throw Py::TypeError("This method accepts no argument");
}


// TooltablePy




// returns a string which represents the object e.g. when printed in python
std::string TooltablePy::representation(void) const
{
    std::stringstream str;
    str.precision(5);
    str << "Tooltable containing ";
    str << getTooltablePtr()->getSize() << " tools";
    return str.str();
}

PyObject *TooltablePy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    return new TooltablePy(new Tooltable);
}

// constructor method
int TooltablePy::PyInit(PyObject* args, PyObject* /*kwd*/)
{
    if (PyArg_ParseTuple(args, "")) {
        return 0;
    }
    PyErr_Clear(); // set by PyArg_ParseTuple()

    PyObject *pcObj;
    if (PyArg_ParseTuple(args, "O!", &(PyDict_Type), &pcObj)) {
        try {
            Py::Dict dict(pcObj);
            setTools(dict);
        } catch(...) {
            PyErr_SetString(PyExc_TypeError, "The dictionary can only contain int:tool pairs");
            return -1;
        }
        return 0;
    }
    PyErr_Clear(); // set by PyArg_ParseTuple()

    if (PyArg_ParseTuple(args, "O!", &(PyList_Type), &pcObj)) {
        Py::List list(pcObj);
        for (Py::List::iterator it = list.begin(); it != list.end(); ++it) {
            if (PyObject_TypeCheck((*it).ptr(), &(Path::ToolPy::Type))) {
                Path::Tool &tool = *static_cast<Path::ToolPy*>((*it).ptr())->getToolPtr();
                getTooltablePtr()->addTool(tool);
            }
        }
        return 0;
    }

    PyErr_SetString(PyExc_TypeError, "Argument must be either empty or a list or a dictionary");
    return -1;
}

// Commands get/set

Py::Dict TooltablePy::getTools(void) const
{
    PyObject *dict = PyDict_New();
    for(std::map<int,Path::Tool*>::iterator i = getTooltablePtr()->Tools.begin(); i != getTooltablePtr()->Tools.end(); ++i) {
        PyObject *tool = new Path::ToolPy(i->second);
        PyDict_SetItem(dict,PYINT_FROMLONG(i->first),tool);
    }
    return Py::Dict(dict);
}

void TooltablePy::setTools(Py::Dict arg)
{
    getTooltablePtr()->Tools.clear();
    PyObject* dict_copy = PyDict_Copy(arg.ptr());
    PyObject *key, *value;
    Py_ssize_t pos = 0;
    while (PyDict_Next(dict_copy, &pos, &key, &value)) {
        if ( PyObject_TypeCheck(key,&(PYINT_TYPE)) && ((PyObject_TypeCheck(value, &(Path::ToolPy::Type))) || PyObject_TypeCheck(value, &PyDict_Type))) {
            int ckey = (int)PYINT_ASLONG(key);
            if (PyObject_TypeCheck(value, &(Path::ToolPy::Type))) {
              Path::Tool &tool = *static_cast<Path::ToolPy*>(value)->getToolPtr();
              getTooltablePtr()->setTool(tool, ckey);
            } else {
              PyErr_Clear();
              Path::Tool *tool = new Path::Tool;
              // The 'pyTool' object must be created on the heap otherwise Python
              // will fail to properly track the reference counts and aborts
              // in debug mode.
              Path::ToolPy* pyTool = new Path::ToolPy(tool);
              PyObject* success = pyTool->setFromTemplate(value);
              if (!success) {
                Py_DECREF(pyTool);
                throw Py::Exception();
              }
              getTooltablePtr()->setTool(*tool, ckey);
              Py_DECREF(pyTool);
              Py_DECREF(success);
            }
        } else {
            throw Py::TypeError("The dictionary can only contain int:tool pairs");
        }
    }
}

// specific methods

PyObject* TooltablePy::copy(PyObject * args)
{
    if (PyArg_ParseTuple(args, "")) {
        return new TooltablePy(new Path::Tooltable(*getTooltablePtr()));
    }
    throw Py::TypeError("This method accepts no argument");
}

PyObject* TooltablePy::addTools(PyObject * args)
{
    PyObject* o;
    if (PyArg_ParseTuple(args, "O!", &(Path::ToolPy::Type), &o)) {
        Path::Tool &tool = *static_cast<Path::ToolPy*>(o)->getToolPtr();
        getTooltablePtr()->addTool(tool);
        //return new TooltablePy(new Path::Tooltable(*getTooltablePtr()));
        Py_INCREF(Py_None);
        return Py_None;
    }
    PyErr_Clear();
    if (PyArg_ParseTuple(args, "O!", &(PyList_Type), &o)) {
        Py::List list(o);
        for (Py::List::iterator it = list.begin(); it != list.end(); ++it) {
            if (PyObject_TypeCheck((*it).ptr(), &(Path::ToolPy::Type))) {
                Path::Tool &tool = *static_cast<Path::ToolPy*>((*it).ptr())->getToolPtr();
                getTooltablePtr()->addTool(tool);
            }
        }
        //return new TooltablePy(new Path::Tooltable(*getTooltablePtr()));
        Py_INCREF(Py_None);
        return Py_None;
    }
    Py_Error(Base::BaseExceptionFreeCADError, "Wrong parameters - tool or list of tools expected");
}

PyObject* TooltablePy::setTool(PyObject * args)
{
    PyObject* o;
    int pos = -1;
    if (PyArg_ParseTuple(args, "iO!", &pos, &(Path::ToolPy::Type), &o)) {
        Path::Tool &tool = *static_cast<Path::ToolPy*>(o)->getToolPtr();
        getTooltablePtr()->setTool(tool,pos);
        //return new TooltablePy(new Path::Tooltable(*getTooltablePtr()));
        Py_INCREF(Py_None);
        return Py_None;
    }
    Py_Error(Base::BaseExceptionFreeCADError, "Wrong parameters - expected tool and optional integer");
}

PyObject* TooltablePy::getTool(PyObject * args)
{
    int pos = -1;
    if (PyArg_ParseTuple(args, "i", &pos)) {
        if (getTooltablePtr()->hasTool(pos))
        {
            Path::Tool tool = getTooltablePtr()->getTool(pos);
            return new ToolPy(new Path::Tool(tool));
        }
        else
        {
            Py_INCREF(Py_None);
            return Py_None;
        }
    }
    Py_Error(Base::BaseExceptionFreeCADError, "Argument must be integer");
}

PyObject* TooltablePy::deleteTool(PyObject * args)
{
    int pos = -1;
    if (PyArg_ParseTuple(args, "|i", &pos)) {
        getTooltablePtr()->deleteTool(pos);
        //return new TooltablePy(new Path::Tooltable(*getTooltablePtr()));
        Py_INCREF(Py_None);
        return Py_None;
    }
    Py_Error(Base::BaseExceptionFreeCADError, "Wrong parameters - expected an integer (optional)");
}

// custom attributes get/set

PyObject *TooltablePy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int TooltablePy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}


PyObject* TooltablePy::setFromTemplate(PyObject * args)
{
    PyObject *dict = 0;
    if (PyArg_ParseTuple(args, "O!", &PyDict_Type, &dict)) {
      Py::Dict d(dict);
      setTools(d);
      Py_Return ;
    }

    PyErr_SetString(PyExc_TypeError, "argument must be a dictionary returned from templateAttrs()");
    return 0;
}

PyObject* TooltablePy::templateAttrs(PyObject * args)
{
    (void)args;
    PyObject *dict = PyDict_New();
    for(std::map<int,Path::Tool*>::iterator i = getTooltablePtr()->Tools.begin(); i != getTooltablePtr()->Tools.end(); ++i) {
        // The 'tool' object must be created on the heap otherwise Python
        // will fail to properly track the reference counts and aborts
        // in debug mode.
        Path::ToolPy* tool = new Path::ToolPy(new Path::Tool(*i->second));
        PyObject *attrs = tool->templateAttrs(0);
        PyDict_SetItem(dict, PYINT_FROMLONG(i->first), attrs);
        Py_DECREF(tool);
    }
    return dict;
}
