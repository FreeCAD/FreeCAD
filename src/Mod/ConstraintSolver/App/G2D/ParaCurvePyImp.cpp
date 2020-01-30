#include "PreCompiled.h"

#include "G2D/ParaCurvePy.h"
#include "G2D/ParaCurvePy.cpp"

#include <Base/DualNumberPy.h>
#include "ValueSetPy.h"
#include "G2D/VectorPy.h"

using namespace FCS;

// returns a string which represents the object e.g. when printed in python
std::string ParaCurvePy::representation(void) const
{
    return getParaCurvePtr()->repr();
}

PyObject* ParaCurvePy::value(PyObject *args)
{
    PyObject* pcvals = nullptr;
    PyObject* pcu = nullptr;
    if (! PyArg_ParseTuple(args, "O!O!", &ValueSetPy::Type, &pcvals, &Base::DualNumberPy::Type, &pcu))
        return nullptr;
    Vector ret = getParaCurvePtr()->value(*HValueSet(pcvals, false), static_cast<Base::DualNumberPy*>(pcu)->value);
    return ret.getPyObject();
}

PyObject* ParaCurvePy::tangent(PyObject *args)
{
    PyObject* pcvals = nullptr;
    PyObject* pcu = nullptr;
    if (! PyArg_ParseTuple(args, "O!O!", &ValueSetPy::Type, &pcvals, &Base::DualNumberPy::Type, &pcu))
        return nullptr;
    Vector ret = getParaCurvePtr()->tangent(*HValueSet(pcvals, false), static_cast<Base::DualNumberPy*>(pcu)->value);
    return ret.getPyObject();
}

PyObject* ParaCurvePy::tangentAtXY(PyObject *args)
{
    PyObject* pcvals = nullptr;
    PyObject* pcu = nullptr;
    if (! PyArg_ParseTuple(args, "O!O!", &ValueSetPy::Type, &pcvals, &VectorPy::Type, &pcu))
        return nullptr;
    Vector ret = getParaCurvePtr()->tangentAtXY(*HValueSet(pcvals, false), Position(static_cast<VectorPy*>(pcu)->value));
    return ret.getPyObject();
}

PyObject* ParaCurvePy::D(PyObject *args)
{
    PyObject* pcvals = nullptr;
    PyObject* pcu = nullptr;
    int n = 0;
    if (! PyArg_ParseTuple(args, "O!O!i", &ValueSetPy::Type, &pcvals, &Base::DualNumberPy::Type, &pcu, &n))
        return nullptr;
    Vector ret = getParaCurvePtr()->D(*HValueSet(pcvals, false), static_cast<Base::DualNumberPy*>(pcu)->value, n);
    return ret.getPyObject();
}

PyObject* ParaCurvePy::length(PyObject *args)
{
    {
        PyObject* pcvals = nullptr;
        PyObject* pcu0 = nullptr;
        PyObject* pcu1 = nullptr;
        if ( PyArg_ParseTuple(args, "O!O!O!", &ValueSetPy::Type, &pcvals, &Base::DualNumberPy::Type, &pcu0, &Base::DualNumberPy::Type, &pcu1)){
            DualNumber ret = getParaCurvePtr()->length(*HValueSet(pcvals, false), static_cast<Base::DualNumberPy*>(pcu0)->value, static_cast<Base::DualNumberPy*>(pcu1)->value);
            return ret.getPyObject();
        }
        PyErr_Clear();
    }
    {
        PyObject* pcvals = nullptr;
        if ( PyArg_ParseTuple(args, "O!", &ValueSetPy::Type, &pcvals)){
            DualNumber ret = getParaCurvePtr()->length(*HValueSet(pcvals, false));
            return ret.getPyObject();
        }
        PyErr_Clear();
    }
    PyErr_SetString(PyExc_TypeError,
        "wrong signature. Supports:"
        "\n"
        "\n (ValueSet)"
        "\n (ValueSet, DualNumber, DualNumber)"
    );
    return nullptr;
}

PyObject* ParaCurvePy::fullLength(PyObject *args)
{
    PyObject* pcvals = nullptr;
    if (! PyArg_ParseTuple(args, "O!", &ValueSetPy::Type, &pcvals))
        return nullptr;
    DualNumber ret = getParaCurvePtr()->fullLength(*HValueSet(pcvals, false));
    return ret.getPyObject();
}

PyObject* ParaCurvePy::pointOnCurveErrFunc(PyObject *args)
{
    PyObject* pcvals = nullptr;
    PyObject* pcpos = nullptr;
    if (! PyArg_ParseTuple(args, "O!O!", &ValueSetPy::Type, &pcvals, &VectorPy::Type, &pcpos))
        return nullptr;
    DualNumber ret = getParaCurvePtr()->pointOnCurveErrFunc(*HValueSet(pcvals, false), Position(static_cast<VectorPy*>(pcpos)->value));
    return ret.getPyObject();
}



Py::Boolean ParaCurvePy::getIsFull(void) const
{
    return Py::Boolean(getParaCurvePtr()->isFull());
}

Py::Boolean ParaCurvePy::getsupports_tangentAtXY(void) const
{
    return Py::Boolean(getParaCurvePtr()->supports_tangentAtXY());
}

Py::Boolean ParaCurvePy::getsupports_D(void) const
{
    return Py::Boolean(getParaCurvePtr()->supports_D());
}

Py::Boolean ParaCurvePy::getsupports_length(void) const
{
    return Py::Boolean(getParaCurvePtr()->supports_length());
}

Py::Boolean ParaCurvePy::getsupports_fullLength(void) const
{
    return Py::Boolean(getParaCurvePtr()->supports_fullLength());
}

Py::Boolean ParaCurvePy::getsupports_pointOnCurveErrFunc(void) const
{
    return Py::Boolean(getParaCurvePtr()->supports_pointOnCurveErrFunc());
}


PyObject *ParaCurvePy::getCustomAttributes(const char* attr) const
{
    return ParaGeometryPy::getCustomAttributes(attr);
}

int ParaCurvePy::setCustomAttributes(const char* attr, PyObject* obj)
{
    return ParaGeometryPy::setCustomAttributes(attr, obj);
}

