#include "PreCompiled.h"

#include "G2D/ParaPointPy.h"
#include "G2D/ParaPointPy.cpp"

using namespace FCS;

PyObject *ParaPointPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of ParaPointPy and the Twin object
    return new ParaPointPy(new ParaPoint);
}

// constructor method
int ParaPointPy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}


// returns a string which represents the object e.g. when printed in python
std::string ParaPointPy::representation(void) const
{
    return ParaGeometryPy::representation();
}



PyObject *ParaPointPy::getCustomAttributes(const char* attr) const
{
    return ParaGeometryPy::getCustomAttributes(attr);
}

int ParaPointPy::setCustomAttributes(const char* attr, PyObject* obj)
{
    return ParaGeometryPy::setCustomAttributes(attr, obj);
}

