
#include "PreCompiled.h"

#include "Mod/Path/PathSimulator/App/PathSim.h"

// inclusion of the generated files (generated out of PathSimPy.xml)
#include "PathSimPy.h"
#include "PathSimPy.cpp"

using namespace PathSimulator;

// returns a string which represents the object e.g. when printed in python
std::string PathSimPy::representation(void) const
{
    return std::string("<PathSim object>");
}

PyObject *PathSimPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of PathSimPy and the Twin object 
    return new PathSimPy(new PathSim);
}

// constructor method
int PathSimPy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}


PyObject* PathSimPy::BeginSimulation(PyObject * /*args*/, PyObject * /*kwds*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not yet implemented");
    return 0;
}

PyObject* PathSimPy::SetCurrentTool(PyObject * /*args*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not yet implemented");
    return 0;
}





Py::Object PathSimPy::getTool(void) const
{
    //return Py::Object();
    throw Py::AttributeError("Not yet implemented");
}

PyObject *PathSimPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int PathSimPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0; 
}


