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
    PyObject *pos;
    char *name="Default tool";
    char *type = "Undefined";
    char *mat = "Undefined";
    PyObject *dia = 0;
    PyObject *len = 0;
    PyObject *fla = 0;
    PyObject *cor = 0;
    PyObject *ang = 0;
    PyObject *hei = 0;

    static char *kwlist[] = {"name", "tooltype", "material", "diameter", "lengthOffset", "flatRadius", "cornerRadius", "cuttingEdgeAngle", "cuttingEdgeHeight" ,NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwd, "|sssOOOOOO", kwlist,
                                     &name, &type, &mat, &dia, &len, &fla, &cor, &ang, &hei ))
        return -1;

    getToolPtr()->Name = name;
    std::string typeStr(type);
    if(typeStr=="Drill")
        getToolPtr()->Type = Tool::DRILL;
    else if(typeStr=="CenterDrill")
        getToolPtr()->Type = Tool::CENTERDRILL;
    if(typeStr=="CounterSink")
        getToolPtr()->Type = Tool::COUNTERSINK;
    if(typeStr=="CounterBore")
        getToolPtr()->Type = Tool::COUNTERBORE;
    if(typeStr=="Reamer")
        getToolPtr()->Type = Tool::REAMER;
    if(typeStr=="Tap")
        getToolPtr()->Type = Tool::TAP;
    else if(typeStr=="EndMill")
        getToolPtr()->Type = Tool::ENDMILL;
    else if(typeStr=="SlotCutter")
        getToolPtr()->Type = Tool::SLOTCUTTER;
    else if(typeStr=="BallEndMill")
        getToolPtr()->Type = Tool::BALLENDMILL;
    else if(typeStr=="ChamferMill")
        getToolPtr()->Type = Tool::CHAMFERMILL;
    else if(typeStr=="CornerRound")
        getToolPtr()->Type = Tool::CORNERROUND;
    else if(typeStr=="Engraver")
        getToolPtr()->Type = Tool::ENGRAVER;
    else 
        getToolPtr()->Type = Tool::UNDEFINED;
        
    std::string matStr(mat);
    if(matStr=="HighSpeedSteel")
        getToolPtr()->Material = Tool::HIGHSPEEDSTEEL;
    else if(matStr=="Carbide")
        getToolPtr()->Material = Tool::CARBIDE;
    else if(matStr=="HighCarbonToolSteel")
        getToolPtr()->Material = Tool::HIGHCARBONTOOLSTEEL;
    else if(matStr=="CastAlloy")
        getToolPtr()->Material = Tool::CASTALLOY;
    else if(matStr=="Ceramics")
        getToolPtr()->Material = Tool::CERAMICS;
    else if(matStr=="Diamond")
        getToolPtr()->Material = Tool::DIAMOND;
    else if(matStr=="Sialon")
        getToolPtr()->Material = Tool::SIALON;
    else 
        getToolPtr()->Material = Tool::MATUNDEFINED;

    getToolPtr()->Diameter = PyFloat_AsDouble(dia);
    getToolPtr()->LengthOffset = PyFloat_AsDouble(len);
    getToolPtr()->FlatRadius = PyFloat_AsDouble(fla);
    getToolPtr()->CornerRadius = PyFloat_AsDouble(cor);
    getToolPtr()->CuttingEdgeAngle = PyFloat_AsDouble(ang);
    getToolPtr()->CuttingEdgeHeight = PyFloat_AsDouble(hei);

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
    if(getToolPtr()->Type == Tool::DRILL)
        return Py::String("Drill");
    else if(getToolPtr()->Type == Tool::CENTERDRILL)
        return Py::String("CenterDrill");
    else if(getToolPtr()->Type == Tool::COUNTERSINK)
        return Py::String("CounterSink");
    else if(getToolPtr()->Type == Tool::COUNTERBORE)
        return Py::String("CounterBore");
    else if(getToolPtr()->Type == Tool::REAMER)
        return Py::String("Reamer");
    else if(getToolPtr()->Type == Tool::TAP)
        return Py::String("Tap");
    else if(getToolPtr()->Type == Tool::ENDMILL)
        return Py::String("EndMill");
    else if(getToolPtr()->Type == Tool::SLOTCUTTER)
        return Py::String("SlotCutter");
    else if(getToolPtr()->Type == Tool::BALLENDMILL)
        return Py::String("BallEndMill");
    else if(getToolPtr()->Type == Tool::CHAMFERMILL)
        return Py::String("ChamferMill");
    else if(getToolPtr()->Type == Tool::CORNERROUND)
        return Py::String("CornerRound");
    else if(getToolPtr()->Type == Tool::ENGRAVER)
        return Py::String("Engraver");
    else
        return Py::String("Undefined");
}

