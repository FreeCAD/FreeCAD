#include "PreCompiled.h"

#include "G2D/ParaConicPy.h"
#include "G2D/ParaConicPy.cpp"

#include "ValueSetPy.h"

#include "PyUtils.h"

// returns a string which represents the object e.g. when printed in python
std::string ParaConicPy::representation(void) const
{
    return getParaConicPtr()->repr();
}



PyObject* ParaConicPy::getFocus1(PyObject* args)
{
    PyObject* pcvals = nullptr;
    if (! PyArg_ParseTuple(args, "O!", &ValueSetPy::Type, &pcvals))
        return nullptr;
    Position ret = getParaConicPtr()->getFocus1(*HValueSet(pcvals, false));
    return ret.getPyObject();
}

PyObject* ParaConicPy::getFocus2(PyObject* args)
{
    PyObject* pcvals = nullptr;
    if (! PyArg_ParseTuple(args, "O!", &ValueSetPy::Type, &pcvals))
        return nullptr;
    Position ret = getParaConicPtr()->getFocus2(*HValueSet(pcvals, false));
    return ret.getPyObject();
}

PyObject* ParaConicPy::getF(PyObject* args)
{
    PyObject* pcvals = nullptr;
    if (! PyArg_ParseTuple(args, "O!", &ValueSetPy::Type, &pcvals))
        return nullptr;
    DualNumber ret = getParaConicPtr()->getF(*HValueSet(pcvals, false));
    return ret.getPyObject();
}

PyObject* ParaConicPy::getRMaj(PyObject* args)
{
    PyObject* pcvals = nullptr;
    if (! PyArg_ParseTuple(args, "O!", &ValueSetPy::Type, &pcvals))
        return nullptr;
    DualNumber ret = getParaConicPtr()->getRMaj(*HValueSet(pcvals, false));
    return ret.getPyObject();
}

PyObject* ParaConicPy::getRMin(PyObject* args)
{
    PyObject* pcvals = nullptr;
    if (! PyArg_ParseTuple(args, "O!", &ValueSetPy::Type, &pcvals))
        return nullptr;
    DualNumber ret = getParaConicPtr()->getRMin(*HValueSet(pcvals, false));
    return ret.getPyObject();
}




PyObject* ParaConicPy::getCustomAttributes(const char* attr) const
{
    return ParaCurvePy::getCustomAttributes(attr);
}

int ParaConicPy::setCustomAttributes(const char* attr, PyObject* obj)
{
    return ParaCurvePy::setCustomAttributes(attr, obj);
}

