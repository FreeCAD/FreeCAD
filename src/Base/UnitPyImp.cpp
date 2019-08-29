
#include "PreCompiled.h"

#include <Base/Unit.h>

// inclusion of the generated files (generated out of UnitPy.xml)
#include <Base/UnitPy.h>
#include <Base/QuantityPy.h>
#include <Base/UnitPy.cpp>

using namespace Base;

// returns a string which represents the object e.g. when printed in python
std::string UnitPy::representation(void) const
{
    const UnitSignature &  Sig = getUnitPtr()->getSignature();
    std::stringstream ret;
    ret << "Unit: "; 
    ret << getUnitPtr()->getString().toUtf8().constData() << " (";
    ret << Sig.Length << ",";
    ret << Sig.Mass  << ",";
    ret << Sig.Time  << ",";
    ret << Sig.ElectricCurrent  << ",";
    ret << Sig.ThermodynamicTemperature << ",";
    ret << Sig.AmountOfSubstance  << ",";
    ret << Sig.LuminousIntensity  << ",";
    ret << Sig.Angle  << ")"; 
    std::string type = getUnitPtr()->getTypeString().toUtf8().constData();
    if(! type.empty())
        ret << " [" << type << "]";

    return ret.str();
}

PyObject *UnitPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of UnitPy and the Twin object 
    return new UnitPy(new Unit);
}

// constructor method
int UnitPy::PyInit(PyObject* args, PyObject* /*kwd*/)
{
    Unit *self = getUnitPtr();

    int i1=0;
    int i2=0;
    int i3=0;
    int i4=0;
    int i5=0;
    int i6=0;
    int i7=0;
    int i8=0;
    if (PyArg_ParseTuple(args, "|iiiiiiii", &i1,&i2,&i3,&i4,&i5,&i6,&i7,&i8)) {
        *self = Unit(i1,i2,i3,i4,i5,i6,i7,i8);
        return 0;
    }
    PyErr_Clear(); // set by PyArg_ParseTuple()

    PyObject *object;

    if (PyArg_ParseTuple(args,"O!",&(Base::QuantityPy::Type), &object)) {
        // Note: must be static_cast, not reinterpret_cast
        *self = static_cast<Base::QuantityPy*>(object)->getQuantityPtr()->getUnit();
        return 0;
    }
    PyErr_Clear(); // set by PyArg_ParseTuple()

    if (PyArg_ParseTuple(args,"O!",&(Base::UnitPy::Type), &object)) {
        // Note: must be static_cast, not reinterpret_cast
        *self = *(static_cast<Base::UnitPy*>(object)->getUnitPtr());
        return 0;
    }
    PyErr_Clear(); // set by PyArg_ParseTuple()
    char* string;
    if (PyArg_ParseTuple(args,"et", "utf-8", &string)) {
        QString qstr = QString::fromUtf8(string);
        PyMem_Free(string);
        *self = Quantity::parse(qstr).getUnit();
        return 0;
    }

    PyErr_SetString(PyExc_TypeError, "Either string, (float,8 ints), Unit() or Quantity()");
    return -1;
}


PyObject* UnitPy::number_add_handler(PyObject *self, PyObject *other)
{
    if (!PyObject_TypeCheck(self, &(UnitPy::Type))) {
        PyErr_SetString(PyExc_TypeError, "First arg must be Unit");
        return 0;
    }
    if (!PyObject_TypeCheck(other, &(UnitPy::Type))) {
        PyErr_SetString(PyExc_TypeError, "Second arg must be Unit");
        return 0;
    }
    Base::Unit *a = static_cast<UnitPy*>(self)->getUnitPtr();
    Base::Unit *b = static_cast<UnitPy*>(other)->getUnitPtr();

    if (*a != *b) {
        PyErr_SetString(PyExc_TypeError, "Units not matching!");
        return 0;
    }

    return new UnitPy(new Unit(*a));
}

PyObject* UnitPy::number_subtract_handler(PyObject *self, PyObject *other)
{
    if (!PyObject_TypeCheck(self, &(UnitPy::Type))) {
        PyErr_SetString(PyExc_TypeError, "First arg must be Unit");
        return 0;
    }
    if (!PyObject_TypeCheck(other, &(UnitPy::Type))) {
        PyErr_SetString(PyExc_TypeError, "Second arg must be Unit");
        return 0;
    }
    Base::Unit *a = static_cast<UnitPy*>(self)->getUnitPtr();
    Base::Unit *b = static_cast<UnitPy*>(other)->getUnitPtr();

    if (*a != *b) {
        PyErr_SetString(PyExc_TypeError, "Units not matching!");
        return 0;
    }

    return new UnitPy(new Unit(*a));
}

