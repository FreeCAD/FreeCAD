#include "PreCompiled.h"

#include "ParaGeometryPy.h"
#include "ParaGeometryPy.cpp"

using namespace FCS;

// returns a string which represents the object e.g. when printed in python
std::string ParaGeometryPy::representation(void) const
{
    return ParaObjectPy::representation();
}

PyObject* ParaGeometryPy::toShape(PyObject* args)
{
    if (! PyArg_ParseTuple(args, ""))
        return nullptr;
    return Py::new_reference_to(getParaGeometryPtr()->toShape().getHandledObject());
}


PyObject *ParaGeometryPy::getCustomAttributes(const char* attr) const
{
    return ParaObjectPy::getCustomAttributes(attr);
}

int ParaGeometryPy::setCustomAttributes(const char* attr, PyObject* obj)
{
    return ParaObjectPy::setCustomAttributes(attr, obj);
}
