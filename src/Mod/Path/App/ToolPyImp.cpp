
#include "PreCompiled.h"

#include "Mod/Path/App/Tooltable.h"

// inclusion of the generated files (generated out of ToolPy.xml)
#include "ToolPy.h"
#include "ToolPy.cpp"

using namespace Path;

// returns a string which represents the object e.g. when printed in python
std::string ToolPy::representation(void) const
{
    return std::string("<Tool object>");
}

PyObject *ToolPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of ToolPy and the Twin object 
    return new ToolPy(new Tool);
}

// constructor method
int ToolPy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}


PyObject* ToolPy::copy(PyObject * /*args*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not yet implemented");
    return 0;
}





Py::String ToolPy::getName(void) const
{
    //return Py::String();
    throw Py::AttributeError("Not yet implemented");
}

void ToolPy::setName(Py::String /*arg*/)
{
    throw Py::AttributeError("Not yet implemented");
}

Py::String ToolPy::getToolType(void) const
{
    //return Py::String();
    throw Py::AttributeError("Not yet implemented");
}

void ToolPy::setToolType(Py::String /*arg*/)
{
    throw Py::AttributeError("Not yet implemented");
}

Py::String ToolPy::getMaterial(void) const
{
    //return Py::String();
    throw Py::AttributeError("Not yet implemented");
}

void ToolPy::setMaterial(Py::String /*arg*/)
{
    throw Py::AttributeError("Not yet implemented");
}

Py::Float ToolPy::getDiameter(void) const
{
    //return Py::Float();
    throw Py::AttributeError("Not yet implemented");
}

void ToolPy::setDiameter(Py::Float /*arg*/)
{
    throw Py::AttributeError("Not yet implemented");
}

Py::Float ToolPy::getLengthOffset(void) const
{
    //return Py::Float();
    throw Py::AttributeError("Not yet implemented");
}

void ToolPy::setLengthOffset(Py::Float /*arg*/)
{
    throw Py::AttributeError("Not yet implemented");
}

Py::Float ToolPy::getFlatRadius(void) const
{
    //return Py::Float();
    throw Py::AttributeError("Not yet implemented");
}

void ToolPy::setFlatRadius(Py::Float /*arg*/)
{
    throw Py::AttributeError("Not yet implemented");
}

Py::Float ToolPy::getCornerRadius(void) const
{
    //return Py::Float();
    throw Py::AttributeError("Not yet implemented");
}

void ToolPy::setCornerRadius(Py::Float /*arg*/)
{
    throw Py::AttributeError("Not yet implemented");
}

Py::Float ToolPy::getCuttingEdgeAngle(void) const
{
    //return Py::Float();
    throw Py::AttributeError("Not yet implemented");
}

void ToolPy::setCuttingEdgeAngle(Py::Float /*arg*/)
{
    throw Py::AttributeError("Not yet implemented");
}

Py::Float ToolPy::getCuttingEdgeHeight(void) const
{
    //return Py::Float();
    throw Py::AttributeError("Not yet implemented");
}

void ToolPy::setCuttingEdgeHeight(Py::Float /*arg*/)
{
    throw Py::AttributeError("Not yet implemented");
}

PyObject *ToolPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int ToolPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0; 
}


