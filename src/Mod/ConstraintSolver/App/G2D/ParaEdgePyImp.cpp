#include "PreCompiled.h"

#include "G2D/ParaEdgePy.h"
#include "G2D/ParaEdgePy.cpp"

using namespace FCS;

// returns a string which represents the object e.g. when printed in python
std::string ParaEdgePy::representation(void) const
{
    return ParaGeometryPy::representation();
}



PyObject *ParaEdgePy::getCustomAttributes(const char* attr) const
{
    return ParaGeometryPy::getCustomAttributes(attr);
}

int ParaEdgePy::setCustomAttributes(const char* attr, PyObject* obj)
{
    return ParaGeometryPy::setCustomAttributes(attr, obj);
}

