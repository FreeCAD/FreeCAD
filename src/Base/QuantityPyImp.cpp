
#include "PreCompiled.h"

#include "Base/Quantity.h"
#include "Base/Vector3D.h"

// inclusion of the generated files (generated out of QuantityPy.xml)
#include "QuantityPy.h"
#include "UnitPy.h"
#include "QuantityPy.cpp"

using namespace Base;

// returns a string which represents the object e.g. when printed in python
std::string QuantityPy::representation(void) const
{
	std::stringstream ret;
	ret << getQuantityPtr()->getValue() << " "; 
	ret << getQuantityPtr()->getUnit().getString().toLatin1().constData();

	return ret.str();
}

PyObject *QuantityPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of QuantityPy and the Twin object 
    return new QuantityPy(new Quantity);
}

// constructor method
int QuantityPy::PyInit(PyObject* args, PyObject* kwd)
{
    Quantity *self = getQuantityPtr();

    double f = DOUBLE_MAX;
    int i1=0;
    int i2=0;
    int i3=0;
    int i4=0;
    int i5=0;
    int i6=0;
    int i7=0;
    int i8=0;
    if (PyArg_ParseTuple(args, "|diiiiiiii", &f,&i1,&i2,&i3,&i4,&i5,&i6,&i7,&i8)) {
        if(f!=DOUBLE_MAX)
            *self = Quantity(f,Unit(i1,i2,i3,i4,i5,i6,i7,i8));
        return 0;
    }
    PyErr_Clear(); // set by PyArg_ParseTuple()

    PyObject *object;

    if (PyArg_ParseTuple(args,"O!",&(Base::QuantityPy::Type), &object)) {
        // Note: must be static_cast, not reinterpret_cast
        *self = *(static_cast<Base::QuantityPy*>(object)->getQuantityPtr());
        return 0;
    }
    PyErr_Clear(); // set by PyArg_ParseTuple()
    if (PyArg_ParseTuple(args,"dO!",&f,&(Base::UnitPy::Type), &object)) {
        // Note: must be static_cast, not reinterpret_cast
        *self = Quantity(f,*(static_cast<Base::UnitPy*>(object)->getUnitPtr()));
        return 0;
    }
    PyErr_Clear(); // set by PyArg_ParseTuple()
    const char* string;
    if (PyArg_ParseTuple(args,"s", &string)) {
        try {
            *self = Quantity::parse(QString::fromLatin1(string));
        }catch(const Base::Exception& e) {
            PyErr_SetString(PyExc_ImportError, e.what());
            return-1;
        }

        return 0;
    }

    PyErr_SetString(PyExc_TypeError, "Either three floats, tuple or Vector expected");
    return -1;
}


PyObject* QuantityPy::pow(PyObject * args)
{
    PyErr_SetString(PyExc_NotImplementedError, "Not yet implemented");
    return 0;
}

PyObject* QuantityPy::getUserPreferred(PyObject *args)
{
    QString uus;
    double factor;
    Py::Tuple res(3);

    QString uss = getQuantityPtr()->getUserString(factor,uus);

    res[0] = Py::String(uss.toLatin1());
    res[1] = Py::Float(factor);
    res[2] = Py::String(uus.toLatin1());

    return Py::new_reference_to(res);
}

PyObject* QuantityPy::getValueAs(PyObject *args)
{
    Quantity quant;

    double f = DOUBLE_MAX;
    int i1=0;
    int i2=0;
    int i3=0;
    int i4=0;
    int i5=0;
    int i6=0;
    int i7=0;
    int i8=0;
    if (PyArg_ParseTuple(args, "d|iiiiiiii", &f,&i1,&i2,&i3,&i4,&i5,&i6,&i7,&i8)) {
        if(f!=DOUBLE_MAX)
            quant = Quantity(f,Unit(i1,i2,i3,i4,i5,i6,i7,i8));
        
    }else{
        PyErr_Clear(); // set by PyArg_ParseTuple()

        PyObject *object;

        if (PyArg_ParseTuple(args,"O!",&(Base::QuantityPy::Type), &object)) {
            // Note: must be static_cast, not reinterpret_cast
            quant = * static_cast<Base::QuantityPy*>(object)->getQuantityPtr();
           
        }else{
            PyErr_Clear(); // set by PyArg_ParseTuple()
            const char* string;
            if (PyArg_ParseTuple(args,"s", &string)) {
                    
                quant = Quantity::parse(QString::fromLatin1(string));
                
            }else{
                PyErr_SetString(PyExc_TypeError, "Either three floats, tuple or Vector expected");
                return 0;
            }
        }
    }
    quant = getQuantityPtr()->getValueAs(quant);

    return new QuantityPy(new Quantity(quant) );
}