void ToolPy::setToolType(Py::String arg)
{
    std::string typeStr(arg.as_std_string());
    if(typeStr=="Drill")
        getToolPtr()->Type = Tool::DRILL;
    else if(typeStr=="CenterDrill")
        getToolPtr()->Type = Tool::CENTERDRILL;
    else if(typeStr=="CounterSink")
        getToolPtr()->Type = Tool::COUNTERSINK;
    else if(typeStr=="CounterBore")
        getToolPtr()->Type = Tool::COUNTERBORE;
    else if(typeStr=="Reamer")
        getToolPtr()->Type = Tool::REAMER;
    else if(typeStr=="Tap")
        getToolPtr()->Type = Tool::TAP;
    else if(typeStr=="EndMill")
        getToolPtr()->Type = Tool::ENDMILL;
    else if(typeStr=="SlotCutter")
        getToolPtr()->Type = Tool::SLOTCUTTER;
    else if(typeStr=="BallEndMill")
        getToolPtr()->Type = Tool::BALLENDMILL;
    else if(typeStr=="ChamferMill")
        getToolPtr()->Type = Tool::CHAMFERMILL;
    else if(typeStr=="CornerRound")
        getToolPtr()->Type = Tool::CORNERROUND;

    else if(typeStr=="Engraver")
        getToolPtr()->Type = Tool::ENGRAVER;
    else 
        getToolPtr()->Type = Tool::UNDEFINED;
}

Py::String ToolPy::getMaterial(void) const
{
    if(getToolPtr()->Material == Tool::HIGHSPEEDSTEEL)
        return Py::String("HighSpeedSteel");
    else if(getToolPtr()->Material == Tool::CARBIDE)
        return Py::String("Carbide");
    else if(getToolPtr()->Material == Tool::HIGHCARBONTOOLSTEEL)
        return Py::String("HighCarbonToolSteel");
    else if(getToolPtr()->Material == Tool::CASTALLOY)
        return Py::String("CastAlloy");
    else if(getToolPtr()->Material == Tool::CERAMICS)
        return Py::String("Ceramics");
    else if(getToolPtr()->Material == Tool::DIAMOND)
        return Py::String("Diamond");
    else if(getToolPtr()->Material == Tool::SIALON)
        return Py::String("Sialon");
    else
        return Py::String("Undefined");
}

