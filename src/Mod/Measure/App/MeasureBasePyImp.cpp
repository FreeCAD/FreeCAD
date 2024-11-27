#include "PreCompiled.h"

#include "MeasureBase.h"

// Inclusion of the generated files (generated out of MeasureBasePy.xml)
#include "MeasureBasePy.h"
#include "MeasureBasePy.cpp"


#include <Base/GeometryPyCXX.h>


// returns a string which represents the object e.g. when printed in python
std::string MeasureBasePy::representation() const
{
    return "<Measure::MeasureBase>";
}

PyObject* MeasureBasePy::PyMake(struct _typeobject*, PyObject*, PyObject*)  // Python wrapper
{
    // create a new instance of MeasureBasePy and the Twin object
    return new MeasureBasePy(new MeasureBase);
}


// constructor method
int MeasureBasePy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}

PyObject* MeasureBasePy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int MeasureBasePy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
