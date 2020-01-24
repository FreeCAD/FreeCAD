#include "PreCompiled.h"

#include "G2D/ParaCurvePy.h"
#include "G2D/ParaCurvePy.cpp"

using namespace FCS;

// returns a string which represents the object e.g. when printed in python
std::string ParaCurvePy::representation(void) const
{
    return ParaGeometryPy::representation();
}



PyObject *ParaCurvePy::getCustomAttributes(const char* attr) const
{
    return ParaGeometryPy::getCustomAttributes(attr);
}

int ParaCurvePy::setCustomAttributes(const char* attr, PyObject* obj)
{
    return ParaGeometryPy::setCustomAttributes(attr, obj);
}