void ToolPy::setMaterial(Py::String arg)
{
    std::string matStr(arg.as_std_string());
    if(matStr=="HighSpeedSteel")
        getToolPtr()->Material = Tool::HIGHSPEEDSTEEL;
    else if(matStr=="Carbide")
        getToolPtr()->Material = Tool::CARBIDE;
    else if(matStr=="HighCarbonToolSteel")
        getToolPtr()->Material = Tool::HIGHCARBONTOOLSTEEL;
    else if(matStr=="CastAlloy")
        getToolPtr()->Material = Tool::CASTALLOY;
    else if(matStr=="Ceramics")
        getToolPtr()->Material = Tool::CERAMICS;
    else if(matStr=="Diamond")
        getToolPtr()->Material = Tool::DIAMOND;
    else if(matStr=="Sialon")
        getToolPtr()->Material = Tool::SIALON;
    else 
        getToolPtr()->Material = Tool::MATUNDEFINED;
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
    throw Py::Exception("This method accepts no argument");
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
    PyObject *pcObj;
    if (PyArg_ParseTuple(args, "|O!", &(PyDict_Type), &pcObj)) {
        PyObject *key, *value;
        Py_ssize_t pos = 0;
        while (PyDict_Next(pcObj, &pos, &key, &value)) {
            if ( !PyObject_TypeCheck(key,&(PyInt_Type)) || !PyObject_TypeCheck(value,&(Path::ToolPy::Type)) ) {
                PyErr_SetString(PyExc_TypeError, "The dictionary can only contain int:tool pairs");
                return -1;
            }
            int ckey = (int)PyInt_AsLong(key);
            Path::Tool &tool = *static_cast<Path::ToolPy*>(value)->getToolPtr();
            getTooltablePtr()->setTool(tool,ckey);
        }
        return 0;
    }
    PyErr_Clear(); // set by PyArg_ParseTuple()
    
    if (PyArg_ParseTuple(args, "|O!", &(PyList_Type), &pcObj)) {
        Py::List list(pcObj);
        for (Py::List::iterator it = list.begin(); it != list.end(); ++it) {
            if (PyObject_TypeCheck((*it).ptr(), &(Path::ToolPy::Type))) {
                Path::Tool &tool = *static_cast<Path::ToolPy*>((*it).ptr())->getToolPtr();
                getTooltablePtr()->addTool(tool);
            }
        }
        return 0;
    }
    
    PyErr_SetString(PyExc_TypeError, "Argument must be a list or a dictionary");
    return -1;    
}

// Commands get/set

Py::Dict TooltablePy::getTools(void) const
{
    PyObject *dict = PyDict_New();
    for(std::map<int,Path::Tool*>::iterator i = getTooltablePtr()->Tools.begin(); i != getTooltablePtr()->Tools.end(); ++i) {
        PyObject *tool = new Path::ToolPy(i->second);
        PyDict_SetItem(dict,PyInt_FromLong(i->first),tool);
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
        if ( PyObject_TypeCheck(key,&(PyInt_Type)) && (PyObject_TypeCheck(value,&(Path::ToolPy::Type))) ) {
            int ckey = (int)PyInt_AsLong(key);
            Path::Tool &tool = *static_cast<Path::ToolPy*>(value)->getToolPtr();
            getTooltablePtr()->setTool(tool,ckey);
        } else {
            throw Py::Exception("The dictionary can only contain int:tool pairs");
        }
    }
}

// specific methods

PyObject* TooltablePy::copy(PyObject * args)
{
    if (PyArg_ParseTuple(args, "")) {
        return new TooltablePy(new Path::Tooltable(*getTooltablePtr()));
    }
    throw Py::Exception("This method accepts no argument");
}

PyObject* TooltablePy::addTools(PyObject * args)
{
    PyObject* o;
    if (PyArg_ParseTuple(args, "O!", &(Path::ToolPy::Type), &o)) {
        Path::Tool &tool = *static_cast<Path::ToolPy*>(o)->getToolPtr();
        getTooltablePtr()->addTool(tool);
        //return new TooltablePy(new Path::Tooltable(*getTooltablePtr()));
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
        } else
            return Py_None;
    }
    Py_Error(Base::BaseExceptionFreeCADError, "Argument must be integer");
}

PyObject* TooltablePy::deleteTool(PyObject * args)
{
    int pos = -1;
    if (PyArg_ParseTuple(args, "|i", &pos)) {
        getTooltablePtr()->deleteTool(pos);
        //return new TooltablePy(new Path::Tooltable(*getTooltablePtr()));
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