PyObject* QuantityPy::number_add_handler(PyObject *self, PyObject *other)
{
    if (!PyObject_TypeCheck(self, &(QuantityPy::Type))) {
        PyErr_SetString(PyExc_TypeError, "First arg must be Quantity");
        return 0;
    }
    if (!PyObject_TypeCheck(other, &(QuantityPy::Type))) {
        PyErr_SetString(PyExc_TypeError, "Second arg must be Quantity");
        return 0;
    }
    Base::Quantity *a = static_cast<QuantityPy*>(self)->getQuantityPtr();
    Base::Quantity *b = static_cast<QuantityPy*>(other)->getQuantityPtr();
    return new QuantityPy(new Quantity(*a+*b) );
}

PyObject* QuantityPy::number_subtract_handler(PyObject *self, PyObject *other)
{
    if (!PyObject_TypeCheck(self, &(QuantityPy::Type))) {
        PyErr_SetString(PyExc_TypeError, "First arg must be Quantity");
        return 0;
    }
    if (!PyObject_TypeCheck(other, &(QuantityPy::Type))) {
        PyErr_SetString(PyExc_TypeError, "Second arg must be Quantity");
        return 0;
    }
    Base::Quantity *a = static_cast<QuantityPy*>(self)->getQuantityPtr();
    Base::Quantity *b = static_cast<QuantityPy*>(other)->getQuantityPtr();
    return new QuantityPy(new Quantity(*a-*b) );
}

PyObject* QuantityPy::number_multiply_handler(PyObject *self, PyObject *other)
{
    if (!PyObject_TypeCheck(self, &(QuantityPy::Type))) {
        PyErr_SetString(PyExc_TypeError, "First arg must be Quantity");
        return 0;
    }

    if (PyObject_TypeCheck(other, &(QuantityPy::Type))) {
        Base::Quantity *a = static_cast<QuantityPy*>(self) ->getQuantityPtr();
        Base::Quantity *b = static_cast<QuantityPy*>(other)->getQuantityPtr();
        
        return new QuantityPy(new Quantity(*a * *b) );
    }
    else if (PyFloat_Check(other)) {
        Base::Quantity *a = static_cast<QuantityPy*>(self) ->getQuantityPtr();
        double b = PyFloat_AsDouble(other);
        return new QuantityPy(new Quantity(*a*b) );
    }
    else {
        PyErr_SetString(PyExc_TypeError, "A Quantity can only be multiplied by Quantity or number");
        return 0;
    }
}

PyObject* QuantityPy::richCompare(PyObject *v, PyObject *w, int op)
{
    if (PyObject_TypeCheck(v, &(QuantityPy::Type)) &&
        PyObject_TypeCheck(w, &(QuantityPy::Type))) {
        const Quantity * u1 = static_cast<QuantityPy*>(v)->getQuantityPtr();
        const Quantity * u2 = static_cast<QuantityPy*>(w)->getQuantityPtr();

        PyObject *res=0;
        if (op == Py_NE) {
            res = (!(*u1 == *u2)) ? Py_True : Py_False;
            Py_INCREF(res);
            return res;
        }
        else if (op == Py_LT) {
            res = (*u1 < *u2) ? Py_True : Py_False;
            Py_INCREF(res);
            return res;
        }
        else if (op == Py_LE) {
            res = (*u1 < *u2)||(*u1 == *u2) ? Py_True : Py_False;
            Py_INCREF(res);
            return res;
        }
        else if (op == Py_GT) {
            res = (!(*u1 < *u2))&&(!(*u1 == *u2)) ? Py_True : Py_False;
            Py_INCREF(res);
            return res;
        }
        else if (op == Py_GE) {
            res = (!(*u1 < *u2)) ? Py_True : Py_False;
            Py_INCREF(res);
            return res;
        }
        else if (op == Py_EQ) {
            res = (*u1 == *u2) ? Py_True : Py_False;
            Py_INCREF(res);
            return res;
        }
        
    }
    
    // This always returns False
    Py_INCREF(Py_NotImplemented);
    return Py_NotImplemented;
   
}

Py::Float QuantityPy::getValue(void) const
{
    return Py::Float(getQuantityPtr()->getValue());
}

void QuantityPy::setValue(Py::Float arg)
{
    getQuantityPtr()->setValue(arg);
}

Py::Object QuantityPy::getUnit(void) const
{
    return Py::Object(new UnitPy(new Unit(getQuantityPtr()->getUnit())));
}

void QuantityPy::setUnit(Py::Object arg)
{
    union PyType_Object pyType = {&(Base::UnitPy::Type)};
    Py::Type UnitType(pyType.o);
    if(!arg.isType(UnitType))
        throw Py::AttributeError("Not yet implemented");

    getQuantityPtr()->setUnit(*static_cast<Base::UnitPy*>((*arg))->getUnitPtr());
}


Py::String QuantityPy::getUserString(void) const
{
    return Py::String(getQuantityPtr()->getUserString().toLatin1());
}

PyObject *QuantityPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int QuantityPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0; 
}






