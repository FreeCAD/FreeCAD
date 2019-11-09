#include "PreCompiled.h"

#include "ParameterStore.h"
#include "ParameterRefPy.h"

#include "ParameterSubsetPy.h"
#include "ParameterSubsetPy.cpp"

PyObject *ParameterSubsetPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of ParameterSubsetPy and the Twin object
    return Py::new_reference_to(ParameterSubset::make());
}

// constructor method
int ParameterSubsetPy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}


// returns a string which represents the object e.g. when printed in python
std::string ParameterSubsetPy::representation(void) const
{
    return std::string("<ParameterSubset object>");
}



PyObject *ParameterSubsetPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int ParameterSubsetPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
