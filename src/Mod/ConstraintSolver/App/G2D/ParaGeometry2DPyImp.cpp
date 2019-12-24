#include "PreCompiled.h"

#include "G2D/ParaGeometry2DPy.h"
#include "G2D/ParaGeometry2DPy.cpp"

#include "ParameterStorePy.h"
#include "ParameterRefPy.h"

using namespace FCS;

// returns a string which represents the object e.g. when printed in python
std::string ParaGeometry2DPy::representation(void) const
{
    return getParaGeometryPtr()->repr();
}



PyObject *ParaGeometry2DPy::getCustomAttributes(const char* attr) const
{
    return ParaGeometryPy::getCustomAttributes(attr);
}

int ParaGeometry2DPy::setCustomAttributes(const char* attr, PyObject* obj)
{
    return ParaGeometryPy::setCustomAttributes(attr, obj);
}