PyObject* UnitPy::number_multiply_handler(PyObject *self, PyObject *other)
{
    if (!PyObject_TypeCheck(self, &(UnitPy::Type))) {
        PyErr_SetString(PyExc_TypeError, "First arg must be Unit");
        return 0;
    }

    if (PyObject_TypeCheck(other, &(UnitPy::Type))) {
        Base::Unit *a = static_cast<UnitPy*>(self) ->getUnitPtr();
        Base::Unit *b = static_cast<UnitPy*>(other)->getUnitPtr();
        
        return new UnitPy(new Unit( (*a) * (*b) ) );
    }
    else {
        PyErr_SetString(PyExc_TypeError, "A Unit can only be multiplied by a Unit");
        return 0;
    }
}

PyObject* UnitPy::richCompare(PyObject *v, PyObject *w, int op)
{
    if (PyObject_TypeCheck(v, &(UnitPy::Type)) &&
        PyObject_TypeCheck(w, &(UnitPy::Type))) {
        const Unit * u1 = static_cast<UnitPy*>(v)->getUnitPtr();
        const Unit * u2 = static_cast<UnitPy*>(w)->getUnitPtr();

        PyObject *res=0;
        if (op != Py_EQ && op != Py_NE) {
            PyErr_SetString(PyExc_TypeError,
            "no ordering relation is defined for Units");
            return 0;
        }
        else if (op == Py_EQ) {
            res = (*u1 == *u2) ? Py_True : Py_False;
            Py_INCREF(res);
            return res;
        }
        else {
            res = (*u1 != *u2) ? Py_True : Py_False;
            Py_INCREF(res);
            return res;
        }
    }
    else {
        // This always returns False
        Py_INCREF(Py_NotImplemented);
        return Py_NotImplemented;
    }
}

Py::String UnitPy::getType(void) const
{
    return Py::String(getUnitPtr()->getTypeString().toUtf8(),"utf-8");
}

Py::Tuple UnitPy::getSignature(void) const
{
    const UnitSignature &  Sig = getUnitPtr()->getSignature();
    Py::Tuple tuple(8);
    tuple.setItem(0, Py::Long(Sig.Length));
    tuple.setItem(1, Py::Long(Sig.Mass));
    tuple.setItem(2, Py::Long(Sig.Time));
    tuple.setItem(3, Py::Long(Sig.ElectricCurrent));
    tuple.setItem(4, Py::Long(Sig.ThermodynamicTemperature));
    tuple.setItem(5, Py::Long(Sig.AmountOfSubstance));
    tuple.setItem(6, Py::Long(Sig.LuminousIntensity));
    tuple.setItem(7, Py::Long(Sig.Angle));
    return tuple;
}



PyObject *UnitPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int UnitPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0; 
}

PyObject * UnitPy::number_divide_handler (PyObject* /*self*/, PyObject* /*other*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return 0;
}

PyObject * UnitPy::number_remainder_handler (PyObject* /*self*/, PyObject* /*other*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return 0;
}

PyObject * UnitPy::number_divmod_handler (PyObject* /*self*/, PyObject* /*other*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return 0;
}

PyObject * UnitPy::number_power_handler (PyObject* /*self*/, PyObject* /*other*/, PyObject* /*modulo*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return 0;
}

PyObject * UnitPy::number_negative_handler (PyObject* /*self*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return 0;
}

PyObject * UnitPy::number_positive_handler (PyObject* /*self*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return 0;
}

PyObject * UnitPy::number_absolute_handler (PyObject* /*self*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return 0;
}

int UnitPy::number_nonzero_handler (PyObject* /*self*/)
{
    return 1;
}

PyObject * UnitPy::number_invert_handler (PyObject* /*self*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return 0;
}

PyObject * UnitPy::number_lshift_handler (PyObject* /*self*/, PyObject* /*other*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return 0;
}

PyObject * UnitPy::number_rshift_handler (PyObject* /*self*/, PyObject* /*other*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return 0;
}

PyObject * UnitPy::number_and_handler (PyObject* /*self*/, PyObject* /*other*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return 0;
}

PyObject * UnitPy::number_xor_handler (PyObject* /*self*/, PyObject* /*other*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return 0;
}

PyObject * UnitPy::number_or_handler (PyObject* /*self*/, PyObject* /*other*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return 0;
}

#if PY_MAJOR_VERSION < 3
int UnitPy::number_coerce_handler (PyObject** /*self*/, PyObject** /*other*/)
{
    return 1;
}
#endif

PyObject * UnitPy::number_int_handler (PyObject* /*self*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return 0;
}

#if PY_MAJOR_VERSION < 3
PyObject * UnitPy::number_long_handler (PyObject* /*self*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return 0;
}
#endif

PyObject * UnitPy::number_float_handler (PyObject* /*self*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return 0;
}

#if PY_MAJOR_VERSION < 3
PyObject * UnitPy::number_oct_handler (PyObject* /*self*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return 0;
}

PyObject * UnitPy::number_hex_handler (PyObject* /*self*/)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not implemented");
    return 0;
}
#endif
