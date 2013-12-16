
#include "PreCompiled.h"

#include "Base/Unit.h"

// inclusion of the generated files (generated out of UnitPy.xml)
#include "UnitPy.h"
#include "QuantityPy.h"
#include "UnitPy.cpp"

using namespace Base;

// returns a string which represents the object e.g. when printed in python
std::string UnitPy::representation(void) const
{
    const UnitSignature &  Sig = getUnitPtr()->getSignature();
	std::stringstream ret;
    ret << "Unit: "; 
	ret << getUnitPtr()->getString().toLatin1().constData() << " (";
    ret << Sig.Length << ",";                 
    ret << Sig.Mass  << ",";                    
    ret << Sig.Time  << ",";                   
    ret << Sig.ElectricCurrent  << ",";        
    ret << Sig.ThermodynamicTemperature << ",";
    ret << Sig.AmountOfSubstance  << ",";      
    ret << Sig.LuminoseIntensity  << ",";      
    ret << Sig.Angle  << ")"; 
    std::string type = getUnitPtr()->getTypeString().toLatin1().constData();
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
int UnitPy::PyInit(PyObject* args, PyObject* kwd)
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
    const char* string;
    if (PyArg_ParseTuple(args,"s", &string)) {
            
        *self = Quantity::parse(QString::fromLatin1(string)).getUnit();
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
    return Py::String(getUnitPtr()->getTypeString().toLatin1());
}



PyObject *UnitPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int UnitPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0; 
}


